#!/bin/bash
#
#  Run random memory test on the DE0
#
#  on homepc: ./programit.sh    to load design into DE0
#  on raspi:  ./run_randmem.sh  to run test program
#
export switchregister=random
cpuhz=80000 exec ../driver/raspictl -mintimes -randmem -quiet
