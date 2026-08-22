/*
 * Proficio / Omnia PSoC — Linux USB-HID firmware upload
 *
 * User: set BOOT jumper → power on (Morse LOADER) → stop ms-sdr →
 *   ./bootloader path/to/firmware.cyacd
 *
 * Programs Cypress .cyacd via HID bootloader (VID 0x04B4 PID 0xB71D).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cybtldr_api.h"
#include "cybtldr_api2.h"
#include "cybtldr_utils.h"
#include "hid_comm.h"

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
        "Usage: %s firmware.cyacd\n"
        "\n"
        "Upload Proficio application firmware over USB.\n"
        "\n"
        "  1. Power off, set BOOT jumper, power on (LED: Morse LOADER)\n"
        "  2. Stop ms-sdr / anything holding the radio USB\n"
        "  3. Run:  %s /path/to/Proficio-....cyacd\n"
        "  4. Power off, remove BOOT jumper, power on\n"
        "\n"
        "Expects HID bootloader %04x:%04x. May need sudo or udev rule.\n",
        argv0, argv0, PROFICIO_BL_VID, PROFICIO_BL_PID);
}

int main(int argc, char **argv)
{
    const char *file;
    int err;

    if (argc != 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        usage(argv[0]);
        return (argc == 2) ? 0 : 1;
    }

    file = argv[1];
    if (strlen(file) < 6 || strcmp(file + strlen(file) - 6, ".cyacd") != 0) {
        fprintf(stderr, "Expected a .cyacd file, got: %s\n\n", file);
        usage(argv[0]);
        return 1;
    }

    hid_comm_set_ids(PROFICIO_BL_VID, PROFICIO_BL_PID);

    printf("Programming %s via USB HID %04x:%04x ...\n",
        file, PROFICIO_BL_VID, PROFICIO_BL_PID);
    err = CyBtldr_Program(file, NULL, 0, hid_comm_get(), progress);
    printf("\n");

    if (err == CYRET_SUCCESS) {
        printf("Success — power off, remove BOOT jumper, power on for normal use.\n");
        return 0;
    }

    fprintf(stderr,
        "Failed: Cypress host error 0x%X\n"
        "  Is BOOT jumper set and Morse LOADER showing?\n"
        "  Is ms-sdr stopped? Try: lsusb | grep -i 04b4\n",
        err);
    return 1;
}
