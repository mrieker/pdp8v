swreg 0                 ;# set switchregister to 0
iodev tty lcucin 1      ;# convert lowercase to uppercase on input
iodev tty telnet 12303  ;# telnet to port 12303 for tty
option set quiet 1      ;# suppress undefined io opcode messages
loadbin bins/focal69.bin
reset 0200
run
puts ""
puts "  telnet into 12303"
puts ""
puts "  should get '*' prompt immediately on telnet 12303 port"
puts ""
puts "  to read source file using high-speed paper tape:"
puts "  enter 'iodev ptape load reader -ascii <filename>' here"
puts "  enter '*'CR command at '*' prompt in focal"
puts "  - seems to work only once!"
puts ""
puts "  enter 'exit' command here when done"
puts ""
