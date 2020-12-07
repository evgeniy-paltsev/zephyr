# SPDX-License-Identifier: Apache-2.0

if(COMPILER STREQUAL gcc)
  # GNU compiler options
  zephyr_compile_options(-mcpu=${GCC_M_CPU} -mno-sdata)
  zephyr_compile_options_ifdef(CONFIG_CPU_ARCEM -mmpy-option=wlh1)
  zephyr_compile_options_ifdef(CONFIG_CPU_ARCHS -mmpy-option=plus_qmacw)
  if(CONFIG_CPU_ARCHS)
    zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpud_all)
  else()
    zephyr_compile_options_ifdef(CONFIG_FPU -mfpu=fpuda_all)
  endif()
else()
  # MWDT compiler options
  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_EM -arcv2em -core3 -Xdiv_rem=radix2
			       -Xmpy_option=mpyd -Xbitscan -Xswap -Xbarrel_shifter
			       -Xshift_assist -Xdsp2 -Xdsp_complex
			       -Xdsp_divsqrt=radix2 -Xdsp_itu -Xdsp_accshift=full
			       -Xfpus_div -Xfpu_mac -Xfpuda -Xfpus_mpy_slow
			       -Xfpus_div_slow -Xbitstream -Xtimer0 -Xtimer1)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_SEM -arcv2em -core3 -Xcode_density
			       -Xdiv_rem=radix2 -Xswap -Xbitscan -Xmpy_option=mpyd
			       -Xshift_assist -Xbarrel_shifter -Xdsp2
			       -Xdsp_complex -Xdsp_divsqrt=radix2
			       -Xdsp_accshift=limited -Xtimer0 -Xtimer1
			       -Xsec_timer0 -Xstack_check -Xsec_modes -Xdmac)

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS -arcv2hs -core4 -uarch_rev=1:2
	-Xcode_density -Xatomic -Xll64 -Xunaligned -Xdiv_rem=radix4 -Xswap
	-Xbitscan -Xmpy_option=qmpyh -Xshift_assist -Xbarrel_shifter -Xfpud_div
	-Xfpu_mac -Xtimer0 -Xrtc -Xstack_check -dcache=16384,64,2,a
	-Xstu=3 -Xvdsp3 -Xvec_width=512 -Xvec_max_fetch_size=16 -Xvec_num_slots=3
	-Xvec_super_with_scalar -Xvec_regs=32 -Xvec_num_rd_ports=6 -Xvec_num_acc=8
	-Xvec_num_mpy=2 -Xvec_mpy32 -Xvec_num_alu=3 -Xvec_guard_bit_option=0
	-DVEC_MEM_SYS_BASE_CORE0=0xb1000000 )

  zephyr_compile_options_ifdef(CONFIG_SOC_NSIM_HS_SMP -arcv2hs -core2 -Xatomic
			       -Xll64 -Xdiv_rem=radix4 -Xunaligned -Xcode_density
			       -Xswap -Xbitscan -Xmpy_option=qmpyh -Xshift_assist
			       -Xbarrel_shifter -Xfpud_div -Xfpu_mac -Xrtc
			       -Xtimer0 -Xtimer1)

endif()

zephyr_sources(
  soc.c
  soc_config.c
  )
