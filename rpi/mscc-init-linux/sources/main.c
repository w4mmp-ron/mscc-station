/*
 * mscc-init (Linux) — user-friendly interactive base config for MSCC-MKII.
 *
 * Writes under $HOME/.local/mscc/ (same tree ms-sdr / sdrcore use):
 *   mscc.ini, cw.ini, i2c.ini, comm-port.ini,
 *   operator speaker & microphone .ini files (user picks)
 *   digital speaker/mic fixed: VirtualA / VirtualB.monitor (install digi path)
 *
 * - Multus identity: libusb VID 0x16C0 PID 0x05DC
 * - Operator audio: PortAudio device name files (user selects)
 * - Digi audio: always VirtualA + VirtualB.monitor (mscc-virtual-audio)
 * - CAT: PTY or /dev path (ttyUSB, ttyACM, ttyS, tnt/tty0tty)
 * - PTT pin for ms-sdr: PIN=0 off, 1=CTS, 2=DCD
 */
#include "platform.h"
#include "port_defines.h"
#include <libusb-1.0/libusb.h>
#include <ctype.h>
#include <stdarg.h>
#include <strings.h>
#include <sys/stat.h>
#include <errno.h>

#define MULTUS_VID 0x16C0
#define MULTUS_PID 0x05DC
#define MAX_INPUT_DEVICES 50
#define MAX_SERIAL_DEVS 64
#define LINE_MAX_LEN 256

struct input_devices {
    int record_number;
    int device_index;
    char name[512];
    int num_channels;
};

struct output_devices {
    int record_number;
    int device_index;
    char name[512];
    int num_channels;
};

struct serial_choice {
    char path[128];   /* "PTY" or "/dev/..." */
    char label[160];  /* short help text */
};

static struct input_devices G_input_devices[MAX_INPUT_DEVICES];
static struct output_devices G_output_devices[MAX_INPUT_DEVICES];
static int num_input_devices_found = 0;
static int num_output_devices_found = 0;

static struct serial_choice G_serial_list[MAX_SERIAL_DEVS];
static int G_serial_count = 0;

static char G_Usb_serial_number[MAX_PATH];
static char G_string_host_name[MAX_PATH];
static char G_string_server_IP[MAX_PATH];
static int G_mscc_port;
static int G_server_port;
static uint8_t G_PCB_Version = 10;
static uint8_t G_Keyer_Installed = 0;
/* 1 = Proficio MKII (ms-sdr PTT sense thread); 0 = legacy. mscc.ini PROFICIO-MKII= */
static uint8_t G_Proficio_Mkii = 1;
static char G_l_path[MAX_PATH];
static const PaDeviceInfo *lpInfo;

/* Written-file summary for the end screen */
static char G_summary_lines[24][200];
static int G_summary_count = 0;

static void summary_add(const char *fmt, ...)
{
    va_list ap;
    if (G_summary_count >= 24)
        return;
    va_start(ap, fmt);
    vsnprintf(G_summary_lines[G_summary_count], sizeof(G_summary_lines[0]), fmt, ap);
    va_end(ap);
    G_summary_count++;
}

static void banner(const char *title)
{
    printf("\n");
    printf("============================================================\n");
    printf("  %s\n", title);
    printf("============================================================\n");
}

static void step_banner(int step, int total, const char *title)
{
    printf("\n");
    printf("------------------------------------------------------------\n");
    printf("  Step %d of %d — %s\n", step, total, title);
    printf("------------------------------------------------------------\n");
}

/* Read a full line; strips CR/LF. Returns 0 on EOF. */
static int read_line(char *buf, size_t buflen)
{
    size_t n;
    if (buflen == 0)
        return 0;
    if (fgets(buf, (int)buflen, stdin) == NULL) {
        buf[0] = '\0';
        return 0;
    }
    n = strlen(buf);
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
        buf[--n] = '\0';
    }
    return 1;
}

/* Trim leading/trailing spaces in place. */
static void trim_inplace(char *s)
{
    char *start;
    size_t n;
    if (s == NULL)
        return;
    start = s;
    while (*start && isspace((unsigned char)*start))
        start++;
    if (start != s)
        memmove(s, start, strlen(start) + 1);
    n = strlen(s);
    while (n > 0 && isspace((unsigned char)s[n - 1]))
        s[--n] = '\0';
}

/*
 * Prompt for integer in [lo, hi]. Empty line → default_val.
 * Re-prompts on bad input. Prints the question (include default in text).
 */
static int prompt_int(const char *question, int lo, int hi, int default_val)
{
    char line[LINE_MAX_LEN];
    long v;
    char *end;

    for (;;) {
        printf("%s", question);
        fflush(stdout);
        if (!read_line(line, sizeof(line))) {
            printf("\n(EOF — using %d)\n", default_val);
            return default_val;
        }
        trim_inplace(line);
        if (line[0] == '\0')
            return default_val;
        errno = 0;
        v = strtol(line, &end, 10);
        if (end == line || *end != '\0' || errno != 0) {
            printf("  Please enter a number");
            if (lo <= hi)
                printf(" from %d to %d", lo, hi);
            printf(" (or Enter for default %d).\n", default_val);
            continue;
        }
        if (v < lo || v > hi) {
            printf("  Out of range — use %d…%d (or Enter for %d).\n",
                lo, hi, default_val);
            continue;
        }
        return (int)v;
    }
}

/* y/n prompt. Empty → default_yes. */
static int prompt_yes_no(const char *question, int default_yes)
{
    char line[LINE_MAX_LEN];
    char c;

    for (;;) {
        printf("%s [%s]: ", question, default_yes ? "Y/n" : "y/N");
        fflush(stdout);
        if (!read_line(line, sizeof(line)))
            return default_yes;
        trim_inplace(line);
        if (line[0] == '\0')
            return default_yes;
        c = (char)tolower((unsigned char)line[0]);
        if (c == 'y')
            return 1;
        if (c == 'n')
            return 0;
        printf("  Please answer y or n (or Enter for default).\n");
    }
}

static int ensure_config_dir(const char *dir)
{
    struct stat st;
    if (stat(dir, &st) == 0) {
        if (S_ISDIR(st.st_mode))
            return 0;
        fprintf(stderr, "ERROR: %s exists but is not a directory\n", dir);
        return -1;
    }
    if (mkdir(dir, 0755) != 0 && errno != EEXIST) {
        fprintf(stderr, "ERROR: cannot create %s: %s\n", dir, strerror(errno));
        return -1;
    }
    printf("Created config directory: %s\n", dir);
    return 0;
}

char *My_getenv(char *myenv)
{
    memset(G_l_path, 0, sizeof(G_l_path));
    {
        const char *home = getenv("HOME");
        (void)myenv;
        if (home == NULL || home[0] == '\0')
            home = "/tmp";
        snprintf(G_l_path, sizeof(G_l_path), "%s/.local/mscc", home);
    }
    return G_l_path;
}

/* --- Multus control USB (libusb-1.0): serial number only --- */
static int read_multus_serial(void)
{
    libusb_context *ctx = NULL;
    libusb_device **list = NULL;
    ssize_t cnt, i;
    int rc;
    int found = -1;

    G_Usb_serial_number[0] = '\0';
    printf("Looking for Multus/Proficio USB control (VID=0x%04X PID=0x%04X)…\n",
        MULTUS_VID, MULTUS_PID);

    rc = libusb_init(&ctx);
    if (rc != 0) {
        fprintf(stderr, "  libusb_init failed: %s\n", libusb_strerror(rc));
        return -1;
    }
    cnt = libusb_get_device_list(ctx, &list);
    if (cnt < 0) {
        fprintf(stderr, "  libusb_get_device_list failed\n");
        libusb_exit(ctx);
        return -1;
    }
    for (i = 0; i < cnt; i++) {
        struct libusb_device_descriptor desc;
        libusb_device_handle *h = NULL;
        unsigned char sn[256];

        if (libusb_get_device_descriptor(list[i], &desc) != 0)
            continue;
        if (desc.idVendor != MULTUS_VID || desc.idProduct != MULTUS_PID)
            continue;

        printf("  Found transceiver on USB.\n");
        if (libusb_open(list[i], &h) != 0) {
            fprintf(stderr,
                "  Open failed (permissions?). Try:\n"
                "    sudo mscc-init\n"
                "  or a udev rule for 16c0:05dc.\n");
            continue;
        }
        if (desc.iSerialNumber != 0) {
            rc = libusb_get_string_descriptor_ascii(h, desc.iSerialNumber,
                sn, sizeof(sn));
            if (rc > 0) {
                sn[rc < (int)sizeof(sn) ? rc : (int)sizeof(sn) - 1] = '\0';
                strncpy(G_Usb_serial_number, (char *)sn,
                    sizeof(G_Usb_serial_number) - 1);
                printf("  Serial number: %s\n", G_Usb_serial_number);
                found = 0;
            }
        } else {
            printf("  Device has no USB serial string — using UNKNOWN.\n");
            strncpy(G_Usb_serial_number, "UNKNOWN",
                sizeof(G_Usb_serial_number) - 1);
            found = 0;
        }
        libusb_close(h);
        if (found == 0)
            break;
    }
    libusb_free_device_list(list, 1);
    libusb_exit(ctx);
    if (found < 0)
        printf("  No Multus USB device found (you can still configure CAT/audio).\n");
    return found;
}

static int write_ok(const char *path, const char *what)
{
    printf("  OK  wrote %s\n      → %s\n", what, path);
    return 0;
}

static int write_fail(const char *path, const char *what)
{
    printf("  FAIL  could not write %s\n       → %s (%s)\n",
        what, path, strerror(errno));
    return -1;
}

static int init_mscc(void)
{
    FILE *fp;
    char path[PATH_MAX];
    const char *home = My_getenv("HOME");

    snprintf(path, sizeof(path), "%s/mscc.ini", home);
    fp = fopen(path, "w");
    if (!fp)
        return write_fail(path, "mscc.ini");
    fprintf(fp, "PROFICIO_SERIAL_NUMBER=%s;\n", G_Usb_serial_number);
    fprintf(fp, "MSCC_PORT=%d;\n", G_mscc_port);
    fprintf(fp, "MSCC_IP=%s;\n", G_string_host_name);
    fprintf(fp, "PROFICIO_DLL_PORT=%d;\n", G_server_port);
    fprintf(fp, "PROFICIO_DLL_IP=%s;\n", G_string_server_IP);
    fprintf(fp, "PCB_VERSION=%d;\n", G_PCB_Version);
    fprintf(fp, "PROFICIO-MKII=%d;\n", (int)G_Proficio_Mkii);
    fclose(fp);
    summary_add("mscc.ini          serial=%s  client→%s:%d  ms-sdr port %d  MKII=%d",
        G_Usb_serial_number, G_string_host_name, G_mscc_port, G_server_port,
        (int)G_Proficio_Mkii);
    return write_ok(path, "mscc.ini (network + serial + PROFICIO-MKII)");
}

static int Create_Fortis_I2C(int mfc, int meter)
{
    FILE *fp;
    char path[PATH_MAX];
    const char *home = My_getenv("HOME");

    snprintf(path, sizeof(path), "%s/i2c.ini", home);
    fp = fopen(path, "w");
    if (!fp)
        return write_fail(path, "i2c.ini");
    fputs("G_MASTER_CONTROLLER_attached=2;\n", fp);
    fprintf(fp, "G_MFC_attached=%d;\n", mfc);
    fputs("G_SOLIDUS_TEMP_SENSOR_attached=0;\n", fp);
    fprintf(fp, "G_METER_attached=%d;\n", meter);
    fputs("G_IQBD_attached=0;\n", fp);
    fputs("G_CURRENT_SENSOR_attached=0;\n", fp);
    fclose(fp);
    summary_add("i2c.ini           Fortis defaults (MFC=%d meter=%d)", mfc, meter);
    return write_ok(path, "i2c.ini");
}

static int Update_CW_ini(void)
{
    FILE *fp;
    char path[PATH_MAX];
    const char *home = My_getenv("HOME");

    snprintf(path, sizeof(path), "%s/cw.ini", home);
    fp = fopen(path, "w");
    if (!fp)
        return write_fail(path, "cw.ini");
    fprintf(fp, "CW_Keyer_Installed=%d;\n", G_Keyer_Installed);
    fprintf(fp, "CW_Keyer_Mode=%d;\n", 0);
    fprintf(fp, "CW_Iambic_Type=%d;\n", 0);
    fprintf(fp, "CW_Iambic_Calibrate=%d;\n", 120);
    fprintf(fp, "CW_Memory=%d;\n", 0);
    fprintf(fp, "CW_Spacing=%d;\n", 0);
    fprintf(fp, "CW_Paddle=%d;\n", 0);
    fprintf(fp, "CW_Weight=%d;\n", 50);
    fprintf(fp, "CW_Tx_Hold=%d;\n", 15);
    fprintf(fp, "CW_Speed=%d;\n", 18);
    fprintf(fp, "CW_Semi_Break_In=%d;\n", 0);
    fprintf(fp, "CW_Semi_Control=%d;\n", 0);
    fprintf(fp, "CW_Side_Tone_Volume=%d;\n", 0);
    fclose(fp);
    summary_add("cw.ini            keyer %s",
        G_Keyer_Installed ? "INSTALLED" : "not installed");
    return write_ok(path, "cw.ini");
}

static int is_pty_name(const char *port_name)
{
    if (port_name == NULL || port_name[0] == '\0')
        return 1;
    if (strcmp(port_name, "0") == 0)
        return 1;
    if (strcasecmp(port_name, "PTY") == 0)
        return 1;
    if (strcasecmp(port_name, "pty") == 0)
        return 1;
    return 0;
}

static int Update_Serial_Config_linux(const char *port_name, int pin_mode)
{
    FILE *fp;
    char path[PATH_MAX];
    const char *home = My_getenv("HOME");
    const char *name;
    int pin = pin_mode;
    int is_pty;
    const char *pin_desc = "off (CAT only)";

    if (pin < 0 || pin > 2)
        pin = 0;

    is_pty = is_pty_name(port_name);
    if (is_pty) {
        name = "PTY";
        if (pin != 0) {
            printf("  Note: PTY has no modem lines — PIN forced to 0.\n");
            printf("        For RTS/CTS PTT use a tty0tty pair (/dev/tnt0 + /dev/tnt1).\n");
            pin = 0;
        }
    } else {
        name = port_name;
    }

    if (pin == 1)
        pin_desc = "CTS (digi app: assert RTS on the other end of the pair)";
    else if (pin == 2)
        pin_desc = "DCD";

    snprintf(path, sizeof(path), "%s/comm-port.ini", home);
    fp = fopen(path, "w");
    if (!fp)
        return write_fail(path, "comm-port.ini");
    fprintf(fp,
        "COMM_PORT_NAME=%s,COMM_PORT_INDEX=0,BAUD_RATE_INDEX=3,"
        "PARITY_INDEX=0,DATA_BITS_INDEX=1,STOP_BITS_INDEX=0,PIN=%d;\n",
        name, pin);
    fclose(fp);

    if (is_pty)
        printf("  CAT = PTY  → ms-sdr will create $HOME/ms-sdr-cat\n");
    else
        printf("  CAT = %s\n", name);
    printf("  PIN = %d  → %s\n", pin, pin_desc);
    summary_add("comm-port.ini     CAT=%s  PIN=%d (%s)", name, pin,
        pin == 0 ? "off" : (pin == 1 ? "CTS" : "DCD"));
    return write_ok(path, "comm-port.ini");
}

static int is_listed_serial_name(const char *d_name)
{
    if (d_name == NULL || d_name[0] == '\0')
        return 0;
    if (strncmp(d_name, "ttyUSB", 6) == 0)
        return 1;
    if (strncmp(d_name, "ttyACM", 6) == 0)
        return 1;
    if (strncmp(d_name, "ttyS", 4) == 0)
        return 1;
    if (strncmp(d_name, "tnt", 3) == 0)
        return 1;
    return 0;
}

static void serial_add(const char *path, const char *label)
{
    if (G_serial_count >= MAX_SERIAL_DEVS)
        return;
    snprintf(G_serial_list[G_serial_count].path,
        sizeof(G_serial_list[0].path), "%s", path);
    snprintf(G_serial_list[G_serial_count].label,
        sizeof(G_serial_list[0].label), "%s", label ? label : "");
    G_serial_count++;
}

static int cmp_serial(const void *a, const void *b)
{
    const struct serial_choice *sa = a;
    const struct serial_choice *sb = b;
    return strcmp(sa->path, sb->path);
}

static void build_serial_menu(void)
{
    DIR *d;
    struct dirent *e;
    int i;

    G_serial_count = 0;
    serial_add("PTY", "Kenwood CAT text only (no hardware PTT pins)");

    d = opendir("/dev");
    if (d) {
        while ((e = readdir(d)) != NULL) {
            char path[128];
            const char *hint = "serial device";
            if (!is_listed_serial_name(e->d_name))
                continue;
            snprintf(path, sizeof(path), "/dev/%s", e->d_name);
            if (strncmp(e->d_name, "tnt", 3) == 0)
                hint = "tty0tty pair end — good for CAT + RTS/CTS PTT";
            else if (strncmp(e->d_name, "ttyUSB", 6) == 0)
                hint = "USB serial";
            else if (strncmp(e->d_name, "ttyACM", 6) == 0)
                hint = "USB ACM serial";
            serial_add(path, hint);
        }
        closedir(d);
    }

    /* Keep PTY first; sort the rest */
    if (G_serial_count > 2)
        qsort(G_serial_list + 1, (size_t)(G_serial_count - 1),
            sizeof(G_serial_list[0]), cmp_serial);

    printf("\n  Available CAT ports:\n");
    for (i = 0; i < G_serial_count; i++) {
        printf("    [%2d]  %-16s  %s\n", i,
            G_serial_list[i].path, G_serial_list[i].label);
    }
    printf("\n  Tips:\n");
    printf("    • PTY  — easiest for CAT only; digital apps use ~/ms-sdr-cat\n");
    printf("    • tty0tty — put ms-sdr on one end (e.g. tnt0), digi app on the other (tnt1)\n");
    printf("    • PIN=1 (CTS) is the usual PTT sense when digi asserts RTS on the pair\n");
}

static void choose_cat_and_pin(void)
{
    int idx;
    int pin;
    int default_pin;
    char custom[LINE_MAX_LEN];
    const char *port;
    int is_tnt;

    build_serial_menu();

    idx = prompt_int(
        "\n  Select CAT port number (Enter = 0 / PTY): ",
        0, G_serial_count > 0 ? G_serial_count - 1 : 0, 0);

    if (G_serial_count == 0) {
        port = "PTY";
    } else {
        port = G_serial_list[idx].path;
    }

    /* Optional custom path override */
    if (!is_pty_name(port)) {
        printf("  Using %s. Press Enter to accept, or type another full path: ",
            port);
        fflush(stdout);
        if (read_line(custom, sizeof(custom))) {
            trim_inplace(custom);
            if (custom[0] != '\0')
                port = custom;
        }
    }

    if (is_pty_name(port)) {
        printf("\n  PTY selected — hardware PTT pins are not available.\n");
        Update_Serial_Config_linux("PTY", 0);
        return;
    }

    is_tnt = (strstr(port, "tnt") != NULL);
    default_pin = is_tnt ? 1 : 0;

    printf("\n  PTT pin sense (ms-sdr reads this on the CAT port):\n");
    printf("    0 = none     CAT only, no pin PTT\n");
    printf("    1 = CTS      recommended for tty0tty (digi app RTS → this end CTS)\n");
    printf("    2 = DCD      carrier-detect style PTT\n");

    {
        char q[160];
        snprintf(q, sizeof(q),
            "  Select PIN (Enter = %d): ", default_pin);
        pin = prompt_int(q, 0, 2, default_pin);
    }

    Update_Serial_Config_linux(port, pin);
}

/* Fixed digi path — created by mscc-virtual-audio; not user-selected. */
#define MSCC_DIGI_SPEAKER "VirtualA"
#define MSCC_DIGI_MIC     "VirtualB.monitor"

/* Digi Virtual* / monitors are not operator devices — hide from pick lists. */
static int is_digi_only_device(const char *name)
{
    if (name == NULL || name[0] == '\0')
        return 1;
    if (strstr(name, "VirtualA") || strstr(name, "VirtualB"))
        return 1;
    if (strstr(name, ".monitor") || strstr(name, "Monitor of"))
        return 1;
    if (strstr(name, "MSCC_Cable") || strstr(name, "MSCCLoop"))
        return 1;
    if (strstr(name, "MSCC_Digi"))
        return 1;
    return 0;
}

static const char *device_hint(const char *name)
{
    if (name == NULL)
        return "";
    if (strstr(name, "Multus") || strstr(name, "Proficio"))
        return "  ← radio I/Q (usually NOT operator phones)";
    if (strstr(name, "audioinjector") || strstr(name, "AudioInjector") ||
        strstr(name, "wm8731"))
        return "  ← typical operator sound card";
    if (strstr(name, "hdmi") || strstr(name, "HDMI"))
        return "  ← HDMI (rarely useful for radio)";
    if (strstr(name, "Loopback") || strstr(name, "loopback") ||
        strstr(name, "aloop"))
        return "  ← ALSA loopback (usually not operator)";
    return "";
}

static void Get_Speaker_Audio_Device(void)
{
    PaDeviceIndex j, devCount = Pa_GetDeviceCount();
    const PaHostApiInfo *apiInfo;

    num_output_devices_found = 0;
    printf("\n  Operator playback devices (headphones / speaker):\n");
    for (j = 0; j < devCount && num_output_devices_found < MAX_INPUT_DEVICES; j++) {
        lpInfo = Pa_GetDeviceInfo(j);
        if (!lpInfo || !lpInfo->name || lpInfo->maxOutputChannels < 1)
            continue;
        if (is_digi_only_device(lpInfo->name))
            continue;
        strncpy(G_output_devices[num_output_devices_found].name, lpInfo->name,
            sizeof(G_output_devices[0].name) - 1);
        G_output_devices[num_output_devices_found].device_index = (int)j;
        G_output_devices[num_output_devices_found].num_channels =
            lpInfo->maxOutputChannels;
        G_output_devices[num_output_devices_found].record_number =
            num_output_devices_found;
        apiInfo = Pa_GetHostApiInfo(lpInfo->hostApi);
        printf("    [%2d]  ch=%d  %-6s  %s%s\n",
            num_output_devices_found,
            lpInfo->maxOutputChannels,
            (apiInfo && apiInfo->name) ? apiInfo->name : "?",
            lpInfo->name,
            device_hint(lpInfo->name));
        num_output_devices_found++;
    }
    if (num_output_devices_found == 0)
        printf("    (none found)\n");
}

static void Get_Microphone_Audio_Device(void)
{
    PaDeviceIndex j, devCount = Pa_GetDeviceCount();
    const PaHostApiInfo *apiInfo;

    num_input_devices_found = 0;
    printf("\n  Operator capture devices (microphone):\n");
    for (j = 0; j < devCount && num_input_devices_found < MAX_INPUT_DEVICES; j++) {
        lpInfo = Pa_GetDeviceInfo(j);
        if (!lpInfo || !lpInfo->name || lpInfo->maxInputChannels < 1)
            continue;
        if (is_digi_only_device(lpInfo->name))
            continue;
        strncpy(G_input_devices[num_input_devices_found].name, lpInfo->name,
            sizeof(G_input_devices[0].name) - 1);
        G_input_devices[num_input_devices_found].device_index = (int)j;
        G_input_devices[num_input_devices_found].num_channels =
            lpInfo->maxInputChannels;
        G_input_devices[num_input_devices_found].record_number =
            num_input_devices_found;
        apiInfo = Pa_GetHostApiInfo(lpInfo->hostApi);
        printf("    [%2d]  ch=%d  %-6s  %s%s\n",
            num_input_devices_found,
            lpInfo->maxInputChannels,
            (apiInfo && apiInfo->name) ? apiInfo->name : "?",
            lpInfo->name,
            device_hint(lpInfo->name));
        num_input_devices_found++;
    }
    if (num_input_devices_found == 0)
        printf("    (none found)\n");
}

static int write_device_name_file(const char *filename, const char *devname)
{
    FILE *fp;
    char path[PATH_MAX];
    char namebuf[512];
    char *name;
    const char *home = My_getenv("HOME");

    snprintf(path, sizeof(path), "%s/%s", home, filename);
    fp = fopen(path, "w");
    if (!fp)
        return write_fail(path, filename);
    strncpy(namebuf, devname, sizeof(namebuf) - 1);
    namebuf[sizeof(namebuf) - 1] = '\0';
    name = strtok(namebuf, "(");
    if (name == NULL)
        name = namebuf;
    while (*name == ' ')
        name++;
    /* trim trailing spaces */
    {
        size_t n = strlen(name);
        while (n > 0 && isspace((unsigned char)name[n - 1]))
            name[--n] = '\0';
    }
    fprintf(fp, "%s", name);
    fclose(fp);
    summary_add("%-18s \"%s\"", filename, name);
    return write_ok(path, filename);
}

static int Update_Operator_Speaker_Config(int record)
{
    if (record < 0 || record >= num_output_devices_found)
        return -1;
    return write_device_name_file("operator-speaker.ini",
        G_output_devices[record].name);
}

static int Update_Operator_Microphone_Config(int record)
{
    if (record < 0 || record >= num_input_devices_found)
        return -1;
    return write_device_name_file("operator-microphone.ini",
        G_input_devices[record].name);
}

/* Digi path is install-time VirtualA / VirtualB.monitor — always rewrite. */
static int Write_Fixed_Digital_Audio_Config(void)
{
    int st = 0;
    printf("\n  Digi audio (fixed by install — not selectable):\n");
    printf("    digital speaker = %s\n", MSCC_DIGI_SPEAKER);
    printf("    digital mic     = %s\n", MSCC_DIGI_MIC);
    if (write_device_name_file("digital-speaker.ini", MSCC_DIGI_SPEAKER) < 0)
        st = -1;
    if (write_device_name_file("digital-microphone.ini", MSCC_DIGI_MIC) < 0)
        st = -1;
    return st;
}

static void Audio_Device_Error(PaError err)
{
    fprintf(stderr, "PortAudio error %d: %s\n", err,
        err != paNoError ? Pa_GetErrorText(err) : "(none)");
    fprintf(stderr, "Install: sudo apt-get install -y libportaudio2 portaudio19-dev\n");
    Pa_Terminate();
    exit(1);
}

static void print_final_summary(void)
{
    int i;
    banner("Done — configuration summary");
    printf("  Directory: %s\n\n", My_getenv("HOME"));
    for (i = 0; i < G_summary_count; i++)
        printf("  • %s\n", G_summary_lines[i]);
    printf("\n  Next steps:\n");
    printf("    1. Digi sinks:  pactl list short sinks | grep Virtual\n");
    printf("       (if missing: mscc-virtual-audio)\n");
    printf("    2. Start stack:  mscc start\n");
    printf("    3. Confirm ms-sdr log shows your CAT path and PIN\n");
    printf("\n");
}

int main(int argc, char **argv)
{
    int status;
    int exit_status = 0;
    int mfc = 0, meter = 0;
    PaError err;
    int speaker_device;
    int microphone_device;
    const char *cfg;
    int total_steps = 4;

    (void)argc;
    (void)argv;

    banner("mscc-init (Linux) — MSCC-MKII base setup");
    printf("  This wizard writes the .ini files used by:\n");
    printf("    ms-sdr · sdrcore-recv · sdrcore-trans\n");
    printf("  Press Enter at any prompt to accept the default in [brackets].\n");

    cfg = My_getenv("HOME");
    printf("\n  Config directory: %s\n", cfg);
    if (ensure_config_dir(cfg) < 0)
        return 1;

    /* ---- Step 1: radio USB ---- */
    step_banner(1, total_steps, "Transceiver USB (optional)");
    status = read_multus_serial();
    G_mscc_port = MSCC_PORT;
    G_server_port = MS_SDR_PORT;
    if (gethostname(G_string_server_IP, sizeof(G_string_server_IP)) != 0)
        strncpy(G_string_server_IP, "127.0.0.1", sizeof(G_string_server_IP) - 1);
    strncpy(G_string_host_name, G_string_server_IP, sizeof(G_string_host_name) - 1);
    G_PCB_Version = 10;
    printf("  This host: %s  (MSCC client port %d, ms-sdr port %d)\n",
        G_string_server_IP, G_mscc_port, G_server_port);

    if (status < 0) {
        strncpy(G_Usb_serial_number, "UNKNOWN", sizeof(G_Usb_serial_number) - 1);
        printf("  Continuing without a live radio serial number.\n");
    }

    /* ---- Step 2: hardware family + keyer + base inis ---- */
    step_banner(2, total_steps, "Radio family, keyer & base config");
    /*
     * PROFICIO-MKII=1 → ms-sdr runs MKII rear PTT sense thread.
     * PROFICIO-MKII=0 → legacy (relay T/R); that thread is skipped.
     * Default Y (MKII) matches current fielded installs.
     */
    G_Proficio_Mkii = (uint8_t)prompt_yes_no(
        "  Is a Proficio MKII transceiver attached? (not a legacy Proficio)", 1);
    G_Keyer_Installed = (uint8_t)prompt_yes_no(
        "  Is the Proficio MKII keyer installed?", 0);

    if (Create_Fortis_I2C(mfc, meter) < 0)
        exit_status = 5;
    if (init_mscc() < 0)
        exit_status = 5;
    if (Update_CW_ini() < 0)
        exit_status = 5;

    /* ---- Step 3: CAT + PIN ---- */
    step_banner(3, total_steps, "Kenwood CAT port & PTT pin");
    choose_cat_and_pin();

    /* ---- Step 4: operator audio only; digi is fixed ---- */
    step_banner(4, total_steps, "Operator audio (digi is fixed VirtualA/B)");
    err = Pa_Initialize();
    if (err != paNoError)
        Audio_Device_Error(err);
    if (Pa_GetVersionInfo())
        printf("  PortAudio: %s\n", Pa_GetVersionInfo()->versionText);

    if (Write_Fixed_Digital_Audio_Config() < 0)
        exit_status = 5;

    printf("\n  Pick operator devices only (headphones / mic for you).\n");
    printf("  Digi path is always %s / %s (mscc-virtual-audio).\n",
        MSCC_DIGI_SPEAKER, MSCC_DIGI_MIC);

    Get_Speaker_Audio_Device();
    if (num_output_devices_found == 0) {
        printf("  WARNING: no operator output devices — skipping operator-speaker.ini.\n");
    } else {
        printf("\n  Operator speaker = headphones / speaker (not Multus I/Q).\n");
        speaker_device = prompt_int(
            "  Operator speaker number [0]: ",
            0, num_output_devices_found - 1, 0);
        if (Update_Operator_Speaker_Config(speaker_device) < 0)
            exit_status = 5;
    }

    Get_Microphone_Audio_Device();
    if (num_input_devices_found == 0) {
        printf("  WARNING: no operator input devices — skipping operator-microphone.ini.\n");
    } else {
        printf("\n  Operator mic = your microphone (not Multus I/Q).\n");
        microphone_device = prompt_int(
            "  Operator microphone number [0]: ",
            0, num_input_devices_found - 1, 0);
        if (Update_Operator_Microphone_Config(microphone_device) < 0)
            exit_status = 5;
    }

    Pa_Terminate();
    print_final_summary();
    return exit_status;
}
