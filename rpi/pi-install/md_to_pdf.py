"""Convert INSTALL.md to a printable letter PDF."""
from __future__ import annotations

import re
from pathlib import Path

from reportlab.lib import colors
from reportlab.lib.pagesizes import letter
from reportlab.lib.styles import ParagraphStyle, getSampleStyleSheet
from reportlab.lib.units import inch
from reportlab.platypus import (  # noqa: E402
    HRFlowable,
    Paragraph,
    Preformatted,
    SimpleDocTemplate,
    Spacer,
    Table,
    TableStyle,
)

HERE = Path(__file__).resolve().parent
MD_PATH = HERE / "INSTALL.md"
OUT_PATH = HERE / "INSTALL.pdf"


def inline_md(s: str) -> str:
    s = s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")
    s = re.sub(r"\*\*(.+?)\*\*", r"<b>\1</b>", s)
    s = re.sub(
        r"`([^`]+)`",
        r'<font face="Courier" size="8">\1</font>',
        s,
    )
    return s


def build_story(text: str):
    styles = getSampleStyleSheet()
    styles.add(
        ParagraphStyle(
            name="H1Doc",
            parent=styles["Heading1"],
            fontSize=16,
            spaceAfter=10,
            spaceBefore=4,
            textColor=colors.HexColor("#003366"),
        )
    )
    styles.add(
        ParagraphStyle(
            name="H2Doc",
            parent=styles["Heading2"],
            fontSize=13,
            spaceAfter=8,
            spaceBefore=14,
            textColor=colors.HexColor("#004488"),
        )
    )
    styles.add(
        ParagraphStyle(
            name="H3Doc",
            parent=styles["Heading3"],
            fontSize=11,
            spaceAfter=6,
            spaceBefore=10,
            textColor=colors.HexColor("#115599"),
        )
    )
    styles.add(
        ParagraphStyle(
            name="BodyDoc",
            parent=styles["Normal"],
            fontSize=9.5,
            leading=12,
            spaceAfter=6,
        )
    )
    styles.add(
        ParagraphStyle(
            name="CodeDoc",
            parent=styles["Code"],
            fontSize=8,
            leading=10,
            backColor=colors.HexColor("#f4f4f4"),
            leftIndent=6,
            rightIndent=6,
            spaceBefore=4,
            spaceAfter=8,
        )
    )
    styles.add(
        ParagraphStyle(
            name="CellDoc",
            parent=styles["Normal"],
            fontSize=8,
            leading=10,
        )
    )
    styles.add(
        ParagraphStyle(
            name="BulletDoc",
            parent=styles["Normal"],
            fontSize=9.5,
            leading=12,
            leftIndent=14,
            spaceAfter=3,
        )
    )
    styles.add(
        ParagraphStyle(
            name="CheckDoc",
            parent=styles["Normal"],
            fontSize=9.5,
            leading=12,
            leftIndent=14,
            spaceAfter=3,
        )
    )

    story = []
    lines = text.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        if not line.strip():
            i += 1
            continue

        if line.strip().startswith("```"):
            i += 1
            buf = []
            while i < len(lines) and not lines[i].strip().startswith("```"):
                buf.append(lines[i])
                i += 1
            i += 1
            story.append(Preformatted("\n".join(buf), styles["CodeDoc"]))
            continue

        if (
            "|" in line
            and i + 1 < len(lines)
            and re.match(r"^\s*\|?\s*[-:| ]+\|", lines[i + 1])
        ):
            rows = []
            while i < len(lines) and "|" in lines[i] and lines[i].strip().startswith("|"):
                raw = lines[i].strip()
                if re.match(r"^\|?\s*[-:| ]+\|?$", raw):
                    i += 1
                    continue
                cells = [c.strip() for c in raw.strip("|").split("|")]
                rows.append([Paragraph(inline_md(c), styles["CellDoc"]) for c in cells])
                i += 1
            if rows:
                ncols = max(len(r) for r in rows)
                for r in rows:
                    while len(r) < ncols:
                        r.append(Paragraph("", styles["CellDoc"]))
                if ncols == 2:
                    widths = [2.2 * inch, 4.8 * inch]
                elif ncols == 3:
                    widths = [1.6 * inch, 2.4 * inch, 3.0 * inch]
                else:
                    widths = [7.0 * inch / ncols] * ncols
                t = Table(rows, colWidths=widths, repeatRows=1)
                t.setStyle(
                    TableStyle(
                        [
                            ("GRID", (0, 0), (-1, -1), 0.4, colors.grey),
                            ("BACKGROUND", (0, 0), (-1, 0), colors.HexColor("#e8eef5")),
                            ("VALIGN", (0, 0), (-1, -1), "TOP"),
                            ("LEFTPADDING", (0, 0), (-1, -1), 4),
                            ("RIGHTPADDING", (0, 0), (-1, -1), 4),
                            ("TOPPADDING", (0, 0), (-1, -1), 3),
                            ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
                        ]
                    )
                )
                story.append(t)
                story.append(Spacer(1, 8))
            continue

        if line.startswith("# "):
            story.append(Paragraph(inline_md(line[2:]), styles["H1Doc"]))
            story.append(
                HRFlowable(
                    width="100%",
                    thickness=1,
                    color=colors.HexColor("#003366"),
                    spaceAfter=8,
                )
            )
            i += 1
            continue
        if line.startswith("## "):
            story.append(Paragraph(inline_md(line[3:]), styles["H2Doc"]))
            i += 1
            continue
        if line.startswith("### "):
            story.append(Paragraph(inline_md(line[4:]), styles["H3Doc"]))
            i += 1
            continue

        m = re.match(r"^[-*] \[([ xX])\] (.+)$", line.strip())
        if m:
            mark = "[X]" if m.group(1).lower() == "x" else "[ ]"
            story.append(Paragraph(f"{mark} {inline_md(m.group(2))}", styles["CheckDoc"]))
            i += 1
            continue
        m = re.match(r"^(\d+)\. (.+)$", line.strip())
        if m:
            story.append(
                Paragraph(
                    f"<b>{m.group(1)}.</b> {inline_md(m.group(2))}",
                    styles["BulletDoc"],
                )
            )
            i += 1
            continue
        if re.match(r"^[-*] ", line.strip()):
            story.append(Paragraph("• " + inline_md(line.strip()[2:]), styles["BulletDoc"]))
            i += 1
            continue
        if re.match(r"^---+$", line.strip()):
            story.append(Spacer(1, 4))
            story.append(
                HRFlowable(
                    width="100%",
                    thickness=0.5,
                    color=colors.lightgrey,
                    spaceBefore=2,
                    spaceAfter=8,
                )
            )
            i += 1
            continue

        story.append(Paragraph(inline_md(line.strip()), styles["BodyDoc"]))
        i += 1

    return story


def main() -> None:
    text = MD_PATH.read_text(encoding="utf-8")
    story = build_story(text)

    def add_page_number(canvas, doc):
        canvas.saveState()
        canvas.setFont("Helvetica", 8)
        canvas.setFillColor(colors.grey)
        page = canvas.getPageNumber()
        canvas.drawCentredString(letter[0] / 2, 0.5 * inch, f"MSCC Pi install — page {page}")
        canvas.drawString(0.75 * inch, letter[1] - 0.5 * inch, "mscc-station / pi-install")
        canvas.restoreState()

    doc = SimpleDocTemplate(
        str(OUT_PATH),
        pagesize=letter,
        leftMargin=0.7 * inch,
        rightMargin=0.7 * inch,
        topMargin=0.7 * inch,
        bottomMargin=0.7 * inch,
        title="MSCC on Raspberry Pi — one how-to",
        author="MSCC station",
    )
    doc.build(story, onFirstPage=add_page_number, onLaterPages=add_page_number)
    print(f"Wrote {OUT_PATH} ({OUT_PATH.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
