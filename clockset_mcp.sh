#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

export LD_LIBRARY_PATH=${SCRIPT_DIR}/libmcp:${SCRIPT_DIR}/libmcp2221-master/lib/unix:${LD_LIBRARY_PATH}

I2CWR="${SCRIPT_DIR}/mcpi2c_wr/mcpi2c_wr 0x68"

# Reset the Si5367/8
$I2CWR 0x88 0x80
$I2CWR 0x88 0x00

# Turn on the FPGA oscillator.
#sendFx2ControlPacket 0x40 0xB6 0x1 0x00
# If we initialize to the external Rubidium clock, we instead
# would disable the FPGA oscillator.

# Register 0: No free run, no clockout always on, CKOUT5 is output, no bypass
$I2CWR 0x00 0x14

# Register 1: Clock priorities. 1, then 2, then 3.
$I2CWR 0x01 0xE4

# Register 2: BWSEL_REG[3:0], 0010. BWSEL = 0000.
$I2CWR 0x02 0x02

# Register 3: CKSEL_REG[1:0], DHOLD, SQ_ICAL, 0101. CKSEL_REG = 10: clock 1 (ext. Rb)
$I2CWR 0x03 0x05

# Register 4: Autoselect clocks enabled (1, then 2, then 3)
$I2CWR 0x04 0x92

# Register 5: CKOUT2/CKOUT1 are LVDS
$I2CWR 0x05 0x3F
# Register 6: CKOUT4/CKOUT3 are LVDS
$I2CWR 0x06 0x3F
# Register 7: CKOUT5 is LVDS. CKIN3 is frequency offset alarms (who cares)
$I2CWR 0x07 0x3B
# Register 8: Hold logic. All normal ops.
$I2CWR 0x08 0x00
# Register 9: Histogram generation, plus hold logic.
$I2CWR 0x09 0xC0
# Register 10: Output buffer power.
$I2CWR 0x0A 0x00
# Register 11: Power down CKIN4 input buffer.
$I2CWR 0x0B 0x48
# Register 19: Frequency offset alarn, who cares.
$I2CWR 0x13 0x2C
# Register 20: Alarm pin setups.
$I2CWR 0x14 0x02
# Register 21: Ignore INC/DEC/FSYNC_ALIGN, tristate CKx_ACTV, ignore CKSEL
# bleh this is default now 
$I2CWR 0x15 0xE0
# Register 22: Default
$I2CWR 0x16 0xDF
# Register 23: mask everything
$I2CWR 0x17 0x1F
# Register 24: mask everything
$I2CWR 0x18 0x3F

# These determine output clock frequencies.
# CKOUTx is 5 GHz divided by N1_HS divided by NCx_LS
# CKOUT1 = D2REFCLK
# CKOUT2 = D3REFCLK
# CKOUT3 = D1REFCLK
# CKOUT4 = D4REFCLK
# CKOUT5 = FPGA_REFCLK

# Right now all daughter refclks are 25 MHz, and the FPGA refclk is 100 MHz.
# This should probably be changed so that the daughter refclks are at least
# 100 MHz.

# Register 25-27: N1_HS=001 (N1=5), NC1_LS = 0x09 (10)
$I2CWR 0x19 0x20
$I2CWR 0x1A 0x00
$I2CWR 0x1B 0x09
# Register 28-30: NC2_LS = 0x09 (10)
$I2CWR 0x1C 0x00
$I2CWR 0x1D 0x00
$I2CWR 0x1E 0x09
# Register 31-33: NC3_LS = 0x09 (10) 
$I2CWR 0x1F 0x00
$I2CWR 0x20 0x00
$I2CWR 0x21 0x09
# Register 34-36: NC4_LS = 0x09 (10)
$I2CWR 0x22 0x00
$I2CWR 0x23 0x00
$I2CWR 0x24 0x09
# Register 37-39: NC5_LS = 0x09 (10)
$I2CWR 0x25 0x00
$I2CWR 0x26 0x00
$I2CWR 0x27 0x09

# These determine the VCO frequency.
# It's (CKINx/N3x)*(N2_HS)*(N2_LS).
# For CKIN1 or CKIN3 = 10 MHz, this is (10/1)*(10)*(500) = 5 GHz

# Register 40-42: N2_HS = 110 (10), N2_LS = 500
$I2CWR 0x28 0xC0
$I2CWR 0x29 0x01
$I2CWR 0x2A 0xF4
# Register 44-45: CKIN1 divider (N31) = 0x0 (1)
$I2CWR 0x2B 0x00
$I2CWR 0x2C 0x00
$I2CWR 0x2D 0x00
# Register 46-48: CKIN2 divider (N32) = 0x0 (1)
$I2CWR 0x2E 0x00
$I2CWR 0x2F 0x00
$I2CWR 0x30 0x00
# Register 49-51: CKIN3 divider (N33) = 0x0 (1)
$I2CWR 0x31 0x00
$I2CWR 0x32 0x00
$I2CWR 0x33 0x00
# done, now do an ICAL
$I2CWR 0x88 0x40

# all done
