#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
CUR_DIR=`pwd`
cd ${SCRIPT_DIR}/libmcp
make $@
cd ${SCRIPT_DIR}/libmcp2221-master/libmcp2221/
make $@
cd ${SCRIPT_DIR}/mcpi2c_wr
make $@
cd ${SCRIPT_DIR}/nofx2ControlLib
make $@
cd ${SCRIPT_DIR}/nofx2ControlReader
make $@
cd ${SCRIPT_DIR}/nofx2EventReader
make $@
cd ${CUR_DIR}