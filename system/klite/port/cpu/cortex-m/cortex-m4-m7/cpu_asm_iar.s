	EXTERN  kl_sched_tcb_now
	EXTERN  kl_sched_tcb_next

	PUBLIC  PendSV_Handler

	SECTION .text:CODE:NOROOT(4)

PendSV_Handler:
	CPSID   I
	LDR     R0, =kl_sched_tcb_now
	LDR     R1, [R0]
	CBZ     R1, POPSTACK
	TST     LR, #0x10
	IT      EQ
	VPUSHEQ {S16-S31}
	PUSH    {LR}
	PUSH    {R4-R11}
	STR     SP, [R1]
POPSTACK
	LDR     R2, =kl_sched_tcb_next
	LDR     R3, [R2]
	STR     R3, [R0]
	LDR     SP, [R3]
	POP     {R4-R11}
	POP     {LR}
	TST     LR, #0x10
	IT      EQ
	VPOPEQ  {S16-S31}
	CPSIE   I
	BX      LR
	END
