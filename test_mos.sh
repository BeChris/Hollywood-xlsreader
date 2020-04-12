#!/bin/sh
./compile_mos.sh && cp -f build/mos/xlsreader.hwp ~/devel/Hollywood/LinkerPlugins/ppc-morphos/xlsreader.hwp && ~/devel/Hollywood/Interpreter -compress -compile test_xlsreader -exetype morphos -overwrite test_xlsreader.hws

