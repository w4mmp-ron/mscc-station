/*
 * psoc-usb-bootload — Linux host for Multus/Proficio PSoC USB HID bootloader
 *
 * Same role as Windows utilities/bootloader.exe (USBBootloaderHost):
 *   put device in bootloader (BOOT jumper or --enter-bootloader), then:
 *     ./psoc-usb-bootload path/to/app.cyacd
 *
 * Protocol: Cypress cybootloaderutils + HID (VID 0x04B4 PID 0xB71D default).
 * App commands: libusb vendor OUT to 0x16C0:0x05DC (CMD 0x0E / 0x0F).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <getopt.h>

#include "cybtldr_api.h"
#include "cybtldr_api2.h"
#include "cybtldr_utils.h"
#include "hid_comm.h"
#include "app_usb.h"

static void progress(uint8_t arrayId, uint16_t rowNum)
{
    (void)arrayId;
    (void)rowNum;
    fputc('.', stdout);
    fflush(stdout);
}

static void usage(const char *argv0)
{
    fprintf(stderr,
        "Usage:\n"
        "  %s [options] firmware.cyacd\n"
        "  %s --enter-bootloader\n"
        "  %s --reboot-app\n"
        "  %s --list\n"
        "\n"
        "Linux USB-HID bootloader host for Proficio / Omnia PSoC\n"
        "(same role as Windows bootloader.exe / USBBootloaderHost).\n"
        "\n"
        "Options:\n"
        "  -e, --enter-bootloader  Send USB CMD 0x0E to running app → PSoC bootloader\n"
        "  -r, --reboot-app        Send USB CMD 0x0F to running app → soft reset only\n"
        "  -l, --list              List HID devices and exit\n"
        "  -v, --vid HEX           HID bootloader VID (default %04x) or app VID with -e/-r\n"
        "  -p, --pid HEX           HID bootloader PID (default %04x) or app PID with -e/-r\n"
        "  -d, --device PATH       hidapi device path (from --list)\n"
        "  -w, --wait SEC          After -e, wait SEC seconds before programming (default 2)\n"
        "  -h, --help              This help\n"
        "\n"
        "Examples:\n"
        "  # 1) Put Proficio app into bootloader (needs app firmware with CMD 0x0E)\n"
        "  %s --enter-bootloader\n"
        "\n"
        "  # 2) Soft-reboot application only (CMD 0x0F)\n"
        "  %s --reboot-app\n"
        "\n"
        "  # 3) List HID devices (look for 04b4:b71d after BOOT / -e)\n"
        "  %s --list\n"
        "\n"
        "  # 4) Program a .cyacd (device already in bootloader)\n"
        "  %s Release/Proficio-MKII-PTT-20260817.cyacd\n"
        "\n"
        "  # 5) Enter bootloader, wait, then program\n"
        "  %s -e -w 3 Release/Proficio-MKII-PTT.cyacd\n"
        "\n"
        "  # 6) Custom app VID/PID for -e/-r (defaults 16c0:05dc)\n"
        "  %s -e -v 16c0 -p 05dc\n"
        "\n"
        "Notes:\n"
        "  - BOOT jumper still works without -e.\n"
        "  - Programming uses HID %04x:%04x; -e/-r use app %04x:%04x by default.\n"
        "  - May need sudo or udev rule for USB access.\n",
        argv0, argv0, argv0, argv0,
        PROFICIO_BL_VID, PROFICIO_BL_PID,
        argv0, argv0, argv0, argv0, argv0, argv0,
        PROFICIO_BL_VID, PROFICIO_BL_PID,
        PROFICIO_APP_VID, PROFICIO_APP_PID);
}

int main(int argc, char **argv)
{
    const char *file = NULL;
    int do_list = 0;
    int do_enter_bl = 0;
    int do_reboot_app = 0;
    unsigned vid = 0; /* 0 = use mode-specific default */
    unsigned pid = 0;
    const char *devpath = NULL;
    int wait_sec = 2;
    int err;
    int pad = 0;
    static struct option long_opts[] = {
        {"list", no_argument, 0, 'l'},
        {"enter-bootloader", no_argument, 0, 'e'},
        {"reboot-app", no_argument, 0, 'r'},
        {"vid", required_argument, 0, 'v'},
        {"pid", required_argument, 0, 'p'},
        {"device", required_argument, 0, 'd'},
        {"wait", required_argument, 0, 'w'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    for (;;) {
        int c = getopt_long(argc, argv, "lerv:p:d:w:h", long_opts, NULL);
        if (c < 0)
            break;
        switch (c) {
        case 'l':
            do_list = 1;
            break;
        case 'e':
            do_enter_bl = 1;
            break;
        case 'r':
            do_reboot_app = 1;
            break;
        case 'v':
            vid = (unsigned)strtoul(optarg, NULL, 16);
            break;
        case 'p':
            pid = (unsigned)strtoul(optarg, NULL, 16);
            break;
        case 'd':
            devpath = optarg;
            break;
        case 'w':
            wait_sec = atoi(optarg);
            if (wait_sec < 0)
                wait_sec = 0;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }

    if (do_list) {
        printf("HID devices:\n");
        return hid_comm_list() < 0 ? 1 : 0;
    }

    if (do_enter_bl && do_reboot_app) {
        fprintf(stderr, "Use only one of --enter-bootloader or --reboot-app\n");
        return 1;
    }

    /* App vendor commands (running firmware, not HID bootloader) */
    if (do_enter_bl || do_reboot_app) {
        unsigned short app_vid = (unsigned short)(vid ? vid : PROFICIO_APP_VID);
        unsigned short app_pid = (unsigned short)(pid ? pid : PROFICIO_APP_PID);
        unsigned char cmd = do_enter_bl ? CMD_ENTER_BOOTLOADER : CMD_REBOOT_APP;

        printf("%s on app USB %04x:%04x ...\n",
            do_enter_bl ? "Enter bootloader (0x0E)" : "Reboot app (0x0F)",
            app_vid, app_pid);
        if (app_usb_vendor_out(app_vid, app_pid, cmd, &pad, sizeof(pad)) != 0)
            return 1;

        if (do_reboot_app) {
            printf("App soft-reset requested.\n");
            return 0;
        }

        /* Optional: program after enter-bootloader if .cyacd given */
        if (optind >= argc) {
            printf("Device should re-enumerate as HID %04x:%04x.\n"
                   "Then: %s firmware.cyacd\n",
                PROFICIO_BL_VID, PROFICIO_BL_PID, argv[0]);
            return 0;
        }
        if (wait_sec > 0) {
            printf("Waiting %d s for HID bootloader ...\n", wait_sec);
            sleep((unsigned)wait_sec);
        }
        /* Fall through to program — reset vid/pid to bootloader defaults */
        vid = 0;
        pid = 0;
    }

    if (optind >= argc) {
        usage(argv[0]);
        return 1;
    }
    file = argv[optind];

    {
        unsigned short bl_vid = (unsigned short)(vid ? vid : PROFICIO_BL_VID);
        unsigned short bl_pid = (unsigned short)(pid ? pid : PROFICIO_BL_PID);
        hid_comm_set_ids(bl_vid, bl_pid);
        if (devpath)
            hid_comm_set_path(devpath);

        printf("Programming %s via USB HID %04x:%04x ...\n", file, bl_vid, bl_pid);
        err = CyBtldr_Program(file, NULL, 0, hid_comm_get(), progress);
        printf("\n");
        if (err == CYRET_SUCCESS) {
            printf("Success — device should reboot into the application.\n");
            return 0;
        }
        fprintf(stderr, "Failed: Cypress host error 0x%X\n", err);
        return 1;
    }
}
