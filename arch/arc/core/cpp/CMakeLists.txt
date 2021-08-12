# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL arcmwdt)
  if(NOT CONFIG_ARCMWDT_LIBC AND CONFIG_CPLUSPLUS)
    zephyr_library_sources(cpp-mwdt-stubs.c)
  endif()
endif()
