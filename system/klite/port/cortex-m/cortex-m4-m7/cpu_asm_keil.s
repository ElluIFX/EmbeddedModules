;/******************************************************************************
;* Copyright (c) 2015-2023 jiangxiaogang<kerndev@foxmail.com>
;*
;* This file is part of KLite distribution.
;*
;* KLite is free software, you can redistribute it and/or modify it under
;* the MIT Licence.
;*
;* Permission is hereby granted, free of charge, to any person obtaining a copy
;* of this software and associated documentation files (the "Software"), to deal
;* in the Software without restriction, including without limitation the rights
;* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
;* copies of the Software, and to permit persons to whom the Software is
;* furnished to do so, subject to the following conditions:
;*
;* The above copyright notice and this permission notice shall be included in all
;* copies or substantial portions of the Software.
;*
;* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
;* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
;* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
;* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
;* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
;* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
;* SOFTWARE.
;******************************************************************************/
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
