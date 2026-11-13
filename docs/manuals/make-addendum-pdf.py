"""
make-addendum-pdf.py: MSCC-Software-Versions-Addendum.md -> .pdf (same folder).

    python make-addendum-pdf.py
    python make-addendum-pdf.py MSCC-Updating-Firmware.md     (any other .md in this folder)

Run after every edit of the addendum (every release). Needs Python 3 with the markdown module
(python -m pip install --user markdown) and Microsoft Edge (prints the page to PDF).
"""
import os
import subprocess
import sys
import tempfile

import markdown

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "MSCC-Software-Versions-Addendum.md")
OUT = os.path.join(HERE, "MSCC-Software-Versions-Addendum.pdf")
EDGE = [r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
        r"C:\Program Files\Microsoft\Edge\Application\msedge.exe"]

CSS = """
@page { size: Letter; margin: 0.8in 0.8in 0.8in 0.8in; }
body { font-family: Calibri, Arial, sans-serif; font-size: 11pt; color: #000; line-height: 1.35; }
h1 { font-size: 20pt; margin: 0 0 4pt 0; }
h2 { font-size: 14pt; margin: 16pt 0 6pt 0; border-bottom: 1px solid #888; padding-bottom: 2pt; }
table { border-collapse: collapse; width: 100%; margin: 6pt 0 8pt 0; page-break-inside: avoid; }
th, td { border: 1px solid #999; padding: 3pt 6pt; text-align: left; vertical-align: top; }
th { background: #e8e8e8; }
code { font-family: Consolas, monospace; font-size: 10pt; }
a { color: #000; }
h2, h2 + p { break-after: avoid; page-break-after: avoid; }
"""


def main():
    global SRC, OUT
    if len(sys.argv) > 1:
        SRC = os.path.join(HERE, sys.argv[1])
        OUT = os.path.splitext(SRC)[0] + ".pdf"
    with open(SRC, encoding="utf-8") as f:
        body = markdown.markdown(f.read(), extensions=["tables"])
    html = ("<!doctype html><html><head><meta charset='utf-8'><title>%s</title><style>%s</style></head><body>%s</body></html>" % (os.path.splitext(os.path.basename(SRC))[0], CSS, body))
    edge = next((e for e in EDGE if os.path.exists(e)), None)
    if not edge:
        sys.exit("Microsoft Edge not found")
    fd, tmp = tempfile.mkstemp(suffix=".html")
    with os.fdopen(fd, "w", encoding="utf-8") as f:
        f.write(html)
    try:
        subprocess.run([edge, "--headless", "--disable-gpu", "--no-pdf-header-footer",
                        "--print-to-pdf=" + OUT, "file:///" + tmp.replace("\\", "/")],
                       check=True, timeout=120)
    finally:
        os.remove(tmp)
    print("wrote", OUT)


if __name__ == "__main__":
    main()
