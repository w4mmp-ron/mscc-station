#define _CRT_SECURE_NO_WARNINGS 1
#include "extern.h"
#include "factory_seed.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

const char *Factory_line_from_major(int major)
{
    switch (major) {
        case 1: return "proficio-legacy";
        case 2: return "geminus-mkii";
        case 3:
        case 4: return "proficio-mkii";
        case 5: return "geminus-legacy";
        case 6: return "ultimus-legacy";
        case 7:
        case 8: return "ultimus-mkii";
        default: return "proficio-mkii";
    }
}

static int file_exists(const char *path)
{
    FILE *fp = fopen(path, "r");
    if (fp) {
        fclose(fp);
        return 1;
    }
    return 0;
}

static int copy_file(const char *src, const char *dst)
{
    FILE *in = fopen(src, "rb");
    FILE *out;
    char buf[4096];
    size_t n;
    if (!in)
        return 0;
    out = fopen(dst, "wb");
    if (!out) {
        fclose(in);
        return 0;
    }
    while ((n = fread(buf, 1, sizeof buf, in)) > 0)
        fwrite(buf, 1, n, out);
    fclose(in);
    fclose(out);
    return 1;
}

static int find_factory_root(char *out, size_t n)
{
    char exe[MAX_PATH];
    char *slash;
    char probe[MAX_PATH];
    DWORD len = GetModuleFileNameA(NULL, exe, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
        return 0;
    slash = strrchr(exe, '\\');
    if (!slash)
        slash = strrchr(exe, '/');
    if (slash)
        *slash = 0;
    snprintf(out, n, "%s\\factory", exe);
    snprintf(probe, sizeof probe, "%s\\iq\\proficio-mkii\\iq.ini", out);
    if (file_exists(probe))
        return 1;
    return 0;
}

static void seed_one(const char *factory_root, const char *kind, const char *leaf,
                     const char *line, const char *live_dir)
{
    char src[MAX_PATH];
    char dst[MAX_PATH];
    snprintf(src, sizeof src, "%s\\%s\\%s\\%s", factory_root, kind, line, leaf);
    snprintf(dst, sizeof dst, "%s\\%s", live_dir, leaf);
    if (file_exists(dst)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_seed. keep live %s\n", line_number++, leaf);
        return;
    }
    if (!file_exists(src)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_seed. missing factory %s\n", line_number++, src);
        return;
    }
    if (copy_file(src, dst)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_seed. copied %s -> %s (line=%s)\n",
                line_number++, src, dst, line);
    } else {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_seed. copy FAILED %s -> %s\n",
                line_number++, src, dst);
    }
}

int Factory_reseed_live_file(const char *kind, const char *leaf)
{
    char factory_root[MAX_PATH];
    char src[MAX_PATH];
    char dst[MAX_PATH];
    const char *line;
    char *live;

    if (!kind || !leaf)
        return 0;
    line = Factory_line_from_major(G_major_version);
    if (!find_factory_root(factory_root, sizeof factory_root)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_reseed. factory tree missing (kind=%s)\n",
                line_number++, kind);
        return 0;
    }
    live = My_getenv("HOME");
    if (live == NULL || live[0] == 0)
        return 0;
    snprintf(src, sizeof src, "%s\\%s\\%s\\%s", factory_root, kind, line, leaf);
    snprintf(dst, sizeof dst, "%s\\%s", live, leaf);
    if (!file_exists(src)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_reseed. missing %s\n", line_number++, src);
        return 0;
    }
    if (copy_file(src, dst)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_reseed. %s -> %s (line=%s major=%d)\n",
                line_number++, src, dst, line, G_major_version);
        return 1;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Factory_reseed. copy FAILED %s\n", line_number++, dst);
    return 0;
}

void Factory_seed_live_inis(void)
{
    char factory_root[MAX_PATH];
    const char *line;
    char *live;

    line = Factory_line_from_major(G_major_version);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Factory_seed. major=%d line=%s\n",
            line_number++, G_major_version, line);
    if (!find_factory_root(factory_root, sizeof factory_root)) {
        print_time(0);
        fprintf(G_fp_logfile,
            "[%d] Factory_seed. factory tree not next to exe (expected factory\\iq\\proficio-mkii\\iq.ini)\n",
            line_number++);
        return;
    }
    live = My_getenv("HOME");
    if (live == NULL || live[0] == 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_seed. live HOME path missing\n", line_number++);
        return;
    }
    seed_one(factory_root, "iq", "iq.ini", line, live);
    seed_one(factory_root, "freq", "freq_cal.ini", line, live);
    seed_one(factory_root, "power", "power_cal.ini", line, live);
}
