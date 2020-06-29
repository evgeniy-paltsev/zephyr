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
add_custom_target(run
  COMMAND
  mdb -arc64 -OK -OKN -nogoifmain -Xatomic -Xtimer0 -Xtimer1 -Xdiv_rem -prop=nsim_isa_addr_size=64 -prop=nsim_isa_pc_size=64 -prop=nsim_mem-dev=uart0,base=0xf0000000,irq=24 -cl -run
  ${APPLICATION_BINARY_DIR}/zephyr/${KERNEL_ELF_NAME}
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
