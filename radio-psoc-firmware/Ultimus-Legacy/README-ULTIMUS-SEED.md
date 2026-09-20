# Ultimus trees (seed copies)

These folders were copied from Proficio so Ultimus can diverge without touching shipping Proficio sources.

- `Ultimus-Legacy/` ← `Proficio-Legacy/`
- `Ultimus-MKII-PTT/` ← `Proficio-MKII-PTT/`
- `Ultimus-MKII-ATU/` ← `Proficio-MKII-ATU/`

Creator project is still `Proficio-Legacy.cydsn` / `.cywrk` (rename deferred). Firmware major is **6**; USB product string in generated `USBFS_descr.c` is **Ultimus**. A Creator USB regenerate can restore “Proficio” until the USBFS component catalog string is edited.

After build, copy shipping `.cyacd` / `.hex` to `../release/<this-folder-name>/`.

**Do not flash Ultimus clones to Proficio customers** until renamed and versioned.
