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


How to compile:
==============
1)Eventually edit build.ini
2)Execute python genbuildfiles.py (generates all the build.* files)
3)Execute ninja -f build.(linux|linux64|mos|...)
(Ninja homepage : https://ninja-build.org/)


Now I explain how I cross compile all plugin.hwp from a Linux Manjaro 64 bits system.

For AROS i386:
=============
git clone https://github.com/aros-development-team/AROS.git
cd AROS
mkdir contrib
mkdir ports
git clone https://github.com/aros-development-team/contrib.git
git clone https://github.com/aros-development-team/ports.git
git submodule update --init --recursive
./configure --prefix=/opt/i386-aros --target=pc-i386
make
cd bin/linux-x86_64/tools/crosstools
mkdir /opt/i386-aros/bin
cp -r * /opt/i386-aros/bin/


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


For MacOS 64 and 32 bits (in a VM):
==================================
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


For MacOS 64 and 32 bits (cross compiler in Linux):
==================================================
1)Log in to a MacOS system where XCode is installed
2)git clone https://github.com/tpoechtrager/osxcross.git
3)cd osxcross
4)./tools/gen_sdk_package.sh
5)=> copy generated MacOSX*.tar.bz2 to linux
6)Log in to the Linux host system
7)git clone https://github.com/tpoechtrager/osxcross.git
8)cd osxcross
10)mkdir /opt/i386-macos
9)TARGET_DIR=/opt/i386-macos ./build.sh


AmigaOS 3.x (m68k):
==================
Download premade cross compilation toolchain at : https://fengestad.no/m68k-amigaos-toolchain/
(or, if it works for you : https://github.com/adtools/amigaos-cross-toolchain)

Cross toolchain to be installed in /opt/m68k-amigaos

Then to compile:
./compile_os3.sh


MorphOS (ppc):
=============
Generate a cross compilation toolchain using https://github.com/AmigaPorts/morphos-cross-toolchain

Cross toolchain to be installed in /opt/ppc-morphos

Then to compile:
./compile_mos.sh

I succeed in installing and running MorphOS under qemu following directives at : http://amigawarez.com/index.php?resources/morphos-on-qemu.297/
Patched openbios-qemu.elf must be downloaded from http://zero.eik.bme.hu/~balaton/qemu/amiga
Installation done under /opt/ppc-morphos-emulator

Before installating the system, a 20G harddisk image must be created using command:
qemu-img create -f raw /opt/ppc-morphos-emulator/mos.raw 20G

To run the emulator on MorphOS 3.11 installation cd:

Then, to install the system from cd:
qemu-system-ppc -machine mac99,via=pmu -m 512 -vga none -device sm501  -boot d -prom-env "boot-device=hd:,\boot.img" -bios /opt/ppc-morphos-emulator/openbios-qemu.elf -hda /opt/ppc-morphos-emulator/mos.raw -hdb /opt/ppc-morphos-emulator/data.raw -serial stdio -net none -netdev user,id=network01 -device sungem,netdev=network01 -ctrl-grab -sdl

Then, after system is installed, to boot on emulated harddrive:
qemu-system-ppc -machine mac99,via=pmu -m 512 -vga none -device sm501  -boot d -prom-env "boot-device=hd:,\boot.img" -bios /opt/ppc-morphos-emulator/openbios-qemu.elf -hda /opt/ppc-morphos-emulator/mos.raw -hdb /opt/ppc-morphos-emulator/data.raw -serial stdio -net none -netdev user,id=network01 -device sungem,netdev=network01 -ctrl-grab -sdl


AmigaOS 4.1 (ppc):
=================
Generate a cross compilation toolchain using https://github.com/sba1/adtools

To generate the cross toolchain (located in /opt/ppc-amigaos):
git clone https://github.com/sba1/adtools.git
cd adtools
git submodule init
git submodule update
bin/gild checkout binutils 2.23.2
bin/gild checkout coreutils 5.2
bin/gild checkout gcc 8 (this step can be !!! VERY LONG !!!)
  Note:As gcc 8 as been tagged on March, 4th to speed up the "bin/gild checkout gcc 8" step above:
    Edit gild/bin/gild-clone and modify:
    call(['git', 'clone', repo, 'repo'])
    into:
    call(['git', 'clone', '--shallow-since', '2020-03-03', repo, 'repo'])

Final command (adapt -j value to amount of CPU available in your computer):
make -C native-build gcc-cross -j4 CROSS_PREFIX=/opt/ppc-amigaos
