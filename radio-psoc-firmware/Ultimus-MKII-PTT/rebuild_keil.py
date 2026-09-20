"""Incremental Keil C51 build for Ultimus-MKII-PTT (Proficio-MKII-PTT.cydsn seed)."""
from __future__ import print_function
import os
import subprocess
import sys

CREATOR = r"C:\Program Files (x86)\Cypress\PSoC Creator\4.4\PSoC Creator"
KEIL = os.path.join(CREATOR, r"import\keil\pk51\9.51\C51\BIN")
GNU = os.path.join(CREATOR, r"import\gnu\c8051\1.0\bin")
CBIN = os.path.join(CREATOR, "bin")

PROJ = "Proficio-MKII-PTT"
APP_C = [
    "main.c", "audio.c", "tx.c", "morse.c", "t1.c", "sync.c", "convert.c",
    "si5351a.c", "si5351.c", "settings01.c", "band.c", "usbvend01.c",
    "pcm3060.c", "temperature.c", "cw.c",
]


def run(cmd, cwd):
    print("  " + " ".join(cmd))
    r = subprocess.run(cmd, cwd=cwd)
    if r.returncode >= 2:
        raise SystemExit("FAILED: %s (exit %s)" % (cmd[0], r.returncode))
    return r.returncode


def compile_c(src, obj_base, cwd, dbg, extra=None):
    lst = os.path.join(dbg, obj_base + ".lst")
    obj = os.path.join(dbg, obj_base + ".obj")
    cmd = [
        os.path.join(KEIL, "C51.exe"), src,
        "NOIV", "LARGE", "MODDP2", "OMF2", "VB(1)", "NOIP",
        "INCDIR(., Generated_Source\\PSoC3)",
        "FF(3)", "DB", "WL(2)",
        "PR(%s)" % lst, "CD", "OT(5,SPEED)", "OA",
    ]
    if extra:
        cmd.extend(extra)
    cmd.append("OJ(%s)" % obj)
    run(cmd, cwd)
    if not os.path.isfile(os.path.join(cwd, obj)):
        raise SystemExit("FAILED: no object %s (Keil license?)" % obj)


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    cydsn = os.path.join(here, PROJ + ".cydsn")
    dbg = os.path.join("DP8051", "DP8051_Keil_951", "Debug")
    dbg_abs = os.path.join(cydsn, dbg)
    env_path = KEIL + os.pathsep + GNU + os.pathsep + CBIN + os.pathsep + os.environ.get("PATH", "")
    os.environ["PATH"] = env_path

    lib = os.path.join(dbg_abs, PROJ + ".lib")
    if not os.path.isfile(lib):
        raise SystemExit("FAILED: %s missing" % lib)

    print("=== C51 application sources ===")
    for src in APP_C:
        print("  compiling", src)
        compile_c(src, os.path.splitext(src)[0], cydsn, dbg)

    print("=== C51 USBFS_descr.c (Ultimus product string) ===")
    compile_c(
        os.path.join("Generated_Source", "PSoC3", "USBFS_descr.c"),
        "USBFS_descr",
        cydsn,
        dbg,
    )
    descr_obj = os.path.join(dbg, "USBFS_descr.obj")
    print("=== LIBX51 replace USBFS_descr in %s.lib ===" % PROJ)
    run([
        os.path.join(KEIL, "LIBX51.exe"),
        "REPLACE", descr_obj, "IN", os.path.join(dbg, PROJ + ".lib"),
    ], cydsn)

    print("=== C51 cyfitter_cfg.c ===")
    compile_c(
        os.path.join("Generated_Source", "PSoC3", "cyfitter_cfg.c"),
        "cyfitter_cfg",
        cydsn,
        dbg,
        extra=["DF(CYAPP_ECC_OFFSET=2080)"],
    )

    print("=== LX51 ===")
    run([os.path.join(KEIL, "LX51.exe"), "@" + os.path.join(dbg, "keilLinkCmds.lnp")], cydsn)

    print("=== OHx51 / elf / entry ===")
    omf = PROJ + ".omf"
    elf = PROJ + ".elf"
    ihx = PROJ + ".ihx"
    run([os.path.join(KEIL, "OHx51.exe"), omf, "HEXFILE(%s)" % ihx], dbg_abs)
    run([os.path.join(GNU, "c8051-elf-omf2elf.exe"), omf, elf], dbg_abs)
    run([
        os.path.join(CBIN, "cysymboladdressfinder.exe"),
        "-f", os.path.join(dbg, elf),
        "-s", "STARTUP1",
        "-p", os.path.join(dbg, "entryaddress.txt"),
        "-l", "FF0000",
    ], cydsn)

    print("=== CyHexTool cyacd + hex ===")
    out = dbg_abs
    bl = os.path.join(
        os.path.dirname(here), "Proficio-bootloader", "bootloader.cydsn",
        "DP8051", "DP8051_Keil_951", "Release", "bootloader.hex",
    )
    cyhex = os.path.join(CBIN, "cyhextool.exe")
    cyacd = os.path.join(out, PROJ + ".cyacd")
    hexfile = os.path.join(out, PROJ + ".hex")
    ihx_abs = os.path.join(out, ihx)
    run([
        cyhex,
        "-o", cyacd,
        "-f", ihx_abs,
        "-flsLine", "256", "-arraySize", "65536",
        "-id", "1E093069", "-rev", "3",
        "-ecc", os.path.join("Generated_Source", "PSoC3", "config.hex"),
        "-metaRow", "0",
        "-blver", "000000000000000000000000000000000000",
        "-chksumEcc", "-blChkType", "1", "-endian", "b",
        "-a", "EEPROM=90200000:800,PROGRAM=00000000:10000,CONFIG=80000000:2000,PROTECT=90400000:40",
        "-acd", "-acdStart", "4100",
        "-e", os.path.join(dbg, "entryaddress.txt"),
    ], cydsn)
    run([
        cyhex,
        "-o", hexfile,
        "-flsLine", "256", "-arraySize", "65536",
        "-bl", bl,
        "-blver", "000000000000000000000000000000000000",
        "-chksumEcc", "-blChkType", "1", "-endian", "b",
        "-prot", os.path.join("Generated_Source", "PSoC3", "protect.hex"),
        "-id", "1E093069",
        "-a", "EEPROM=90200000:800,PROGRAM=00000000:10000,CONFIG=80000000:2000,PROTECT=90400000:40",
        "-meta", "0301", "-cunv", "0800C005", "-wonv", "BC90ACAF",
        "-ecc", os.path.join("Generated_Source", "PSoC3", "config.hex"),
        "-ld", ihx_abs + "=4100:BEBF",
        "-acdStart", "4100",
        "-e", os.path.join(dbg, "entryaddress.txt"),
    ], cydsn)

    print("BUILD OK")
    print("  ", cyacd)
    print("  ", hexfile)


if __name__ == "__main__":
    main()
