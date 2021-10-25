# SPDX-License-Identifier: Apache-2.0

board_runner_args(openocd "--use-elf")
#if(CONFIG_CPU_HAS_MPU)
board_runner_args(openocd --cmd-pre-load "arc jtag set-aux-reg 0x409 0")
board_runner_args(mdb-hw "--jtag=digilent")
include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/mdb-hw.board.cmake)
