#!/bin/bash
#
#  run with real tube set
#  assumes paddles plugged in but are not used
#
#    -mintimes -cpuhz 22000 < latest for 'rpi seq ma alu acl' boards
#    -mintimes -cpuhz 21000 < latest for 'rpi seq ma alu acl pc' boards
#
#    maybe add -cpuhz 5
#    maybe add -printstate
#    maybe add -mintimes
#    maybe add -pause
#
./loadmod.sh
./iowowner.sh
. ./iowsns.si
exec ./testboard.`uname -m` -csrclib rpi seq ma alu acl pc "$@"
