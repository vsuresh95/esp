# Copyright (c) 2011-2021 Columbia University, System Level Design Group
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
define_hls_module tiled_app ../src/tiled_app.cpp


#
# Testbench or system level modules
#
define_system_module tb ../tb/system.cpp ../tb/sc_main.cpp

######################################################################
# HLS and Simulation configurations
######################################################################
set DEFAULT_ARGV ""


set data_width "64"
foreach gr [list cfa ma ] {
foreach inv [list asi] {
foreach dma [list 64] {

    if {$inv eq "reg"} {
        define_io_config * IOCFG_DMA$dma\_INV$inv\_$gr  -DDMA_WIDTH=$dma -DDATA_WIDTH=$data_width
    } else {
    if {$gr eq "cfa"} {
        define_io_config * IOCFG_DMA$dma\_INV$inv\_$gr -DDMA_WIDTH=$dma -DENABLE_SM -DSYNTH_APP_CFA -DDATA_WIDTH=$data_width
    } else {
        define_io_config * IOCFG_DMA$dma\_INV$inv\_$gr  -DDMA_WIDTH=$dma -DENABLE_SM -DDATA_WIDTH=$data_width
    }
    }

    define_system_config tb TESTBENCH_DMA$dma\_INV$inv\_$gr  -io_config IOCFG_DMA$dma\_INV$inv\_$gr

    define_sim_config "BEHAV_DMA$dma\_INV$inv\_$gr " "tiled_app BEH" "tb TESTBENCH_DMA$dma\_INV$inv\_$gr" -io_config IOCFG_DMA$dma\_INV$inv\_$gr  -argv $DEFAULT_ARGV

    foreach cfg [list BASIC] {
	set cname $cfg\_DMA$dma\_INV$inv\_$gr 

    if {$inv eq "reg"} {
	    define_hls_config tiled_app $cname -io_config IOCFG_DMA$dma\_INV$inv\_$gr --clock_period=$CLOCK_PERIOD $COMMON_HLS_FLAGS -DHLS_DIRECTIVES_$cfg -DDATA_WIDTH=$data_width
    } else {
    if {$gr eq "cfa"} {
	    define_hls_config tiled_app $cname -io_config IOCFG_DMA$dma\_INV$inv\_$gr --clock_period=$CLOCK_PERIOD $COMMON_HLS_FLAGS -DHLS_DIRECTIVES_$cfg -DENABLE_SM -DSYNTH_APP_CFA -DDATA_WIDTH=$data_width
    } else {
	    define_hls_config tiled_app $cname -io_config IOCFG_DMA$dma\_INV$inv\_$gr --clock_period=$CLOCK_PERIOD $COMMON_HLS_FLAGS -DHLS_DIRECTIVES_$cfg -DENABLE_SM -DDATA_WIDTH=$data_width
    }
    }
	if {$TECH_IS_XILINX == 1} {
	    define_sim_config "$cname\_V" "tiled_app RTL_V $cname" "tb TESTBENCH_DMA$dma\_INV$inv\_$gr" -io_config IOCFG_DMA$dma\_INV$inv\_$gr -argv $DEFAULT_ARGV -verilog_top_modules glbl
	} else {
	    define_sim_config "$cname\_V" "tiled_app RTL_V $cname" "tb TESTBENCH_DMA$dma\_INV$inv\_$gr" -io_config IOCFG_DMA$dma\_INV$inv\_$gr  -argv $DEFAULT_ARGV
	}
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
