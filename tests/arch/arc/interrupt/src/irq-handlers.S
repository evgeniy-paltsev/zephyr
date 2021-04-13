#include <toolchain.h>
#include <arch/cpu.h>
#include <linker/sections.h>

#include "thread-to-irq.h"

GTEXT(irq0_handler)
GTEXT(irq1_handler)
GTEXT(_payload_thread)

/* 32-bit wide */
GDATA(arc_isr0_token)
GDATA(arc_isr1_token)

/* 32-bit wide */
GDATA(arc_irq0_regs_1to12_ok)
GDATA(arc_irq0_regs_13_ok)
GDATA(arc_irq0_regs_14to25_ok)

GDATA(arc_thread_regs_1to12_ok)
GDATA(arc_thread_regs_13_ok)
GDATA(arc_thread_regs_14to25_ok)

/* system-bit wide */
GDATA(arc_thread_sp_before)
GDATA(arc_thread_sp_after)
GDATA(arc_irq0_sp_before)
GDATA(arc_irq0_sp_after)


SECTION_FUNC(TEXT, _payload_thread)

	mov r1,  THREAD_REG_TOKEN(1)
	mov r2,  THREAD_REG_TOKEN(2)
	mov r3,  THREAD_REG_TOKEN(3)
	mov r4,  THREAD_REG_TOKEN(4)
	mov r5,  THREAD_REG_TOKEN(5)
	mov r6,  THREAD_REG_TOKEN(6)
	mov r7,  THREAD_REG_TOKEN(7)
	mov r8,  THREAD_REG_TOKEN(8)
	mov r9,  THREAD_REG_TOKEN(9)
	mov r10, THREAD_REG_TOKEN(10)
	mov r11, THREAD_REG_TOKEN(11)
	mov r12, THREAD_REG_TOKEN(12)
	mov r13, THREAD_REG_TOKEN(13)
	mov r14, THREAD_REG_TOKEN(14)
	mov r15, THREAD_REG_TOKEN(15)
	mov r16, THREAD_REG_TOKEN(16)
	mov r17, THREAD_REG_TOKEN(17)
	mov r18, THREAD_REG_TOKEN(18)
	mov r19, THREAD_REG_TOKEN(19)
	mov r20, THREAD_REG_TOKEN(20)
	mov r21, THREAD_REG_TOKEN(21)
	mov r22, THREAD_REG_TOKEN(22)
	mov r23, THREAD_REG_TOKEN(23)
	mov r24, THREAD_REG_TOKEN(24)
	mov r25, THREAD_REG_TOKEN(25)

	/* log SP before IRQ 0 trigger */
	st sp, [arc_thread_sp_before]

	/* Trigger IRQ 0 */
	mov r0, IRQ0_LINE
	sr r0, [_ARC_V2_AUX_IRQ_HINT]

	nop
	nop
	nop
	nop

	/* log SP after IRQ 0 trigger */
	st sp, [arc_thread_sp_after]

	/* Check r1 to r12 */
	mov r0, THREAD_REG_TOKEN(1)
	brne r0, r1, arc_thread_regs_1to12_nok
	mov r0, THREAD_REG_TOKEN(2)
	brne r0, r2, arc_thread_regs_1to12_nok
	mov r0, THREAD_REG_TOKEN(3)
	brne r0, r3, arc_thread_regs_1to12_nok
	mov r0, THREAD_REG_TOKEN(4)
	brne r0, r4, arc_thread_regs_1to12_nok
	mov r0, THREAD_REG_TOKEN(5)
	brne r0, r5, arc_thread_regs_1to12_nok
	mov r0, THREAD_REG_TOKEN(6)
	brne r0, r6, arc_thread_regs_1to12_nok
	mov r0, THREAD_REG_TOKEN(7)
	brne r0, r7, arc_thread_regs_1to12_nok
	mov r0, THREAD_REG_TOKEN(8)
	brne r0, r8, arc_thread_regs_1to12_nok
	mov r0, THREAD_REG_TOKEN(9)
	brne r0, r9, arc_thread_regs_1to12_nok
	mov r0, THREAD_REG_TOKEN(10)
	brne r0, r10, arc_thread_regs_1to12_nok
	mov r0, THREAD_REG_TOKEN(11)
	brne r0, r11, arc_thread_regs_1to12_nok
	mov r0, THREAD_REG_TOKEN(12)
	brne r0, r12, arc_thread_regs_1to12_nok
	/* Mark as OK */
	st THR_R_1TO12_OK, [arc_thread_regs_1to12_ok]
arc_thread_regs_1to12_nok:

	/* Check r13 */
	mov r0, THREAD_REG_TOKEN(13)
	brne r0, r13, arc_thread_regs_13_nok
	/* Mark as OK */
	st THR_R_13_OK, [arc_thread_regs_13_ok]
arc_thread_regs_13_nok:

	/* Check r14 to r25 */
	mov r0, THREAD_REG_TOKEN(14)
	brne r0, r14, arc_thread_regs_14to25_nok
	mov r0, THREAD_REG_TOKEN(15)
	brne r0, r15, arc_thread_regs_14to25_nok
	mov r0, THREAD_REG_TOKEN(16)
	brne r0, r16, arc_thread_regs_14to25_nok
	mov r0, THREAD_REG_TOKEN(17)
	brne r0, r17, arc_thread_regs_14to25_nok
	mov r0, THREAD_REG_TOKEN(18)
	brne r0, r18, arc_thread_regs_14to25_nok
	mov r0, THREAD_REG_TOKEN(19)
	brne r0, r19, arc_thread_regs_14to25_nok
	mov r0, THREAD_REG_TOKEN(20)
	brne r0, r20, arc_thread_regs_14to25_nok
	mov r0, THREAD_REG_TOKEN(21)
	brne r0, r21, arc_thread_regs_14to25_nok
	mov r0, THREAD_REG_TOKEN(22)
	brne r0, r22, arc_thread_regs_14to25_nok
	mov r0, THREAD_REG_TOKEN(23)
	brne r0, r23, arc_thread_regs_14to25_nok
	mov r0, THREAD_REG_TOKEN(24)
	brne r0, r24, arc_thread_regs_14to25_nok
	mov r0, THREAD_REG_TOKEN(25)
	brne r0, r25, arc_thread_regs_14to25_nok
	/* Mark as OK */
	st THR_R_14TO25_OK, [arc_thread_regs_14to25_ok]
arc_thread_regs_14to25_nok:

	j_s [blink]


SECTION_FUNC(TEXT, irq1_handler)
	mov r0, ISR1_TOKEN
	st r0, [arc_isr1_token]

	mov r1,  IRQ_REG_TOKEN(1, 1)
	mov r2,  IRQ_REG_TOKEN(1, 2)
	mov r3,  IRQ_REG_TOKEN(1, 3)
	mov r4,  IRQ_REG_TOKEN(1, 4)
	mov r5,  IRQ_REG_TOKEN(1, 5)
	mov r6,  IRQ_REG_TOKEN(1, 6)
	mov r7,  IRQ_REG_TOKEN(1, 7)
	mov r8,  IRQ_REG_TOKEN(1, 8)
	mov r9,  IRQ_REG_TOKEN(1, 9)
	mov r10, IRQ_REG_TOKEN(1, 10)
	mov r11, IRQ_REG_TOKEN(1, 11)
	mov r12, IRQ_REG_TOKEN(1, 12)
#ifdef CONFIG_64BIT
	mov r13, IRQ_REG_TOKEN(1, 13)
#endif


;	mov r14, IRQ_REG_TOKEN(1, 14)
;	mov r15, IRQ_REG_TOKEN(1, 15)
;	mov r16, IRQ_REG_TOKEN(1, 16)
;	mov r17, IRQ_REG_TOKEN(1, 17)
;	mov r18, IRQ_REG_TOKEN(1, 18)
;	mov r19, IRQ_REG_TOKEN(1, 19)
;	mov r20, IRQ_REG_TOKEN(1, 20)
;	mov r21, IRQ_REG_TOKEN(1, 21)
;	mov r22, IRQ_REG_TOKEN(1, 22)
;	mov r23, IRQ_REG_TOKEN(1, 23)
;	mov r24, IRQ_REG_TOKEN(1, 24)
;	mov r25, IRQ_REG_TOKEN(1, 25)

	j_s [blink]


SECTION_FUNC(TEXT, irq0_handler)
	mov r0, ISR0_TOKEN
	st r0, [arc_isr0_token]

	/* Save callee manually */
#ifndef CONFIG_64BIT
	push r13
#endif
	push r14
	push r15
	push r16
	push r17
	push r18
	push r19
	push r20
	push r21
	push r22
	push r23
	push r24
	push r25

	mov r1,  IRQ_REG_TOKEN(0, 1)
	mov r2,  IRQ_REG_TOKEN(0, 2)
	mov r3,  IRQ_REG_TOKEN(0, 3)
	mov r4,  IRQ_REG_TOKEN(0, 4)
	mov r5,  IRQ_REG_TOKEN(0, 5)
	mov r6,  IRQ_REG_TOKEN(0, 6)
	mov r7,  IRQ_REG_TOKEN(0, 7)
	mov r8,  IRQ_REG_TOKEN(0, 8)
	mov r9,  IRQ_REG_TOKEN(0, 9)
	mov r10, IRQ_REG_TOKEN(0, 10)
	mov r11, IRQ_REG_TOKEN(0, 11)
	mov r12, IRQ_REG_TOKEN(0, 12)
	mov r13, IRQ_REG_TOKEN(0, 13)
	mov r14, IRQ_REG_TOKEN(0, 14)
	mov r15, IRQ_REG_TOKEN(0, 15)
	mov r16, IRQ_REG_TOKEN(0, 16)
	mov r17, IRQ_REG_TOKEN(0, 17)
	mov r18, IRQ_REG_TOKEN(0, 18)
	mov r19, IRQ_REG_TOKEN(0, 19)
	mov r20, IRQ_REG_TOKEN(0, 20)
	mov r21, IRQ_REG_TOKEN(0, 21)
	mov r22, IRQ_REG_TOKEN(0, 22)
	mov r23, IRQ_REG_TOKEN(0, 23)
	mov r24, IRQ_REG_TOKEN(0, 24)
	mov r25, IRQ_REG_TOKEN(0, 25)

	/* log SP before IRQ 1 trigger */
	st sp, [arc_irq0_sp_before]

	/* Trigger nested (high-priority) IRQ 1 */
	mov r0, IRQ1_LINE
	sr r0, [_ARC_V2_AUX_IRQ_HINT]

	nop
	nop
	nop
	nop

	/* log SP after IRQ 1 trigger */
	st sp, [arc_irq0_sp_after]

	/* Check r1 to r12 */
	mov r0, IRQ_REG_TOKEN(0, 1)
	brne r0, r1, arc_irq0_regs_1to12_nok
	mov r0, IRQ_REG_TOKEN(0, 2)
	brne r0, r2, arc_irq0_regs_1to12_nok
	mov r0, IRQ_REG_TOKEN(0, 3)
	brne r0, r3, arc_irq0_regs_1to12_nok
	mov r0, IRQ_REG_TOKEN(0, 4)
	brne r0, r4, arc_irq0_regs_1to12_nok
	mov r0, IRQ_REG_TOKEN(0, 5)
	brne r0, r5, arc_irq0_regs_1to12_nok
	mov r0, IRQ_REG_TOKEN(0, 6)
	brne r0, r6, arc_irq0_regs_1to12_nok
	mov r0, IRQ_REG_TOKEN(0, 7)
	brne r0, r7, arc_irq0_regs_1to12_nok
	mov r0, IRQ_REG_TOKEN(0, 8)
	brne r0, r8, arc_irq0_regs_1to12_nok
	mov r0, IRQ_REG_TOKEN(0, 9)
	brne r0, r9, arc_irq0_regs_1to12_nok
	mov r0, IRQ_REG_TOKEN(0, 10)
	brne r0, r10, arc_irq0_regs_1to12_nok
	mov r0, IRQ_REG_TOKEN(0, 11)
	brne r0, r11, arc_irq0_regs_1to12_nok
	mov r0, IRQ_REG_TOKEN(0, 12)
	brne r0, r12, arc_irq0_regs_1to12_nok
	/* Mark as OK */
	st IRQ0_R_1TO12_OK, [arc_irq0_regs_1to12_ok]
arc_irq0_regs_1to12_nok:

	/* Check r13 */
	mov r0, IRQ_REG_TOKEN(0, 13)
	brne r0, r13, arc_irq0_regs_13_nok
	/* Mark as OK */
	st IRQ0_R_13_OK, [arc_irq0_regs_13_ok]
arc_irq0_regs_13_nok:

	/* Check r14 to r25 */
	mov r0, IRQ_REG_TOKEN(0, 14)
	brne r0, r14, arc_irq0_regs_14to25_nok
	mov r0, IRQ_REG_TOKEN(0, 15)
	brne r0, r15, arc_irq0_regs_14to25_nok
	mov r0, IRQ_REG_TOKEN(0, 16)
	brne r0, r16, arc_irq0_regs_14to25_nok
	mov r0, IRQ_REG_TOKEN(0, 17)
	brne r0, r17, arc_irq0_regs_14to25_nok
	mov r0, IRQ_REG_TOKEN(0, 18)
	brne r0, r18, arc_irq0_regs_14to25_nok
	mov r0, IRQ_REG_TOKEN(0, 19)
	brne r0, r19, arc_irq0_regs_14to25_nok
	mov r0, IRQ_REG_TOKEN(0, 20)
	brne r0, r20, arc_irq0_regs_14to25_nok
	mov r0, IRQ_REG_TOKEN(0, 21)
	brne r0, r21, arc_irq0_regs_14to25_nok
	mov r0, IRQ_REG_TOKEN(0, 22)
	brne r0, r22, arc_irq0_regs_14to25_nok
	mov r0, IRQ_REG_TOKEN(0, 23)
	brne r0, r23, arc_irq0_regs_14to25_nok
	mov r0, IRQ_REG_TOKEN(0, 24)
	brne r0, r24, arc_irq0_regs_14to25_nok
	mov r0, IRQ_REG_TOKEN(0, 25)
	brne r0, r25, arc_irq0_regs_14to25_nok
	/* Mark as OK */
	st IRQ0_R_14TO25_OK, [arc_irq0_regs_14to25_ok]
arc_irq0_regs_14to25_nok:

	/* Restore callee manually */
	pop r25
	pop r24
	pop r23
	pop r22
	pop r21
	pop r20
	pop r19
	pop r18
	pop r17
	pop r16
	pop r15
	pop r14
#ifndef CONFIG_64BIT
	pop r13
#endif

	j_s [blink]
