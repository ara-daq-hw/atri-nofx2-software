#!/bin/bash

# this script JUST LAUNCHES ARAAcqd in the FOREGROUND
# I _do not know_ how ARAAcqd is spawned in Crazy Weird
# Operations Mode.

NOFX2_SOFT_DIR=/home/ara/atri-nofx2-software/
PRE=${NOFX2_SOFT_DIR}/nofx2ControlLib/nofx2ControlLib.so
LPATH=${NOFX2_SOFT_DIR}/libmcp:${NOFX2_SOFT_DIR}/libmcp2221-master/libmcp2221/bin

LD_LIBRARY_PATH=${LPATH}:${LD_LIBRARY_PATH} LD_PRELOAD=${PRE} ARAAcqd
