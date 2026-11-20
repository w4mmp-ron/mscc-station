#!/bin/bash
# pi-volts.sh - show the Pi's 5 V input voltage (Pi 5) and the low-voltage flags (every Pi).
#
#   pi-volts.sh        one reading
#   pi-volts.sh 2      a reading every 2 s (Ctrl-C to stop), e.g. while transmitting
#
# Pi 5: reads the 5 V input from the power chip; it warns below about 4.63 V.
# Other Pis (Pi 4, Zero 2 W): there is no chip to read the 5 V input, so it shows
# "5V n/a" and the core voltage instead; the LOW flags still say if the input dropped
# too low (about 4.63 V). Flags come from "vcgencmd get_throttled".

if vcgencmd pmic_read_adc EXT5V_V 2>/dev/null | grep -q '^ *EXT5V_V'; then
    HAVE_ADC=1
else
    HAVE_ADC=0
fi

show() {
    if [ "$HAVE_ADC" = 1 ]; then
        v=$(vcgencmd pmic_read_adc EXT5V_V | sed 's/.*=//; s/V$//')
        volts=$(printf '5V %.3f V' "$v")
    else
        c=$(vcgencmd measure_volts core | sed 's/.*=//; s/V$//')
        volts=$(printf '5V n/a  core %.3f V' "$c")
    fi
    t=$(vcgencmd get_throttled | sed 's/.*=//')
    msg=""
    (( t & 0x1 ))     && msg+=" LOW-NOW"
    (( t & 0x10000 )) && msg+=" WAS-LOW-SINCE-BOOT"
    (( t & 0x4 ))     && msg+=" THROTTLED-NOW"
    (( t & 0x40000 )) && msg+=" WAS-THROTTLED"
    [ -z "$msg" ] && msg=" OK"
    printf '%s  %s  flags %s%s\n' "$(date +%H:%M:%S)" "$volts" "$t" "$msg"
}

if [ -n "$1" ]; then
    while true; do show; sleep "$1"; done
else
    show
fi
