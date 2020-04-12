#!/bin/sh
#This script can be run under Linux to launch m68k vbcc

PATH=/opt/m68k-amigaos/bin:$PATH ninja -f build.os3fpu $@

