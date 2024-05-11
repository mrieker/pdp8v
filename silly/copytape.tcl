#
#  ../driver/raspictl -script copytape.tcl
#
reset [loadlink copytape.oct]
option set skipopt 1
run
puts ""
puts "  iodev ptape load punch <outputfile>"
puts "  iodev ptape load reader <inputfile>"
puts "  exit when done"
puts ""
