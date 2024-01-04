;/******************************************************************************
;/* @file start
;/* @brief  arm9 start code for armcc
;/* @date   2022-02-22
;/* @author kerndev@foxmail.com
;/*****************************************************************************/
Stack_Size      EQU     0x00000400
Heap_Size       EQU     0x00000000

	AREA STACK, NOINIT, READWRITE, ALIGN=3
IRQ_StackMem    SPACE   Stack_Size
IRQ_StackAddr
SVC_StackMem    SPACE   Stack_Size
SVC_StackAddr

	AREA HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

;/*****************************************************************************/
; Vector_Table
	PRESERVE8
	AREA RESET, CODE, READONLY
Vector_Table
	LDR     PC, Reset_Addr
	LDR     PC, Undefined_Addr
	LDR     PC, SWI_Addr
	LDR     PC, Prefetch_Addr
	LDR     PC, Abort_Addr
	LDR     PC, Reserved_Addr
	LDR     PC, IRQ_Addr
	LDR     PC, FIQ_Addr

Reset_Addr      DCD     Reset_Handler
Undefined_Addr  DCD     Reset_Handler
SWI_Addr        DCD     Reset_Handler
Prefetch_Addr   DCD     Reset_Handler
Abort_Addr      DCD     Reset_Handler
Reserved_Addr   DCD     Reset_Handler
IRQ_Addr        DCD     IRQ_Handler
FIQ_Addr        DCD     FIQ_Handler

;/*****************************************************************************/
; Mode bits and interrupt flag (I&F) defines
USR_MODE        EQU    0x10
FIQ_MODE        EQU    0x11
IRQ_MODE        EQU    0x12
SVC_MODE        EQU    0x13
ABT_MODE        EQU    0x17
UDF_MODE        EQU    0x1B
SYS_MODE        EQU    0x1F
I_BIT           EQU    0x80
F_BIT           EQU    0x40

;/*****************************************************************************/
	AREA |.text|, CODE, READONLY
IRQ_Handler        PROC
	EXPORT  IRQ_Handler [WEAK]
	B      .
	ENDP
FIQ_Handler        PROC
	EXPORT  FIQ_Handler [WEAK]
	B      .
	ENDP

;/*****************************************************************************/
; Reset Handler
Reset_Handler PROC
	IMPORT __main
	MSR    CPSR_c, #IRQ_MODE :OR: I_BIT :OR: F_BIT
	LDR    SP, =IRQ_StackAddr
	MSR    CPSR_c, #SVC_MODE :OR: I_BIT :OR: F_BIT
	LDR    SP, =SVC_StackAddr

	LDR    R0, =Vector_Table
	MRC    p15, 0, R2, c1, c0, 0  ;Read CP15 to R2
	ANDS   R2, R2, #(1 << 13)
	LDREQ  R1, =0X00000000
	LDRNE  R1, =0XFFFF0000
	LDMIA  R0!, {R2-R9}
	STMIA  R1!, {R2-R9}
	LDMIA  R0!, {R2-R9}
	STMIA  R1!, {R2-R9}
	LDR    R0, =__main
	BX     R0
	ENDP

;/*****************************************************************************/
; User Initial Stack & Heap
	IF      :DEF:__MICROLIB
	EXPORT  __initial_sp
	EXPORT  __heap_base
	EXPORT  __heap_limit
	ELSE
	IMPORT  __use_two_region_memory
	EXPORT  __user_initial_stackheap

__user_initial_stackheap PROC
	LDR     R0, = Heap_Mem
	LDR     R1, = (SVC_StackMem + Stack_Size)
	LDR     R2, = (Heap_Mem +  Heap_Size)
	LDR     R3, = SVC_StackMem
	BX      LR
	ENDP
	ALIGN

	ENDIF

	END
