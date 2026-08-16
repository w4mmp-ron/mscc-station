#!/usr/bin/env python3
"""
Play the CQ message already stored in the keyer EEPROM.
Does not store or change memory.

  mscc stop
  sudo python3 keyer-mem-play.py
"""
import os
import subprocess
import sys

here = os.path.dirname(os.path.abspath(__file__))
test = os.path.join(here, "keyer-mem-test.py")
sys.exit(subprocess.call([sys.executable, test, "--play-only"] + sys.argv[1:]))
