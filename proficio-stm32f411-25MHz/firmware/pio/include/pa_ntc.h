#ifndef PA_NTC_H
#define PA_NTC_H

void pa_ntc_init(void);
/** Read PA/board NTC on PA4; updates E_pa_temp (°C, integer). */
void pa_ntc_poll(void);

#endif /* PA_NTC_H */
