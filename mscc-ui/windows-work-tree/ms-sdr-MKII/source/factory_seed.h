#ifndef FACTORY_SEED_H
#define FACTORY_SEED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Map FW major → factory product-line folder name. Unknown → proficio-mkii. */
const char *Factory_line_from_major(int major);

/* Same line: leave an existing live iq.ini, power_cal.ini, and amplifier_cal.ini.
 * Different line: stash those live files, then parked (or factory) → live.
 * Missing live with a parked copy: parked → live.
 * No live and no parked: factory → parked → live. Amp cal with no factory file: -99.
 * Freq stays seed-if-missing and is not parked. Sets the reload flags. */
void Factory_seed_live_inis(void);

/* After the network is up. INITIALIZE and/or IQ reload only if seed replaced a live file. */
void Factory_seed_reload_servers(void);

/* Overwrite the live file from factory/<kind>/<line>/<leaf>. Does not touch cal/<line>/. */
int Factory_reseed_live_file(const char *kind, const char *leaf);

/* Save settings: copy live iq.ini, power_cal.ini, and amplifier_cal.ini into cal/<line>/
 * and write LAST_LINE.txt. Unknown major (not 1..8): warn and no-op. */
void Factory_park_live_settings(void);

#ifdef __cplusplus
}
#endif
#endif
