#!/bin/sh

DISPLAY=:1
PEKWM_CONFIG_FILE=$PWD/pekwm.config.sys
export DISPLAY PEKWM_CONFIG_FILE
$VALGRIND ../../build/src/sys/pekwm_sys --log-level trace
