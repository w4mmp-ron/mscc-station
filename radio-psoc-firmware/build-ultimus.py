r"""
Build Ultimus firmware from the Proficio source trees (no separate Ultimus sources).

Ultimus radios are hand-buildable (through-hole) Proficios. The firmware is the same code;
only the firmware major (radio identity for MSCC / factory cal) and the USB product string
differ. So each Ultimus image is built from the matching Proficio tree:

    legacy    Proficio-Legacy    major 1  ->  Ultimus-Legacy    major 6
    mkii-ptt  Proficio-MKII-PTT  major 3  ->  Ultimus-MKII-PTT  major 8
    mkii-atu  Proficio-MKII-ATU  major 4  ->  Ultimus-MKII-ATU  major 7

How it works
  1. Build the Proficio project in PSoC Creator as usual (Shift+F6). That makes the
     Generated_Source, the component library and the Debug objects this script reuses.
  2. Run:  py -3 build-ultimus.py legacy      (or mkii-ptt, mkii-atu, all)
           or double-click build-ultimus.bat
  3. The script copies the .cydsn to _ultimus-build\<name>\ (git-ignored; the Proficio
     tree is not touched), recompiles the application sources with DF(ULTIMUS) (basic-plus.h
     picks the Ultimus major), sets the USB product string to "Ultimus", links and makes the
     .cyacd / .hex with the same Keil / Cypress tools and options Creator uses, then copies
     them to the Proficio tree's Release\ folder as <name>-<major>.<minor>.cyacd/.hex, e.g.
     Proficio-MKII-ATU\Release\Ultimus-MKII-ATU-7.244.cyacd (never overwriting a different
     build of the same version: see copy-release.py).

Requires: PSoC Creator 4.4 (Keil C51 licence) on this PC, Python 3.
The Proficio basic-plus.h must contain the switch:
    #ifdef ULTIMUS
    #define FIRMWARE_VERSION_MAJOR <ultimus major>
    #else
    #define FIRMWARE_VERSION_MAJOR <proficio major>
    #endif
2026-10-31, Stew / Claude.
"""
from __future__ import print_function
import importlib.util
import os
import re
import shutil
import subprocess
import sys

CREATOR = r"C:\Program Files (x86)\Cypress\PSoC Creator\4.4\PSoC Creator"
KEIL = os.path.join(CREATOR, r"import\keil\pk51\9.51\C51\BIN")
GNU = os.path.join(CREATOR, r"import\gnu\c8051\1.0\bin")
CBIN = os.path.join(CREATOR, "bin")

HERE = os.path.dirname(os.path.abspath(__file__))
DBG = os.path.join("DP8051", "DP8051_Keil_951", "Debug")
BOOTLOADER_HEX = os.path.join(HERE, "Proficio-bootloader", "bootloader.cydsn",
                              "DP8051", "DP8051_Keil_951", "Release", "bootloader.hex")

VARIANTS = {
    "legacy":   ("Proficio-Legacy",   "Proficio-Legacy",   "Ultimus-Legacy",   6),
    "mkii-ptt": ("Proficio-MKII-PTT", "Proficio-MKII-PTT", "Ultimus-MKII-PTT", 8),
    "mkii-atu": ("Proficio-MKII-ATU", "Proficio-MKII-ATU", "Ultimus-MKII-ATU", 7),
}
PRODUCT = "Ultimus"

_spec = importlib.util.spec_from_file_location("copy_release", os.path.join(HERE, "copy-release.py"))
copy_release = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(copy_release)


def fail(msg):
    raise SystemExit("FAILED: " + msg)


def run(cmd, cwd):
    print("  " + " ".join(cmd))
    r = subprocess.run(cmd, cwd=cwd)
    if r.returncode >= 2:   # Keil: 1 = warnings, 2+ = errors
        fail("%s (exit %s)" % (os.path.basename(cmd[0]), r.returncode))


def c51(src, obj_base, cwd, extra=()):
    lst = os.path.join(DBG, obj_base + ".lst")
    obj = os.path.join(DBG, obj_base + ".obj")
    if os.path.exists(os.path.join(cwd, obj)):
        os.remove(os.path.join(cwd, obj))
    cmd = [os.path.join(KEIL, "C51.exe"), src,
           "NOIV", "LARGE", "MODDP2", "OMF2", "VB(1)", "NOIP",
           "INCDIR(., Generated_Source\\PSoC3)", "FF(3)", "DB", "WL(2)",
           "PR(%s)" % lst, "CD", "OT(5,SPEED)", "OA"] + list(extra) + ["OJ(%s)" % obj]
    run(cmd, cwd)
    if not os.path.isfile(os.path.join(cwd, obj)):
        fail("no object for %s (Keil licence?)" % src)


def app_sources(cydsn, proj):
    """Application .c files from Creator's link command file (objects next to the .cydsn)."""
    lnp = open(os.path.join(cydsn, DBG, "keilLinkCmds.lnp")).read()
    srcs = []
    for obj in re.findall(r"Debug\\([\w\-]+)\.obj", lnp):
        if os.path.isfile(os.path.join(cydsn, obj + ".c")):
            srcs.append(obj + ".c")
    if "main.c" not in srcs:
        fail("could not read the application sources from keilLinkCmds.lnp")
    return srcs


def set_product_string(path):
    """USB iProduct string descriptor "Proficio" -> "Ultimus" in generated USBFS_descr.c."""
    t = open(path).read()
    pat = re.compile(
        r'(\* String Descriptor: ")Proficio("\s*\n\*+/\s*\n'
        r'/\* Descriptor Length\s*\*/ )0x[0-9A-Fa-f]+u,(\s*\n/\* DescriptorType: STRING\s*\*/ 0x03u,\s*\n)'
        r'((?:\s*\(uint8\)\'.\', 0u,)+)')
    m = list(pat.finditer(t))
    if len(m) != 1:
        fail("USB product string \"Proficio\" not found exactly once in USBFS_descr.c")
    chars = "".join("(uint8)'%s', 0u," % c for c in PRODUCT)
    rep = (m[0].group(1) + PRODUCT + m[0].group(2) + "0x%02Xu," % (2 + 2 * len(PRODUCT))
           + m[0].group(3) + " " + chars)
    t = t[:m[0].start()] + rep + t[m[0].end():]
    open(path, "w").write(t)


def build(key):
    tree, proj, name, major = VARIANTS[key]
    src_cydsn = os.path.join(HERE, tree, proj + ".cydsn")
    print("=== %s from %s (major %d) ===" % (name, tree, major))

    hdr = open(os.path.join(src_cydsn, "basic-plus.h")).read()
    if not re.search(r"#ifdef\s+ULTIMUS\s+#define\s+FIRMWARE_VERSION_MAJOR\s+%d\b" % major, hdr):
        fail("%s basic-plus.h has no '#ifdef ULTIMUS / FIRMWARE_VERSION_MAJOR %d' switch yet"
             % (tree, major))

    cyacd = os.path.join(src_cydsn, DBG, proj + ".cyacd")
    if not os.path.isfile(cyacd):
        fail("%s not built yet: build it in PSoC Creator first" % tree)
    built = os.path.getmtime(cyacd)
    for f in os.listdir(src_cydsn):
        if f.endswith((".c", ".h", ".cysch", ".cydwr")) or f == proj + ".cyprj":
            if os.path.getmtime(os.path.join(src_cydsn, f)) > built + 2:
                fail("%s changed after the last Creator build: build %s in Creator first" % (f, tree))
    for f in os.listdir(os.path.join(src_cydsn, "TopDesign")):
        if os.path.getmtime(os.path.join(src_cydsn, "TopDesign", f)) > built + 2:
            fail("TopDesign changed after the last Creator build: build %s in Creator first" % tree)

    work_root = os.path.join(HERE, "_ultimus-build", name)
    work = os.path.join(work_root, proj + ".cydsn")
    if os.path.isdir(work_root):
        shutil.rmtree(work_root)
    shutil.copytree(src_cydsn, work, ignore=shutil.ignore_patterns("codegentemp"))

    set_product_string(os.path.join(work, "Generated_Source", "PSoC3", "USBFS_descr.c"))

    print("--- C51 application sources with DF(ULTIMUS)")
    for src in app_sources(work, proj):
        c51(src, os.path.splitext(src)[0], work, ["DF(ULTIMUS)"])

    print("--- C51 USBFS_descr.c (product string \"%s\") into %s.lib" % (PRODUCT, proj))
    c51(os.path.join("Generated_Source", "PSoC3", "USBFS_descr.c"), "USBFS_descr", work)
    run([os.path.join(KEIL, "LIBX51.exe"), "REPLACE", os.path.join(DBG, "USBFS_descr.obj"),
         "IN", os.path.join(DBG, proj + ".lib")], work)

    print("--- LX51 / OHx51 / elf / entry address")
    run([os.path.join(KEIL, "LX51.exe"), "@" + os.path.join(DBG, "keilLinkCmds.lnp")], work)
    dbg_abs = os.path.join(work, DBG)
    run([os.path.join(KEIL, "OHx51.exe"), proj + ".omf", "HEXFILE(%s.ihx)" % proj], dbg_abs)
    run([os.path.join(GNU, "c8051-elf-omf2elf.exe"), proj + ".omf", proj + ".elf"], dbg_abs)
    entry = os.path.join(DBG, "entryaddress.txt")
    run([os.path.join(CBIN, "cysymboladdressfinder.exe"), "-f", os.path.join(DBG, proj + ".elf"),
         "-s", "STARTUP1", "-p", entry, "-l", "FF0000"], work)

    print("--- CyHexTool .cyacd / .hex (same options as Creator)")
    ihx = os.path.join(dbg_abs, proj + ".ihx")
    out_cyacd = os.path.join(dbg_abs, proj + ".cyacd")
    out_hex = os.path.join(dbg_abs, proj + ".hex")
    for f in (out_cyacd, out_hex):
        if os.path.exists(f):
            os.remove(f)
    common = ["-flsLine", "256", "-arraySize", "65536", "-id", "1E093069",
              "-a", "EEPROM=90200000:800,PROGRAM=00000000:10000,CONFIG=80000000:2000,PROTECT=90400000:40"]
    ecc = os.path.join("Generated_Source", "PSoC3", "config.hex")
    cyhex = os.path.join(CBIN, "cyhextool.exe")
    run([cyhex, "-o", out_cyacd, "-f", ihx] + common +
        ["-rev", "3", "-ecc", ecc, "-metaRow", "0", "-blver", "0" * 36, "-chksumEcc",
         "-blChkType", "1", "-endian", "b", "-acd", "-acdStart", "4100", "-e", entry], work)
    run([cyhex, "-o", out_hex] + common +
        ["-bl", BOOTLOADER_HEX, "-blver", "0" * 36, "-chksumEcc", "-blChkType", "1", "-endian", "b",
         "-prot", os.path.join("Generated_Source", "PSoC3", "protect.hex"),
         "-meta", "0301", "-cunv", "0800C005", "-wonv", "BC90ACAF", "-ecc", ecc,
         "-ld", ihx + "=4100:BEBF", "-acdStart", "4100", "-e", entry], work)
    for f in (out_cyacd, out_hex):
        if not os.path.isfile(f):
            fail("CyHexTool made no %s" % f)

    _, minor = copy_release.firmware_version(os.path.join(src_cydsn, "basic-plus.h"), ultimus=True)
    rel = os.path.join(HERE, tree, "Release")
    used = copy_release.place({".cyacd": out_cyacd, ".hex": out_hex}, rel,
                              "%s-%d.%d" % (name, major, minor))
    print("OK: %s -> %s\\Release\\%s.cyacd / .hex" % (name, tree, used))


def main():
    args = [a.lower() for a in sys.argv[1:]] or ["all"]
    keys = list(VARIANTS) if args == ["all"] else args
    for k in keys:
        if k not in VARIANTS:
            fail("unknown radio '%s' (use: %s, all)" % (k, ", ".join(VARIANTS)))
    os.environ["PATH"] = os.pathsep.join([KEIL, GNU, CBIN, os.environ.get("PATH", "")])
    done = []
    for k in keys:
        tree, proj, name, major = VARIANTS[k]
        if args == ["all"] and "#ifdef ULTIMUS" not in open(
                os.path.join(HERE, tree, proj + ".cydsn", "basic-plus.h")).read():
            print("--- skipping %s: %s has no ULTIMUS switch yet" % (name, tree))
            continue
        build(k)
        done.append(name)
    print("BUILD OK: " + (", ".join(done) if done else "nothing to build"))


if __name__ == "__main__":
    main()
