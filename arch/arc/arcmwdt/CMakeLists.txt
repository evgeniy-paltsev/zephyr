# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL arcmwdt)
  if(CONFIG_ARCMWDT_LIBC OR CONFIG_CPLUSPLUS)
    zephyr_library_sources(arcmwdt-dtr-stubs.c)
  endif()
endif()
