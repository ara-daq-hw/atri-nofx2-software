#!/bin/bash

# this script JUST LAUNCHES ARAAcqd in the FOREGROUND
# I _do not know_ how ARAAcqd is spawned in Crazy Weird
# Operations Mode.

# get current script dir
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# set up library search paths and preloads
NOFX2_SOFT_DIR=${SCRIPT_DIR}

PRE=${NOFX2_SOFT_DIR}/nofx2ControlLib/nofx2ControlLib.so
LPATH=${NOFX2_SOFT_DIR}/libmcp:${NOFX2_SOFT_DIR}/libmcp2221-master/libmcp2221/bin

# launch ARAAcqd
LD_LIBRARY_PATH=${LPATH}:${LD_LIBRARY_PATH} LD_PRELOAD=${PRE} ARAAcqd
