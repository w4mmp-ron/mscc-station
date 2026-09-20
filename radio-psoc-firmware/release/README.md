# Firmware release drops

After a successful PSoC Creator / Keil build on **Shack**, Grok Build copies the shipping artifacts here:

| Artifact | Typical source | Drop here |
|----------|----------------|-----------|
| Application `.cyacd` | tree `Release/*.cyacd` (after `copy-release`) | `release/<RadioName>/` |
| Application `.hex` | Creator/Keil output (same build) | `release/<RadioName>/` |

Do **not** put bootloader `.hex` here unless Stew explicitly asks (LOADER is MiniProg3 / factory only).

Folder names match the sibling source trees under `radio-psoc-firmware/`. Prefer dated filenames (e.g. `Ultimus-MKII-PTT-20260920.cyacd`) when copying.

`.gitkeep` files keep empty folders in git until the first drop.
