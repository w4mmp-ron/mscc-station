r"""
Copy a PSoC Creator build into the tree's own Release\ folder, named by firmware version.

    py -3 copy-release.py <TreeFolder> <ProjectName> [<ReleaseName>]

    <TreeFolder>   folder under radio-psoc-firmware, e.g. Proficio-Ultimus-MKII-ATU
    <ProjectName>  Creator project in it, e.g. Proficio-MKII-ATU (Geminus trees: Geminus)
    <ReleaseName>  name for the release files, default = <ProjectName> (Geminus-MKII: Geminus-MKII)

Reads FIRMWARE_VERSION_MAJOR / _MINOR from <Tree>\<Project>.cydsn\basic-plus.h (the non-Ultimus
major when there is an #ifdef ULTIMUS switch) and copies
    <Tree>\<Project>.cydsn\DP8051\DP8051_Keil_951\Debug\<Project>.cyacd / .hex
to
    <Tree>\Release\<ReleaseName>-<major>.<minor>.cyacd / .hex
e.g. Proficio-Ultimus-MKII-ATU\Release\Proficio-MKII-ATU-4.244.cyacd

A release is never overwritten with different contents: if <name>-4.244.cyacd already exists and
differs, the new build is saved as <name>-4.244-b2 (b3, ...) and you are told to bump
FIRMWARE_VERSION_MINOR. Rebuilding the same code again just confirms the existing file.

Called by each tree's copy-release.bat (PSoC Creator post-build). 2026-11-02, Stew / Claude.
"""
from __future__ import print_function
import filecmp
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DBG = os.path.join("DP8051", "DP8051_Keil_951", "Debug")


def fail(msg):
    print("copy-release FAILED: " + msg)
    sys.exit(1)


def firmware_version(header_path, ultimus=False):
    """(major, minor) from basic-plus.h. ultimus=True picks the #ifdef ULTIMUS major."""
    t = open(header_path).read()
    t = re.sub(r"//[^\n]*", "", t)
    sw = re.search(r"#ifdef\s+ULTIMUS(.*?)#else(.*?)#endif", t, re.S)
    if sw:
        block = sw.group(1) if ultimus else sw.group(2)
        m = re.search(r"#define\s+FIRMWARE_VERSION_MAJOR\s+(\d+)", block)
    else:
        if ultimus:
            fail("%s has no #ifdef ULTIMUS switch" % header_path)
        m = re.search(r"#define\s+FIRMWARE_VERSION_MAJOR\s+(\d+)", t)
    n = re.search(r"#define\s+FIRMWARE_VERSION_MINOR\s+(\d+)", t)
    if not m or not n:
        fail("no FIRMWARE_VERSION_MAJOR / _MINOR in %s" % header_path)
    return int(m.group(1)), int(n.group(1))


def place(files, dest_dir, base):
    """files: {ext: source path}. Copies to dest_dir/base.ext without overwriting a different
    release. Returns the base name actually used."""
    if not os.path.isdir(dest_dir):
        os.makedirs(dest_dir)
    name = base
    n = 1
    while True:
        clash = False
        for ext, src in files.items():
            d = os.path.join(dest_dir, name + ext)
            if os.path.exists(d) and not filecmp.cmp(src, d, shallow=False):
                clash = True
        if not clash:
            break
        n += 1
        name = "%s-b%d" % (base, n)
    for ext, src in files.items():
        shutil.copyfile(src, os.path.join(dest_dir, name + ext))
    if name != base:
        print("WARNING: %s already exists with different contents. Saved this build as %s."
              % (base, name))
        print("WARNING: bump FIRMWARE_VERSION_MINOR in basic-plus.h before the next release.")
    return name


def main():
    if len(sys.argv) < 3:
        fail("usage: copy-release.py <TreeFolder> <ProjectName> [<ReleaseName>]")
    tree = sys.argv[1]
    proj = sys.argv[2]
    rname = sys.argv[3] if len(sys.argv) > 3 else proj
    root = os.path.join(HERE, tree)
    cydsn = os.path.join(root, proj + ".cydsn")
    major, minor = firmware_version(os.path.join(cydsn, "basic-plus.h"))
    files = {}
    for ext in (".cyacd", ".hex"):
        f = os.path.join(cydsn, DBG, proj + ext)
        if not os.path.isfile(f):
            fail("missing %s: build the project in PSoC Creator first" % f)
        files[ext] = f
    base = "%s-%d.%d" % (rname, major, minor)
    name = place(files, os.path.join(root, "Release"), base)
    print("OK: %s\\Release\\%s.cyacd / .hex" % (tree, name))


if __name__ == "__main__":
    main()
