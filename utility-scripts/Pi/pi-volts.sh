#!/bin/bash
# pi-volts.sh - show the Pi 5 5 V input voltage and the low-voltage flags.
#
#   pi-volts.sh        one reading
#   pi-volts.sh 2      a reading every 2 s (Ctrl-C to stop), e.g. while transmitting
#
# Pi 5 warns below about 4.63 V. Flags come from "vcgencmd get_throttled".

show() {
    v=$(vcgencmd pmic_read_adc EXT5V_V | sed 's/.*=//; s/V$//')
    t=$(vcgencmd get_throttled | sed 's/.*=//')
    msg=""
    (( t & 0x1 ))     && msg+=" LOW-NOW"
    (( t & 0x10000 )) && msg+=" WAS-LOW-SINCE-BOOT"
    (( t & 0x4 ))     && msg+=" THROTTLED-NOW"
    [ -z "$msg" ] && msg=" OK"
    printf '%s  %.3f V  flags %s%s\n' "$(date +%H:%M:%S)" "$v" "$t" "$msg"
}

if [ -n "$1" ]; then
    while true; do show; sleep "$1"; done
else
    show
fi
