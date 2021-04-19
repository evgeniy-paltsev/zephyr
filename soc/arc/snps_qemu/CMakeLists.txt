# SPDX-License-Identifier: Apache-2.0
zephyr_compile_options(-mcpu=${GCC_M_CPU})

if(CONFIG_64BIT)
  zephyr_compile_options(-ffixed-r13)
else()
  zephyr_compile_options(-mno-sdata)
endif()
