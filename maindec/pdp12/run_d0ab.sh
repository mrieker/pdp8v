#!/bin/bash
#
#  Run PDP-12 basic instruction test
#
#  Gets undefined LINC opcode 0004 at 06744
#  ...but does not halt and otherwise passes
#
#  Rings TTY bell approx once per second
#
#   export MAINDECOPTS='-nohw'
#   $ ./run_d0ab.sh
#   raspictl: HALT PC=00022 L=0 AC=0000
#   Linc::execute: LINC HALT PC=00026 L=0 AC=7777
#   IODevTTY::prthread: wrote char 007 <.>
#   IODevTTY::prthread: wrote char 007 <.>
#   IODevTTY::prthread: wrote char 007 <.>
#   IODevTTY::prthread: wrote char 007 <.>
#   IODevTTY::prthread: wrote char 007 <.>
#
#   export MAINDECOPTS='-cpuhz 52000 -mintimes'
#   - dings bell 3 times / minute
#
export iodevrtc_factor=1
export iodevtty=/dev/pts/2
export iodevtty_debug=1
export senseregister=77
export leftswitches=7777
export rightswitches=7777
exec ../../driver/raspictl -binloader -startpc 20 -haltprint -haltcont -mintimes $MAINDECOPTS maindec-12-d0ab-pb.bin 2>&1 | grep -v 'unsupported linc opcode 0004 at 06744'
