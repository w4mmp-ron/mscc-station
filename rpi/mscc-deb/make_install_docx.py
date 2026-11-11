"""Generate INSTALL-FOR-PI.docx from INSTALL-FOR-PI.md (2026-11-10: was hand-written text that
went stale; now the .md is the only source). Then make the PDF with LibreOffice:

    python make_install_docx.py
    "C:\\Program Files\\LibreOffice\\program\\soffice.exe" --headless --convert-to pdf INSTALL-FOR-PI.docx

Handles the Markdown used in that file: # headings, paragraphs, - / 1. lists, | tables |,
``` code blocks, > quotes, --- rules, **bold**, `code`, [text](link) and <link>.
"""
import os
import re
from docx import Document
from docx.shared import Pt, Inches, RGBColor
from docx.oxml.ns import qn
from docx.oxml import OxmlElement

HERE = os.path.dirname(os.path.abspath(__file__))
SRC = os.path.join(HERE, "INSTALL-FOR-PI.md")
OUT = os.path.join(HERE, "INSTALL-FOR-PI.docx")

doc = Document()
section = doc.sections[0]
section.page_width = Inches(8.5)
section.page_height = Inches(11)
section.top_margin = Inches(0.7)
section.bottom_margin = Inches(0.7)
section.left_margin = Inches(0.85)
section.right_margin = Inches(0.85)

style = doc.styles["Normal"]
style.font.name = "Calibri"
style.font.size = Pt(11)
style.paragraph_format.space_after = Pt(6)
style.paragraph_format.space_before = Pt(0)
for i in range(1, 4):
    hs = doc.styles[f"Heading {i}"]
    hs.font.name = "Calibri"
    hs.font.color.rgb = RGBColor(0x1F, 0x4E, 0x79)
    hs.font.bold = True
    hs.font.size = Pt({1: 18, 2: 14, 3: 12}[i])

INLINE = re.compile(r"(\*\*[^*]+\*\*|`[^`]+`|\[[^\]]+\]\([^)]+\)|<https?://[^>]+>)")


def add_inline(par, text, size=None, bold=False):
    def run(t, code=False):
        r = par.add_run(t)
        if code:
            r.font.name = "Consolas"
            r.font.size = Pt(size - 1 if size else 10)
        elif size:
            r.font.size = Pt(size)
        if bold:
            r.bold = True
        return r

    for part in INLINE.split(text):
        if not part:
            continue
        if part.startswith("**") and part.endswith("**"):
            add_inline(par, part[2:-2], size, True)
        elif part.startswith("`") and part.endswith("`"):
            run(part[1:-1], code=True)
        elif part.startswith("[") and "](" in part:
            label, link = part[1:].split("](", 1)
            link = link[:-1]
            # the label is often a `path`
            if label.startswith("`") and label.endswith("`"):
                run(label[1:-1], code=True)
            else:
                run(label)
            if link.startswith("http"):
                run(f" ({link})")
        elif part.startswith("<http"):
            run(part[1:-1])
        else:
            run(part)


def add_inline_bold(par, text, size):
    add_inline(par, text, size, True)


def shade(cell, hex_fill):
    tcPr = cell._tc.get_or_add_tcPr()
    shd = OxmlElement("w:shd")
    shd.set(qn("w:val"), "clear")
    shd.set(qn("w:color"), "auto")
    shd.set(qn("w:fill"), hex_fill)
    tcPr.append(shd)


def add_code(lines):
    p = doc.add_paragraph()
    p.paragraph_format.left_indent = Inches(0.2)
    p.paragraph_format.space_after = Pt(8)
    pPr = p._p.get_or_add_pPr()
    shd = OxmlElement("w:shd")
    shd.set(qn("w:val"), "clear")
    shd.set(qn("w:color"), "auto")
    shd.set(qn("w:fill"), "F2F2F2")
    pPr.append(shd)
    for i, line in enumerate(lines):
        r = p.add_run(line)
        r.font.name = "Consolas"
        r.font.size = Pt(9.5)
        if i < len(lines) - 1:
            r.add_break()


def add_table(rows):
    cells = [[c.strip() for c in r.strip().strip("|").split("|")] for r in rows]
    cells = [r for r in cells if not all(re.fullmatch(r":?-{2,}:?", c) for c in r if c)]
    ncol = max(len(r) for r in cells)
    t = doc.add_table(rows=len(cells), cols=ncol)
    t.style = "Table Grid"
    for i, r in enumerate(cells):
        for j in range(ncol):
            cell = t.cell(i, j)
            cell.text = ""
            par = cell.paragraphs[0]
            text = r[j] if j < len(r) else ""
            if i == 0:
                add_inline_bold(par, text.replace("**", ""), 10)
                shade(cell, "D9E2F3")
            else:
                add_inline(par, text, 10)
    doc.add_paragraph()


lines = open(SRC, encoding="utf-8").read().replace("\r\n", "\n").split("\n")
i = 0
para = []


def flush_para():
    global para
    if para:
        text = " ".join(s.strip() for s in para)
        add_inline(doc.add_paragraph(), text)
        para = []


while i < len(lines):
    line = lines[i]
    s = line.strip()
    if s.startswith("```"):
        flush_para()
        i += 1
        code = []
        while i < len(lines) and not lines[i].strip().startswith("```"):
            code.append(lines[i])
            i += 1
        add_code(code)
    elif s.startswith("|"):
        flush_para()
        rows = []
        while i < len(lines) and lines[i].strip().startswith("|"):
            rows.append(lines[i])
            i += 1
        add_table(rows)
        continue
    elif m := re.match(r"(#{1,3})\s+(.*)", s):
        flush_para()
        h = doc.add_heading(level=len(m.group(1)))
        add_inline_bold(h, m.group(2).replace("**", ""), None)
    elif s == "---":
        flush_para()
    elif s.startswith(">"):
        flush_para()
        quote = []
        while i < len(lines) and lines[i].strip().startswith(">"):
            quote.append(lines[i].strip()[1:].strip())
            i += 1
        p = doc.add_paragraph()
        p.paragraph_format.left_indent = Inches(0.3)
        add_inline(p, " ".join(quote))
        continue
    elif re.match(r"[-*]\s+", s):
        flush_para()
        text = re.sub(r"^[-*]\s+", "", s)
        while i + 1 < len(lines) and lines[i + 1].startswith("  ") and lines[i + 1].strip() \
                and not re.match(r"\s*([-*]|\d+\.)\s+", lines[i + 1]):
            i += 1
            text += " " + lines[i].strip()
        add_inline(doc.add_paragraph(style="List Bullet"), text)
    elif re.match(r"\d+\.\s+", s):
        flush_para()
        text = re.sub(r"^\d+\.\s+", "", s)
        while i + 1 < len(lines) and lines[i + 1].startswith("   ") and lines[i + 1].strip() \
                and not re.match(r"\s*([-*]|\d+\.)\s+", lines[i + 1]):
            i += 1
            text += " " + lines[i].strip()
        add_inline(doc.add_paragraph(style="List Number"), text)
    elif not s:
        flush_para()
    else:
        para.append(line)
    i += 1
flush_para()

doc.save(OUT)
print("wrote", OUT)
