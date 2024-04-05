.syntax unified
.text
.thumb

.extern kl_sched_tcb_now
.extern kl_sched_tcb_next

.global PendSV_Handler

.thumb_func
PendSV_Handler:
	CPSID   I
	LDR     R0, =kl_sched_tcb_now
	LDR     R1, [R0]
	CMP     R1, #0
	BEQ     POPSTACK
	PUSH    {R4-R7}
	MOV     R4, R8
	MOV     R5, R9
	MOV     R6, R10
	MOV     R7, R11
	PUSH    {R4-R7}
	MOV     R2,SP
	STR     R2, [R1]

POPSTACK:
	LDR     R2, =kl_sched_tcb_next
	LDR     R3, [R2]
	STR     R3, [R0]
	LDR     R0, [R3]
	MOV     SP, R0
	POP     {R4-R7}
	MOV     R8, R4
	MOV     R9, R5
	MOV     R10,R6
	MOV     R11,R7
	POP     {R4-R7}
	CPSIE   I
	BX      LR
