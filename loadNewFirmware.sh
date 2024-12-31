#!/bin/bash

# get current script dir
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

if [ ! -e /dev/xillybus_spi_in ] ; then
    echo "Not in bootloader mode. Run 'toBootloader.sh' first."
    exit 1
fi

python3 ${SCRIPT_DIR}/loadSPIFlash.py $1
