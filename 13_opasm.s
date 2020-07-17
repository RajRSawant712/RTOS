; Stop Go C/ASM Mix Example
; Jason Losh

;-----------------------------------------------------------------------------
; Hardware Target
;-----------------------------------------------------------------------------

; Target Platform: EK-TM4C123GXL Evaluation Board
; Target uC:       TM4C123GH6PM
; System Clock:    40 MHz

; Hardware configuration:

;-----------------------------------------------------------------------------
; Device includes, defines, and assembler directives
;-----------------------------------------------------------------------------
	.def waitPbPress
	.def activate_PSP
    .def pushPSP_R4_11
	.def popPSP_R4_11
	.def get_PSP
	.def get_MSP
	.def set_PSP
	.def get_R0
	.def get_R1
	.def moveDummy1R4_11
	.def moveDummy2R4_11
	.def dummyStackOP
	.def setTMPL
;-----------------------------------------------------------------------------
; Register values and large immediate values
;-----------------------------------------------------------------------------

.thumb
.const
GPIO_PORTF_DATA_R       .field   0x400253FC
XPSR_DATA				.field   0x01000000
LC_DATA					.field 	 0xFFFFFFF9

;-----------------------------------------------------------------------------
; Subroutines
;-----------------------------------------------------------------------------

.text

; Blocking function that returns only when SW1 is pressed
waitPbPress:
               LDR    R0, GPIO_PORTF_DATA_R  ; get pointer to port F
               LDR    R0, [R0]               ; read port F
               AND    R0, #0x10              ; mask off all but bit 4
               CBNZ   R0, retry              ; 0 if bit set test (note: only support 0-126 branches)
               BX     LR                     ; return from subroutine
retry:         B      waitPbPress



setTMPL:
			   MOV    R1,#3
			   MSR    CONTROL,R1
			   ISB
               BX     LR                     ; return from subroutine


activate_PSP:
			   MSR    PSP,R0
			   ISB
			   MOV    R1,#2
			   MSR    CONTROL,R1
			   ISB
               BX     LR                     ; return from subroutine

pushPSP_R4_11:
			   MRS    R0,PSP
			   STMDB  R0!,{R4-R11}
			   MSR    PSP,R0
			   BX     LR                     ; return from subroutine (end func)

popPSP_R4_11:
			   LDMIA  R0!,{R4-R11}
			   MSR    PSP,R0
			   BX     LR                     ; return from subroutine (end func)

get_PSP:
			   MRS    R0,PSP
			   BX     LR                     ; return from subroutine (end func)

get_MSP:
			   MRS    R0,MSP
			   BX     LR                     ; return from subroutine (end func)


set_PSP:
			   MSR    PSP,R0
			   BX     LR                     ; return from subroutine (end func)
get_R0:
			   MOV    R4,R1
			   BX     LR					 ; return from subroutine (end func)

get_R1:
			   MOV	  R0,R4
			   BX     LR					 ; return from subroutine (end func)


moveDummy1R4_11:
			   MOV    R4,#4
			   MOV    R5,#5
			   MOV    R6,#6
			   MOV    R7,#7
			   MOV    R8,#8
			   MOV    R9,#9
			   MOV    R10,#10
			   MOV    R11,#11
			   BX     LR					 ; return from subroutine (end func)


moveDummy2R4_11:
			   MOV    R4,#4
			   MOV    R5,#4
			   MOV    R6,#4
			   MOV    R7,#4
			   MOV    R8,#4
			   MOV    R9,#4
			   MOV    R10,#4
			   MOV    R11,#4
			   BX     LR					 ; return from subroutine (end func)

dummyStackOP:								; r0 -psp , r1 - pc , r2 - lr
			 ;todo MRS  MSR    PSP,R0
			   MOV 	  R3, #16777216
			   SUB 	  R0,R0,#4
			;   STR	  R3, [R0]				; store XPSR[R3] val in PSP
			   SUB 	  R0,R0,#4
			 ;  STR 	  R1,[R0]               ; store PC [R1] in psp
			   SUB 	  R0,R0,#4
		;	   MOV 	  R2,#-3
			  ; STR	  R2,[R0]               ; store LR[R2] in psp
			   MOV    R14, R2
			   SUB 	  R0,R0,#4
			   MOV    R3,#3
			   ;STR    R3,[R0]
			   SUB 	  R0,R0,#4
			   MOV    R2,#2
			   ;STR    R2,[R0]
			   SUB 	  R0,R0,#4
			   MOV    R1,#1
			   ;STR    R1,[R0]
			   SUB 	  R0,R0,#4
			   ;STR    R1,[R0]

			   MSR    PSP,R0
			   BX     LR					 ; return from subroutine (end func)


.endm
