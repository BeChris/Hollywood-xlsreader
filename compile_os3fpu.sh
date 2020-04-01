#!/bin/sh
#This script can be run under Linux to launch m68k vbcc

PATH=/opt/m68k-amigaos/bin:$PATH make -f makefile.os3fpu $@

