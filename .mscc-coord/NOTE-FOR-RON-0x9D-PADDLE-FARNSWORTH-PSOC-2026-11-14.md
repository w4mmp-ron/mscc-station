# Note for Ron: 0x9D SET_PADDLE_FARNSWORTH not in any PSoC tree (2026-11-14)

From Stew (written with Grok Bot). Question, no code changed.

## What we see
- ms-sdr (Linux and Windows) sends 0x9D SET_PADDLE_FARNSWORTH in initialize_keyer at every server start when a keyer is installed.
- Stew's Ultimus MKII ATU (7.246, built from current source) rejects it: ms-sdr.log shows "usbControlMsgOUT FAILED. Status: -9" (USB stall). Harmless, logged only, the rest of initialize_keyer goes through.
- Checked origin/main (c5834cf2): no 0x9D case in usbvend.h / usbvend01.c / cw.c in any of the five trees, MKII-PTT included. 0x76 SET_MEM_TEXT_WPM is there in all of them.
- Your 11-05 note (NOTE-FOR-STEW-KEYER-PADDLE-FARNSWORTH-2026-11-05) lists "PSoC boards: pass 0x9D to the PIC like 0x76 in Configure_CW" as not done. Keyer-MCU-app-20261112 already handles 0x9D, so only the PSoC forwarding is missing.

## Question
Was the PSoC side left out on purpose (waiting on something), or just not done yet? If it's just pending:
- MKII trees only (MKII-PTT, MKII-ATU, Geminus-MKII, plus the Ultimus builds from build-ultimus.py). Legacy and Geminus-Legacy have no keyer and ms-sdr never sends 0x9D to them.
- Same pattern as 0x76: an E_paddle_farns variable, an OUT case in usbvend01.c, and a Configure_CW state to push it to the PIC.

Do you want to do it in your tree, or should Stew have Build do it on Shack with the pending cmd-080 (Multus IQ USB name) firmware batch? Either way the firmware gets built once for all radios.

## Also FYI
- cmd-080 "Multus IQ" USB product name is built on Shack for all 8 images (PL 1.237/6.237, PTT 3.245/8.245, ATU 4.246/7.246, GL 5.154, GM 2.155), not committed yet. Stew is testing before "finished". If you change firmware, pull after that commit, or tell Stew first so the version numbers don't collide.
- Stew is updating the keyer PIC on his Ultimus MKII ATU now. That's the first 0x9F test on a PSoC (daughter-board) radio, per STEW-KEYER-UPDATE-TEST.md.
