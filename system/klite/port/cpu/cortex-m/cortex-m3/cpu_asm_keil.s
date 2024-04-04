	IMPORT  kl_sched_tcb_now
	IMPORT  kl_sched_tcb_next

	EXPORT  PendSV_Handler

	AREA |.text|, CODE, READONLY, ALIGN=2
	PRESERVE8

PendSV_Handler  PROC
	CPSID   I
	LDR     R0, =kl_sched_tcb_now
	LDR     R1, [R0]
	CBZ     R1, POPSTACK
	PUSH    {R4-R11}
	STR     SP, [R1]
POPSTACK
	LDR     R2, =kl_sched_tcb_next
	LDR     R3, [R2]
	STR     R3, [R0]
	LDR     SP, [R3]
	POP     {R4-R11}
	CPSIE   I
	BX      LR
	ALIGN
	ENDP

	END
