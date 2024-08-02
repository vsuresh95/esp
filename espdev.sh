#!/bin/bash

unset UART_IP
unset UART_PORT
unset SSH_IP
unset SSH_PORT
unset ESPLINK_IP
unset ESPLINK_PORT
unset LINUX_MAC
unset UART_FILE

COL_CYAN='\033[0;96m'
COL_NORMAL='\033[0;39m'

normal=$(tput sgr0; echo -e $COL_NORMAL)
def=$(tput bold; echo -e $COL_CYAN)

echo "=== SLD FPGAs environment for ESP ==="
echo ""
echo "  [1] - vc707-01"
echo "  [2] - vc707-02"
echo "  [3] - vc707-03"
echo "  [4] - vc707-04"
echo "  [5] - vc707-05"
echo "  [6] - profgpa-01 (bertha)"
echo "  [7] - profgpa-02 (agatha)"
echo "  [8] - profgpa-03 (cortana)"
echo "  [9] - vcu118-01"
echo "  [10] - vcu118-02"
echo "  [11] - vcu128-01"
echo "  [12] - zynqmp04"
echo "  [13] - epochs-0 (bertha)"
echo ""

read -p "  Plase select your target FPGA board (1-10) ${def}[1]${normal}: " opt
opt=${opt:-1}
case $opt in
    1 ) echo "  == vc707-01 =="
	export FPGA_HOST=espdev.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3122
	export UART_IP=espdev.cs.columbia.edu
	export UART_PORT=4322
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5502
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46307
	export LINUX_MAC=000A3502CB80
	;;
    2 ) echo "  == vc707-02 =="
	export FPGA_HOST=espdev.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3123
	export UART_IP=espdev.cs.columbia.edu
	export UART_PORT=4323
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5503
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46308
	export LINUX_MAC=000A3502CB79
	;;
    3 ) echo "  == vc707-03 =="
	export FPGA_HOST=espdev.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3131
	export UART_IP=espdev.cs.columbia.edu
	export UART_PORT=4330
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5507
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46313
	export LINUX_MAC=000A35074FD0
	;;
    4 ) echo "  == vc707-04 =="
	export FPGA_HOST=goliah.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3132
	export UART_IP=goliah.cs.columbia.edu
	export UART_PORT=4332
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5509
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46315
	export LINUX_MAC=000A350300AF
	;;
    5 ) echo "  == vc707-05 =="
	export FPGA_HOST=goliah.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3133
	export UART_IP=goliah.cs.columbia.edu
	export UART_PORT=4333
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5510
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46316
	export LINUX_MAC=000A35075014
	;;
    6 ) echo "  == profpga-01 =="
	export FPGA_HOST=espdev.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3124
	export UART_IP=espdev.cs.columbia.edu
	export UART_PORT=4324
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5504
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46309
	export LINUX_MAC=00006EA1542B
	;;
    7 ) echo "  == profpga-02 =="
	export FPGA_HOST=espdev.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3125
	export UART_IP=espdev.cs.columbia.edu
	export UART_PORT=4325
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5505
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46310
	export LINUX_MAC=00006EA1542C
	;;
    8 ) echo "  == profpga-03 =="
	export FPGA_HOST=espdev.cs.columbia.edu
	export XIL_HW_SERVER_PORT=
	export UART_IP=espdev.cs.columbia.edu
	export UART_PORT=4325
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5511
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46317
	export LINUX_MAC=00006EA1542D
	;;
    9 ) echo "  == vcu118-01 =="
	export FPGA_HOST=espdev.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3126
	export UART_IP=espdev.cs.columbia.edu
	export UART_PORT=4326
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5506
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46311
	export LINUX_MAC=000A3504EFB1
	;;
    10 ) echo "  == vcu118-02 =="
	export FPGA_HOST=espdev.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3132
	export UART_IP=espdev.cs.columbia.edu
	export UART_PORT=4331
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5508
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46314
	export LINUX_MAC=000A350746D0
	export UART_FILE=minicom.log
	;;
    11 ) echo "  == vcu128 =="
	export FPGA_HOST=goliah.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3127
	export UART_IP=goliah.cs.columbia.edu
	export UART_PORT=4327
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5513
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46312
	export LINUX_MAC=000A3504E2F5
	;;
    12 ) echo "  == zynqmp04 =="
	export FPGA_HOST=goliah.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3130
	export UART_IP=goliah.cs.columbia.edu
	export UART_PORT=4328
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5524
	;;
    13 ) echo "  == EPOCHS-0 =="
	export FPGA_HOST=espdev.cs.columbia.edu
	export XIL_HW_SERVER_PORT=3124
	export UART_IP=espdev.cs.columbia.edu
	export UART_PORT=4329
	export SSH_IP=espgate.cs.columbia.edu
	export SSH_PORT=5518
	export ESPLINK_IP="128.59.22.75"
	export ESPLINK_PORT=46319
	export FPGA_PROXY_IP="128.59.22.75"
	export FPGA_PROXY_PORT=46320
	export LINUX_MAC=00006EA2542C
	;;
    * ) echo "  ERROR: invalid selection"
	;;
esac

echo "  FPGA_HOST               : ${FPGA_HOST}"
echo "  XIL_HW_SERVER_PORT      : ${XIL_HW_SERVER_PORT}"
echo "  UART_IP                 : ${UART_IP}"
echo "  UART_PORT               : ${UART_PORT}"
echo "  SSH_IP                  : ${SSH_IP}"
echo "  SSH_PORT                : ${SSH_PORT}"
if [ "${ESPLINK_IP}" == "" ]; then
    echo "  ZYNQ-specific:"
    echo "   ***"
    echo "   SSH info refer to Linux running on the Zynq PS"
    echo "   UART info refer to the ESP instance on the PL (UART2USB channel 2)"
    echo "   There is no Ethernet interface available to ESP on ZYNQ; Run esplink_zynq on the PS to control ESP"
    echo "   ***"
else
    echo "  ESPLINK_IP              : ${ESPLINK_IP}"
    echo "  ESPLINK_PORT            : ${ESPLINK_PORT}"
    echo "  LINUX_MAC               : ${LINUX_MAC}"
fi
echo ""
echo "=== Configuration complete ==="
