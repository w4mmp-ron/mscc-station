#ifndef FACTORY_SEED_H
#define FACTORY_SEED_H

#ifdef __cplusplus
extern "C" {
#endif

/* Map FW major → factory product-line folder name. Unknown → proficio-mkii. */
const char *Factory_line_from_major(int major);

/* If live iq.ini / freq_cal.ini / power_cal.ini are missing, copy from
 * factory/<kind>/<line>/ next to the exe. Does not overwrite existing live files. */
void Factory_seed_live_inis(void);

/* Overwrite live file from factory/<kind>/<line>/<leaf>. Returns 1 on success. */
int Factory_reseed_live_file(const char *kind, const char *leaf);

#ifdef __cplusplus
}
#endif
#endif
