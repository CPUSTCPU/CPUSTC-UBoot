if {![info exists ::env(BIT_FILE)] || $::env(BIT_FILE) eq ""} {
    error "BIT_FILE is required"
}

set bit_file [file normalize $::env(BIT_FILE)]
if {![file exists $bit_file]} {
    error "bitstream does not exist: $bit_file"
}

open_hw_manager
connect_hw_server -allow_non_jtag
open_hw_target

set devices [get_hw_devices -filter {PART =~ "xc7a200t*"}]
if {[llength $devices] != 1} {
    error "expected exactly one xc7a200t, found [llength $devices]: $devices"
}

set device [lindex $devices 0]
puts "PROGRAM_DEVICE=$device"
puts "PROGRAM_BIT=$bit_file"
set_property PROGRAM.FILE $bit_file $device
program_hw_devices $device
refresh_hw_device $device
puts "PROGRAM_RESULT=success"
close_hw_manager
