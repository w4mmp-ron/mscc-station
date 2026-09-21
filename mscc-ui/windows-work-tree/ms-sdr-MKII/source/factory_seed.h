#ifndef FACTORY_SEED_H
#define FACTORY_SEED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Map FW major → factory product-line folder name. Unknown → proficio-mkii. */
const char *Factory_line_from_major(int major);

/* Swap/load per-line IQ+QRP user cache (AppData cal/<line>/). Freq: factory if live missing. */
void Factory_seed_live_inis(void);

/* Overwrite live file from factory/<kind>/<line>/<leaf>. IQ/QRP also overwrite cal/<line>/. Returns 1 on success. */
int Factory_reseed_live_file(const char *kind, const char *leaf);

/* Copy live iq.ini or power_cal.ini into cal/<current line>/. */
void Factory_mirror_live_to_cal(const char *leaf);

#ifdef __cplusplus
}
#endif
#endif
