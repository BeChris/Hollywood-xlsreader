#!/bin/sh
#This script can be run under Linux to launch ppc gcc

PATH=/opt/ppc-amigaos/bin:$PATH ninja -f build.os4 $@

