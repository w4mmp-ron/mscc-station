# For Stew: radio calibration not read if the network is late at startup

From Ron, 2026-11-06. An option for you; your choice whether and when.

## What I see

At startup ms-sdr waits up to 5 seconds for the network. If it's ready, ms-sdr reads power,
TX IQ and RX IQ from the radio (`Radio_cal_load_power`, `Radio_cal_load_iq`, after
`Factory_seed_reload_servers`) in `main.c`. If it isn't, the log says "network not ready -
appliance startup deferred to client connect". The client connect does run
`Apply_Appliance_Startup`, but nothing reads the radio's calibration, so that run uses the ini
files' values until the next restart.

## What the Pi ms-sdr does now (3.194)

One function in `power_calibration.c`, done once per run as soon as the network is up:

```c
static pthread_mutex_t radio_cal_startup_lock = PTHREAD_MUTEX_INITIALIZER;
static int radio_cal_startup_done = 0;

void Radio_cal_startup_load(const char *who) {
    pthread_mutex_lock(&radio_cal_startup_lock);
    if (!radio_cal_startup_done && G_network_initialized) {
        Factory_seed_reload_servers();
        Radio_cal_load_power();
        Radio_cal_load_iq();
        radio_cal_startup_done = 1;
    }
    pthread_mutex_unlock(&radio_cal_startup_lock);
}
```

- `main.c`: the three calls are replaced by `Radio_cal_startup_load("startup");`
- `main-controller.c`, client connect, just before `if (!G_appliance_startup_done)`:
  `Radio_cal_startup_load("client connect");`

## If you want it

The Windows ms-sdr has the same code in the same places (`main.c` ~1340,
`main-controller.c` ~1550), so the same change fits. I can make it if you'd like.
