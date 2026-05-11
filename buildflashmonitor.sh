#!/bin/bash

fuser -k /dev/ttyUSB0 2>/dev/null; sleep 0.3
. /home/levi/esp-idf/export.sh ; idf.py build flash monitor

