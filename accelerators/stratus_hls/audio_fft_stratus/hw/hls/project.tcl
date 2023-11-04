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
define_hls_module audio_fft ../src/audio_fft.cpp


#
# Testbench or system level modules
#
define_system_module tb ../tb/system.cpp ../tb/sc_main.cpp

######################################################################
# HLS and Simulation configurations
######################################################################
set DEFAULT_ARGV ""

set FX_IL "-DFX32_IL=14 -DFX64_IL=42"

# Baseline acc - no ASI
foreach dma [list 64] {
    define_io_config * IOCFG_DMA$dma\_BASELINE -DDMA_WIDTH=$dma -DFX_WIDTH=32

    define_system_config tb TESTBENCH_DMA$dma\_BASELINE -io_config IOCFG_DMA$dma\_BASELINE

    define_sim_config "BEHAV_DMA$dma\_BASELINE" "audio_fft BEH" "tb TESTBENCH_DMA$dma\_BASELINE" -io_config IOCFG_DMA$dma\_BASELINE -argv $DEFAULT_ARGV

    foreach cfg [list BASIC] {
	set cname $cfg\_DMA$dma\_BASELINE
	define_hls_config audio_fft $cname -io_config IOCFG_DMA$dma\_BASELINE --clock_period=$CLOCK_PERIOD $COMMON_HLS_FLAGS -DHLS_DIRECTIVES_$cfg
	if {$TECH_IS_XILINX == 1} {
	    define_sim_config "$cname\_V" "audio_fft RTL_V $cname" "tb TESTBENCH_DMA$dma\_BASELINE" -io_config IOCFG_DMA$dma\_BASELINE -argv $DEFAULT_ARGV -verilog_top_modules glbl
	} else {
	    define_sim_config "$cname\_V" "audio_fft RTL_V $cname" "tb TESTBENCH_DMA$dma\_BASELINE" -io_config IOCFG_DMA$dma\_BASELINE -argv $DEFAULT_ARGV
	}
    }
}

# With ASI
foreach dma [list 64] {
    define_io_config * IOCFG_DMA$dma\_SM -DDMA_WIDTH=$dma -DENABLE_SM -DFX_WIDTH=32

    define_system_config tb TESTBENCH_DMA$dma\_SM -io_config IOCFG_DMA$dma\_SM

    define_sim_config "BEHAV_DMA$dma\_SM" "audio_fft BEH" "tb TESTBENCH_DMA$dma\_SM" -io_config IOCFG_DMA$dma\_SM -argv $DEFAULT_ARGV

    foreach cfg [list BASIC] {
	set cname $cfg\_DMA$dma\_SM
	define_hls_config audio_fft $cname -io_config IOCFG_DMA$dma\_SM --clock_period=$CLOCK_PERIOD $COMMON_HLS_FLAGS -DHLS_DIRECTIVES_$cfg
	if {$TECH_IS_XILINX == 1} {
	    define_sim_config "$cname\_V" "audio_fft RTL_V $cname" "tb TESTBENCH_DMA$dma\_SM" -io_config IOCFG_DMA$dma\_SM -argv $DEFAULT_ARGV -verilog_top_modules glbl
	} else {
	    define_sim_config "$cname\_V" "audio_fft RTL_V $cname" "tb TESTBENCH_DMA$dma\_SM" -io_config IOCFG_DMA$dma\_SM -argv $DEFAULT_ARGV
	}
    }
}

# With ASI and internal pipelining
foreach dma [list 64] {
    define_io_config * IOCFG_DMA$dma\_PP -DDMA_WIDTH=$dma -DENABLE_PP -DFX_WIDTH=32

    define_system_config tb TESTBENCH_DMA$dma\_PP -io_config IOCFG_DMA$dma\_PP

    define_sim_config "BEHAV_DMA$dma\_PP" "audio_fft BEH" "tb TESTBENCH_DMA$dma\_PP" -io_config IOCFG_DMA$dma\_PP -argv $DEFAULT_ARGV

    foreach cfg [list BASIC] {
	set cname $cfg\_DMA$dma\_PP
	define_hls_config audio_fft $cname -io_config IOCFG_DMA$dma\_PP --clock_period=$CLOCK_PERIOD $COMMON_HLS_FLAGS -DHLS_DIRECTIVES_$cfg
	if {$TECH_IS_XILINX == 1} {
	    define_sim_config "$cname\_V" "audio_fft RTL_V $cname" "tb TESTBENCH_DMA$dma\_PP" -io_config IOCFG_DMA$dma\_PP -argv $DEFAULT_ARGV -verilog_top_modules glbl
	} else {
	    define_sim_config "$cname\_V" "audio_fft RTL_V $cname" "tb TESTBENCH_DMA$dma\_PP" -io_config IOCFG_DMA$dma\_PP -argv $DEFAULT_ARGV
	}
    }
}

#
# Compile Flags
#
set_attr hls_cc_options "$INCLUDES $FX_IL"

#
# Simulation Options
#
use_systemc_simulator xcelium
set_attr cc_options "$INCLUDES $FX_IL -DCLOCK_PERIOD=$SIM_CLOCK_PERIOD -std=gnu++11"
# enable_waveform_logging -vcd
set_attr end_of_sim_command "make saySimPassed"
