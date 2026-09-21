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

static void ensure_dir(const char *path)
{
    CreateDirectoryA(path, NULL);
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

static void last_line_path(char *out, size_t n, const char *live)
{
    snprintf(out, n, "%s\\cal\\LAST_LINE.txt", live);
}

static void read_last_line(const char *live, char *out, size_t n)
{
    char path[MAX_PATH];
    FILE *fp;
    out[0] = 0;
    last_line_path(path, sizeof path, live);
    fp = fopen(path, "r");
    if (!fp)
        return;
    if (fgets(out, (int)n, fp)) {
        size_t L = strlen(out);
        while (L > 0 && (out[L - 1] == '\n' || out[L - 1] == '\r' || out[L - 1] == ' '))
            out[--L] = 0;
    }
    fclose(fp);
}

static void write_last_line(const char *live, const char *line)
{
    char caldir[MAX_PATH];
    char path[MAX_PATH];
    FILE *fp;
    snprintf(caldir, sizeof caldir, "%s\\cal", live);
    ensure_dir(caldir);
    last_line_path(path, sizeof path, live);
    fp = fopen(path, "w");
    if (!fp)
        return;
    fprintf(fp, "%s\n", line);
    fclose(fp);
}

static void ensure_cal_line_dir(const char *live, const char *line)
{
    char p[MAX_PATH];
    snprintf(p, sizeof p, "%s\\cal", live);
    ensure_dir(p);
    snprintf(p, sizeof p, "%s\\cal\\%s", live, line);
    ensure_dir(p);
}

static void stash_active_to_cal(const char *live, const char *prev)
{
    char src[MAX_PATH], dst[MAX_PATH];
    if (!prev || !prev[0])
        return;
    ensure_cal_line_dir(live, prev);
    snprintf(src, sizeof src, "%s\\iq.ini", live);
    snprintf(dst, sizeof dst, "%s\\cal\\%s\\iq.ini", live, prev);
    if (file_exists(src) && copy_file(src, dst)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] cal stash line=%s iq.ini\n", line_number++, prev);
    }
    snprintf(src, sizeof src, "%s\\power_cal.ini", live);
    snprintf(dst, sizeof dst, "%s\\cal\\%s\\power_cal.ini", live, prev);
    if (file_exists(src) && copy_file(src, dst)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] cal stash line=%s power_cal.ini\n", line_number++, prev);
    }
}

static void load_iq_or_power(const char *live, const char *line, const char *kind, const char *leaf,
                             const char *factory_root, int bootstrap_live)
{
    char calp[MAX_PATH], livep[MAX_PATH], facp[MAX_PATH];
    snprintf(livep, sizeof livep, "%s\\%s", live, leaf);
    snprintf(calp, sizeof calp, "%s\\cal\\%s\\%s", live, line, leaf);
    if (factory_root && factory_root[0])
        snprintf(facp, sizeof facp, "%s\\%s\\%s\\%s", factory_root, kind, line, leaf);
    else
        facp[0] = 0;
    ensure_cal_line_dir(live, line);
    if (file_exists(calp)) {
        if (copy_file(calp, livep)) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] cal load line=%s %s\n", line_number++, line, leaf);
        }
        return;
    }
    /* Same radio / first cmd-020 boot: keep existing user live, then cache it. */
    if (bootstrap_live && file_exists(livep)) {
        if (copy_file(livep, calp)) {
            print_time(0);
            fprintf(G_fp_logfile, "[%d] cal bootstrap live→line=%s %s\n", line_number++, line, leaf);
        }
        return;
    }
    if (facp[0] && file_exists(facp) && copy_file(facp, livep)) {
        copy_file(facp, calp);
        print_time(0);
        fprintf(G_fp_logfile, "[%d] cal seed factory→line=%s %s\n", line_number++, line, leaf);
        return;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] cal seed FAILED line=%s %s\n", line_number++, line, leaf);
}

static void seed_freq_if_missing(const char *factory_root, const char *line, const char *live)
{
    char src[MAX_PATH], dst[MAX_PATH];
    snprintf(dst, sizeof dst, "%s\\freq_cal.ini", live);
    if (file_exists(dst)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_seed. keep live freq_cal.ini\n", line_number++);
        return;
    }
    snprintf(src, sizeof src, "%s\\freq\\%s\\freq_cal.ini", factory_root, line);
    if (file_exists(src) && copy_file(src, dst)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_seed. copied %s -> %s (line=%s)\n",
                line_number++, src, dst, line);
    }
}

void Factory_mirror_live_to_cal(const char *leaf)
{
    char *live;
    const char *line;
    char src[MAX_PATH], dst[MAX_PATH];
    if (!leaf)
        return;
    live = My_getenv("HOME");
    if (live == NULL || live[0] == 0)
        return;
    line = Factory_line_from_major(G_major_version);
    ensure_cal_line_dir(live, line);
    snprintf(src, sizeof src, "%s\\%s", live, leaf);
    snprintf(dst, sizeof dst, "%s\\cal\\%s\\%s", live, line, leaf);
    if (!file_exists(src))
        return;
    if (copy_file(src, dst)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] cal write-through line=%s %s\n", line_number++, line, leaf);
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
    if (!copy_file(src, dst)) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_reseed. copy FAILED %s\n", line_number++, dst);
        return 0;
    }
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Factory_reseed. %s -> %s (line=%s major=%d)\n",
            line_number++, src, dst, line, G_major_version);
    /* IQ/QRP Reset also overwrites user cache; freq has no cal\ cache. */
    if (strcmp(leaf, "iq.ini") == 0 || strcmp(leaf, "power_cal.ini") == 0)
        Factory_mirror_live_to_cal(leaf);
    return 1;
}

void Factory_seed_live_inis(void)
{
    char factory_root[MAX_PATH];
    const char *line;
    char *live;
    char prev[128];
    int have_factory;
    int switching;

    line = Factory_line_from_major(G_major_version);
    print_time(0);
    fprintf(G_fp_logfile, "[%d] Factory_seed. major=%d line=%s\n",
            line_number++, G_major_version, line);
    live = My_getenv("HOME");
    if (live == NULL || live[0] == 0) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] Factory_seed. live HOME path missing\n", line_number++);
        return;
    }
    have_factory = find_factory_root(factory_root, sizeof factory_root);
    if (!have_factory) {
        print_time(0);
        fprintf(G_fp_logfile,
            "[%d] Factory_seed. factory tree not next to exe (expected factory\\iq\\proficio-mkii\\iq.ini)\n",
            line_number++);
        factory_root[0] = 0;
    }

    read_last_line(live, prev, sizeof prev);
    switching = (prev[0] && strcmp(prev, line) != 0);
    if (switching) {
        print_time(0);
        fprintf(G_fp_logfile, "[%d] cal stash line=%s (switching to %s)\n",
                line_number++, prev, line);
        stash_active_to_cal(live, prev);
    }

    load_iq_or_power(live, line, "iq", "iq.ini", factory_root, !switching);
    load_iq_or_power(live, line, "power", "power_cal.ini", factory_root, !switching);
    if (have_factory)
        seed_freq_if_missing(factory_root, line, live);
    write_last_line(live, line);
}
