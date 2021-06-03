zephyr_system_include_directories(include)

zephyr_library()
zephyr_library_sources(
  arcmwdt-string.c
  libc-hooks.c
)
