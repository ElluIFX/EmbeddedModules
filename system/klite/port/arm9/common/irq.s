;/******************************************************************************
;/* @file   irq.s
;/* @brief  irq mode for ARM9
;/* @date   2022-02-22
;/* @author kerndev@foxmail.com
;/*****************************************************************************/

;/*****************************************************************************/
; Mode bits and interrupt flag (I&F) defines
IRQ_MODE_NO_INT EQU    0xD2
SVC_MODE_NO_INT EQU    0xD3

;/*****************************************************************************/
	PRESERVE8
	AREA |.text|, CODE, READONLY
IRQ_Handler     PROC
	EXPORT IRQ_Handler
	IMPORT intc_handler
	IMPORT sched_tcb_now
	IMPORT sched_tcb_next
	MSR    CPSR_c, #IRQ_MODE_NO_INT
	STMFD  SP!, {R0-R12}    ; PUSH R0-R12
	SUB    R0, LR, #4
	STMFD  SP!, {R0}        ; PUSH PC
	MRS    R0, SPSR
	STMFD  SP!, {R0}        ; PUSH SPSR
	MOV    R12, SP
	ADD    SP, SP, #4*15    ; SP BACK

	MSR    CPSR_c, #SVC_MODE_NO_INT
	LDMFD  R12!, {R11}      ; POP SPSR
	LDMFD  R12!, {R10}      ; POP PC
	STMFD  SP!, {R10}       ; PUSH PC
	STMFD  SP!, {LR}        ; PUSH LR
	LDMFD  R12!, {R0-R7}    ;
	STMFD  SP!, {R0-R7}     ;
	LDMFD  R12!, {R0-R4}    ;
	STMFD  SP!, {R0-R4}     ;
	STMFD  SP!, {R11}       ; PUSH SPSR
	BL     intc_handler

	LDR    R0, =sched_tcb_now
	LDR    R1, [R0]
	LDR    R3, =sched_tcb_next
	LDR    R2, [R3]
	CMP    R1, R2
	BEQ    IRQ_EXIT
	CMP    R1, #0
	STRNE  SP, [R1]
	STR    R2, [R0]
	LDR    SP, [R2]

IRQ_EXIT
	LDMFD  SP!, {R11}
	MSR    SPSR_cxsf, R11
	LDMFD  SP!, {R8-R12}    ; POP R8-R12
	LDMFD  SP!, {R0-R7}     ; POP R0-R7
	LDMFD  SP!, {LR}        ; POP LR
	LDMFD  SP!, {PC}^       ; POP PC
	NOP
	ENDP
	END
