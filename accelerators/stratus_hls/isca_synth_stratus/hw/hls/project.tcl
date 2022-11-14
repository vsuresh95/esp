# Copyright (c) 2011-2022 Columbia University, System Level Design Group
# SPDX-License-Identifier: Apache-2.0

############################################################
# Design Parameters
############################################################

#
# Source the common configurations
#
source ../../../common/hls/project.tcl


#
# Set the private memory library
#
use_hls_lib "./memlib"


#
# Local synthesis attributes
#
if {$TECH eq "virtex7"} {
    # Library is in ns, but simulation uses ps!
    set CLOCK_PERIOD 10.0
    set SIM_CLOCK_PERIOD 10000.0
    set_attr default_input_delay      0.1
}
if {$TECH eq "zynq7000"} {
    # Library is in ns, but simulation uses ps!
    set CLOCK_PERIOD 10.0
    set SIM_CLOCK_PERIOD 10000.0
    set_attr default_input_delay      0.1
}
if {$TECH eq "virtexu"} {
    # Library is in ns, but simulation uses ps!
    set CLOCK_PERIOD 8.0
    set SIM_CLOCK_PERIOD 8000.0
    set_attr default_input_delay      0.1
}
if {$TECH eq "virtexup"} {
    # Library is in ns, but simulation uses ps!
    set CLOCK_PERIOD 6.4
    set SIM_CLOCK_PERIOD 6400.0
    set_attr default_input_delay      0.1
}
if {$TECH eq "cmos32soi"} {
    set CLOCK_PERIOD 1000.0
    set SIM_CLOCK_PERIOD 1000.0
    set_attr default_input_delay      100.0
}
if {$TECH eq "gf12"} {
    set CLOCK_PERIOD 1.0
    set SIM_CLOCK_PERIOD 1.0
    set_attr default_input_delay      0.1
}
set_attr clock_period $CLOCK_PERIOD


#
# System level modules to be synthesized
#
define_hls_module isca_synth ../src/isca_synth.cpp


#
# Testbench or system level modules
#
define_system_module tb ../tb/system.cpp ../tb/sc_main.cpp

######################################################################
# HLS and Simulation configurations
######################################################################
set DEFAULT_ARGV ""

foreach dma [list 64] {
    foreach tile_size [list 2 4 8 16 32] {
    define_io_config * IOCFG_TILE_SIZE$tile_size\_DMA$dma -DDMA_WIDTH=$dma -DTILE_SIZE=$tile_size

    define_system_config tb TESTBENCH_TILE_SIZE$tile_size\_DMA$dma -io_config IOCFG_TILE_SIZE$tile_size\_DMA$dma

    define_sim_config "BEHAV_TILE_SIZE$tile_size\_DMA$dma" "isca_synth BEH" "tb TESTBENCH_TILE_SIZE$tile_size\_DMA$dma" -io_config IOCFG_TILE_SIZE$tile_size\_DMA$dma -argv $DEFAULT_ARGV

    foreach cfg [list BASIC] {
	set cname $cfg\_TILE_SIZE$tile_size\_DMA$dma
	define_hls_config isca_synth $cname -io_config IOCFG_TILE_SIZE$tile_size\_DMA$dma --clock_period=$CLOCK_PERIOD $COMMON_HLS_FLAGS -DHLS_DIRECTIVES_$cfg
	if {$TECH_IS_XILINX == 1} {
	    define_sim_config "$cname\_V" "isca_synth RTL_V $cname" "tb TESTBENCH_TILE_SIZE$tile_size\_DMA$dma" -io_config IOCFG_TILE_SIZE$tile_size\_DMA$dma -argv $DEFAULT_ARGV -verilog_top_modules glbl
	} else {
	    define_sim_config "$cname\_V" "isca_synth RTL_V $cname" "tb TESTBENCH_TILE_SIZE$tile_size\_DMA$dma" -io_config IOCFG_TILE_SIZE$tile_size\_DMA$dma -argv $DEFAULT_ARGV
	}
    }
}
}

#
# Compile Flags
#
set_attr hls_cc_options "$INCLUDES"

#
# Simulation Options
#
use_systemc_simulator incisive
set_attr cc_options "$INCLUDES -DCLOCK_PERIOD=$SIM_CLOCK_PERIOD"
# enable_waveform_logging -vcd
set_attr end_of_sim_command "make saySimPassed"
