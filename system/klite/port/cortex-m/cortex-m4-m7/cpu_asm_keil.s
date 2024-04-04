	IMPORT  kl_sched_tcb_now
	IMPORT  kl_sched_tcb_next

	EXPORT  PendSV_Handler

	AREA |.text|, CODE, READONLY, ALIGN=2
	PRESERVE8

PendSV_Handler  PROC
	CPSID       I ; disable interrupt
	LDR         R0, =kl_sched_tcb_now ; R0 = kl_sched_tcb_now
	LDR         R1, [R0] ; R1 = kl_sched_tcb_now
	CBZ         R1, POPSTACK ; if kl_sched_tcb_now == 0, POPSTACK
	IF      {FPU} != "SoftVFP" ; if FPU Enabled
	TST         LR, #0x10 ; if LR[4] == 1 (FPU used in the task)
	VPUSHEQ     {S16-S31} ; save FPU registers to the stack
	ENDIF
	PUSH        {LR,R4-R11} ; save LR, R4-R11
	STR         SP, [R1] ; kl_sched_tcb_now->sp = SP
POPSTACK
	LDR         R2, =kl_sched_tcb_next ; R2 = kl_sched_tcb_next
	LDR         R3, [R2] ; R3 = kl_sched_tcb_next
	STR         R3, [R0] ; kl_sched_tcb_now = kl_sched_tcb_next
	LDR         SP, [R3] ; SP = kl_sched_tcb_next->sp
	POP         {R4-R11} ; restore R4-R11
	IF      {FPU} != "SoftVFP" ; if FPU Enabled
	POP         {LR} ; restore LR
	TST         LR, #0x10 ; if LR[4] == 1 (FPU used in the task)
	VPOPEQ      {S16-S31} ; restore FPU registers from the stack
	ENDIF
	CPSIE       I ; enable interrupt
	BX          LR ; return
	ALIGN
	ENDP

	END
