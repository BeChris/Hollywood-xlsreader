xlsreader - Read XLS files from Hollywood

Hollywood is a commercial multimedia-oriented programming language that can be used to create applications and games very easily (https://hollywood-mal.com/)

This plugin exposes one new function to Hollywood scripts : xlsreader.OpenXls()

/* Open an .xls file and:
** 1)If everything went well return its content as a table as described below:
** {
**   ["first tab"] = {
**    (note:first row) {"First column value", "Second column value", "Third column value", ...},
**    (note:second row) {"First column value", "Second column value", "Third column value", ...},
**    (note:third row) {"First column value", "Second column value", "Third column value", ...},
**     ...
**   },
**   ["second tab"] = {
**    (note:first row) {"First column value", "Second column value", "Third column value", ...},
**    (note:second row) {"First column value", "Second column value", "Third column value", ...},
**    (note:third row) {"First column value", "Second column value", "Third column value", ...},
**     ...
**   },
**   ...
** }
**
** 2)If something went wrong raise an error containing message
*/


Now I explain how I cross compile all plugin.hwp from a Linux Manjaro 64 bits system.

For Windows 32 bits:
===================
wine must be installed (currently I have the very last wine 5.4)
winetricks must also be installed

then (to be done only once):
WINEPREFIX=<prefix> winetricks -q psdkwin7

Then execute compile_windows.sh script everytime to compile all source files and generate the final plugin.hwp in build/win32/

For Windows 64 bits:
===================
TODO

For MacOS 64 and 32 bits:
========================
Install virtualbox at least 6.1.4

Then, in a VM, install MacOS following guide at : https://github.com/myspaghetti/macos-guest-virtualbox

Note : To install HighSierra instead of Catalina, I modified in macos-guest-virtualbox.sh:
macOS_release_name="Catalina"
by:
macOS_release_name="HighSierra"

Then, after having launched the script:
Upon "Press enter when the Terminal command prompt is ready." prompt appears I pressed CTRL+C to interrupt it (the installation continue without issues in the vm).

Then within the vm, open a Terminal and enter:
gcc

The update manager will propose to install XCode or developper command line tools : select installation for developper command line tools.

Then, for all compilations, change to the plugin source directory and enter:
make -f makefile.macos64 and/or make -f makefile.macos
=> plugin.hwp will be generated in build/macos64 and/or in build/macos.

AmigaOS 3.x (m68k):
==================
Download premade cross compilation toolchain at : https://fengestad.no/m68k-amigaos-toolchain/
(or, if it works for you : https://github.com/adtools/amigaos-cross-toolchain)

Cross toolchain to be installed in /opt/m68k-amigaos

Then to compile:
./compile_os3.sh


