#
#  ../driver/raspictl -script typetape.tcl
#
reset [loadlink typetape.oct]
iodev tty telnet 12303
option set skipopt 1
run
puts ""
puts "  telnet <thishost> 12303 -- to see output"
puts ""
puts "  iodev ptape load reader <inputfile>"
puts "  exit when done"
puts ""
