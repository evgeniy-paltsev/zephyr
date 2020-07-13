# SPDX-License-Identifier: Apache-2.0

find_program(
  NSIM
  nsimdrv
  )

if(${CONFIG_SOC_NSIM_EM})
 set(NSIM_PROPS nsim_em.props)
elseif(${CONFIG_SOC_NSIM_SEM})
 set(NSIM_PROPS nsim_sem.props)
elseif(${CONFIG_SOC_NSIM_HS})
 set(NSIM_PROPS nsim_hs.props)
elseif(${CONFIG_SOC_NSIM_HS_NIRQ})
 set(NSIM_PROPS nsim_hs_nirq.props)
endif()

if(${CONFIG_SOC_NSIM_HS_NIRQ})
#MDB implementation
#add_custom_target(run
#  COMMAND
#  mdb -arc64 -OK -OKN -nogoifmain -Xatomic -Xtimer0 -Xtimer1 -Xdiv_rem -Xunaligned -prop=nsim_isa_addr_size=64 -prop=nsim_isa_pc_size=64 -prop=nsim_mem-dev=uart0,kind=dwuart,base=0xf0000000,irq=24 -cl -run
#  ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
#  DEPENDS ${logical_target_for_zephyr_elf}
#  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
#  USES_TERMINAL
#  )

#nSIM implementation
add_custom_target(run
  COMMAND
  nsimdrv -p nsim_profiling=0 -p maxlastpc=0 -p trace_enabled=0 -p nsim_isa_family=arc64 -p nsim_isa_core=1 -p arcver=113 -p nsim_isa_atomic_option=1 -p nsim_isa_shift_option=3 -p nsim_isa_code_density_option=2 -p nsim_isa_div_rem_option=2 -p nsim_isa_swap_option=1 -p nsim_isa_bitscan_option=1 -p nsim_isa_enable_timer_0=1 -p nsim_isa_enable_timer_1=1 -p nsim_isa_has_interrupts=1 -p nsim_isa_dual_issue_option=1 -p nsim_isa_div64_option=1 -p nsim_isa_mpy64=1 -p nsim_isa_mpy_option=9 -p nsim_hostlink=1 -p gverbose=0 -p nsim_isa_addr_size=64 -p nsim_isa_pc_size=64 -p nsim_mem-dev=uart0,kind=dwuart,base=0xf0000000,irq=24 -p port=0x378 -- ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  DEPENDS ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  USES_TERMINAL
  )

else()
add_custom_target(run
  COMMAND
  ${NSIM}
  -propsfile
  ${BOARD_DIR}/support/${NSIM_PROPS}
  ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
  DEPENDS ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  USES_TERMINAL
  )

endif()

add_custom_target(debugserver
  COMMAND
  ${NSIM}
  -propsfile
  ${BOARD_DIR}/support/${NSIM_PROPS}
  -gdb -port=3333
  DEPENDS ${logical_target_for_zephyr_elf}
  WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
  USES_TERMINAL
  )
