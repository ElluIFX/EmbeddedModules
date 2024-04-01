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
	EXTERN  sched_tcb_now
	EXTERN  sched_tcb_next
	
	PUBLIC  PendSV_Handler
	
	SECTION .text:CODE:NOROOT(4)
	
PendSV_Handler:
	CPSID   I
	LDR     R0, =sched_tcb_now
	LDR     R1, [R0]
	CBZ     R1, POPSTACK
	PUSH    {R4-R11}
	STR     SP, [R1]
POPSTACK
	LDR     R2, =sched_tcb_next
	LDR     R3, [R2]
	STR     R3, [R0]
	LDR     SP, [R3]
	POP     {R4-R11}
	CPSIE   I
	BX      LR
	
	END
	
