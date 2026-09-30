# MSCC operation manuals (drafts)

Draft manuals for Ron's review.

| File | What it covers |
|------|----------------|
| [`MSCC-Linux-Local-Operation-DRAFT.pdf`](MSCC-Linux-Local-Operation-DRAFT.pdf) | MSCC Operator's Guide: Linux and Raspberry Pi, local operation (preliminary) |
| [`MSCC-Windows-Operation-DRAFT.pdf`](MSCC-Windows-Operation-DRAFT.pdf) | MSCC Operator's Guide: Windows (preliminary) |
| [`MSCC-Remote-Operation-DRAFT.pdf`](MSCC-Remote-Operation-DRAFT.pdf) | MSCC Operator's Guide: Remote Operation (preliminary) |

All are preliminary drafts. Open questions were marked in **yellow boxes** inside each PDF.

## Ron answers applied (paste-ready — PDFs not regenerated yet)

Ron filled [`.mscc-coord/QUESTIONS-FOR-RON.md`](../../.mscc-coord/QUESTIONS-FOR-RON.md)
(2026-09-28/29). Paste-ready replacement text for the Linux and Windows guides:

**[`.mscc-coord/MANUAL-UPDATES-FROM-RON.md`](../../.mscc-coord/MANUAL-UPDATES-FROM-RON.md)**

Summary:

| Topic | Ron | Manual action |
|-------|-----|---------------|
| Pi groups | postinst adds installer user; no manual `usermod` for that user; log out after first install; other users need `usermod` | Clear Pi yellow; replace Pi §2.2 text |
| Upgrade / cal | kept on upgrade is correct; seed only if folder missing/empty | Clear yellow on §2.6 / p.16; keep keep-sentence; optional seed note |
| CW POTENTIA / QSK | Potentia break-in timing; default off | Replace yellows in Linux CW + Windows CW |
| CW PHONES | not used (`0x70` ignored) | Say "not used"; UI removal is `.mscc-coord/BACKLOG.md` BL-001 |

Remote guide: no change from these answers. Remaining Windows-only yellows (if any) stay in the Windows PDF until answered separately.
