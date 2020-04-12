#!/bin/sh
ninja -f build.linux64 && cp -f build/linux64/xlsreader.hwp ~/devel/Hollywood/Plugins/xlsreader.hwp && ~/devel/Hollywood/Interpreter test_xlsreader.hws

