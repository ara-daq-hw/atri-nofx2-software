#!/bin/bash

# this script JUST LAUNCHES ARAAcqd in the FOREGROUND
# I _do not know_ how ARAAcqd is spawned in Crazy Weird
# Operations Mode.

# MAYBE this script should launch ARAAcqd in the background??
# And have signal handling maybe??
# I HAVE NO IDEA

# VERBOSITY CONTROLS FOR CONTROL/EVENT READER
# FROM 0 UP TO 3
# 3 PRODUCES A BUCKET TON OF OUTPUT

CONTROL_VERBOSE=0
EVENT_VERBOSE=0

# AAAAUGH THIS IS SUCH A GIGANTIC MESS
SETUP_SCRIPT=/home/ara/WorkingDAQ/setupAraDaq.sh

# Modified script to start up nofx2ControlReader and nofx2EventReader
# first and then kill then on exit since I'm effing sick of doing it
SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
NOFX2_SOFT_DIR=${SCRIPT_DIR}
PACKET_PIPE=/tmp/atri_inpkts
FRAME_PIPE=/tmp/atri_inframes

if [ -e ${PACKET_PIPE} ] ; then
    rm -rf ${PACKET_PIPE}
fi
if [ -e ${FRAME_PIPE} ] ; then
    rm -rf ${FRAME_PIPE}
fi

${NOFX2_SOFT_DIR}/nofx2ControlReader/nofx2ControlReader ${CONTROL_VERBOSE} &
CTRLPID=$!
${NOFX2_SOFT_DIR}/nofx2EventReader/nofx2EventReader ${EVENT_VERBOSE} &
EVNTPID=$!
sleep 0.2

source ${SETUP_SCRIPT}
PRE=${NOFX2_SOFT_DIR}/nofx2ControlLib/nofx2ControlLib.so
LPATH=${NOFX2_SOFT_DIR}/libmcp:${NOFX2_SOFT_DIR}/libmcp2221-master/libmcp2221/bin

LD_LIBRARY_PATH=${LPATH}:${LD_LIBRARY_PATH} LD_PRELOAD=${PRE} ARAAcqd

# this is a terrible way to do this!!
kill $CTRLPID
kill $EVNTPID
wait
sleep 0.2

rm -rf ${PACKET_PIPE}
rm -rf ${FRAME_PIPE}
