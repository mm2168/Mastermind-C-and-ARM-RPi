



@ I HAVE COMPLETED THE TASK FOR THIS FILE BUT DID NOT USE THIS FILE FOR COMPILATION
@ THE FILE WITH THIS TASK THAT IS USED FOR COMPILATION IS matches.c USING INLINE ASSEMBLER CODE






@ This ARM Assembler code should implement a matching function, for use in the MasterMind program, as
@ described in the CW2 specification. It should produce as output 2 numbers, the first for the
@ exact matches (peg of right colour and in right position) and approximate matches (peg of right
@ color but not in right position). Make sure to count each peg just once!
	
@ Example (first sequence is secret, second sequence is guess):
@ 1 2 1
@ 3 1 3 ==> 0 1
@ You can return the result as a pointer to two numbers, or two values
@ encoded within one number
@
@ -----------------------------------------------------------------------------

.text
@ this is the matching fct that should be called from the C part of the CW	
.global         matches
@ use the name `main` here, for standalone testing of the assembler code
@ when integrating this code into `master-mind.c`, choose a different name
@ otw there will be a clash with the main function in the C code
.global         alg
alg: 
	LDR  R2, =secret	@ pointer to secret sequence
	LDR  R3, =guess		@ pointer to guess sequence

	@ you probably need to initialise more values here
	MOV R6, #0 @ exact
	MOV R4, #0 @ counter
	
	B teste
loope: 
	LDR R0, [R2]		@ Get secret element
	LDR R1, [R3]		@ get guess element
	CMP R0, R1			@ compare elements
	BNE skipe			@ if not equal, move to skipe
	ADD R6, #1			@ increment exact value
	LDR R0, =n			@ set r0 to 0 (null value)
	LDR R0, [R0]
	STR R0, [r2]		@ set secret and guess element to null
	STR R0, [r3]
skipe:
	ADD R2, #4	@ move to next elements
	ADD R3, #4
	ADD R4, #1	@ increment counter
teste:
	LDR R0, =LEN	@ set r0 to length of sequence
	CMP R4, R0		@ check counter has not run off of the end
	BLT loope
	MOV R4, #10
	MUL R6, R6, R4		@ move exact counter to tens column
	PUSH {R6}			@ push exact number to stack
	
	
	MOV R4, #0			@ guess counter
	MOV R5, #0			@ secret counter
	MOV R6, #0			@ approx
	
	LDR  R2, =secret	@ pointer to secret sequence
	LDR  R3, =guess		@ pointer to guess sequence
	
	B testa
loopa:
	LDR R0, [R2]	@ get secret element
	LDR R1, =n
	LDR R1, [R1]	@ set r1 to null
	CMP R0, R1		@ compare secret element to null
	BEQ nullval		@ if null, branch
	LDR R1, [R3]	@ set r1 to guess element
	CMP R0, R1		@ compare secret to guess
	BNE skipa		@ if not equal, move to skipa
	ADD R6, #1		@ increment approx by 1
	LDR R0, =n		@ load null to r0
	LDR R0, [R0]
	STR R0, [r2]	@ set elements to 0 (null value)
	STR R0, [r3]
	B nullval		@ move to nullval, dont bother looping the rest of the guess seq
	
skipa:
	ADD R3, #4	@ move to next guess element
	ADD R4, #1	@ increment counter
	
testa:
	LDR R0, =LEN	@ Check loop counter against seq len
	CMP R4, R0
	BLT loopa
	
nullval:
	MOV R4, #0		@ reset guess counter
	LDR R3, =guess	@ reset guess 
	ADD R2, #4	@ move to next secret element
	ADD R5, #1	@ increment secret counter
	LDR R0, =LEN	@ compare the length
	CMP R5, R0
	BLT loopa
	
	MOV R0, R6		@ move approx to r0
	POP {R6}		@ get exact off the stack
	ADD R0, R6		@ add both together into r0
	
exit:	@MOV	 R0, R4		@ load result to output register
	MOV 	 R7, #1		@ load system call code
	SWI 	 0		@ return this value

@ -----------------------------------------------------------------------------
@ sub-routines

@ this is the matching fct that should be callable from C	
matches:			@ Input: R0, R1 ... ptr to int arrays to match ; Output: R0 ... exact matches (10s) and approx matches (1s) of base COLORS
	PUSH {R5-R8}
	LDR  R7, R0
	LDR  R8, R1
	LDR  R2, R7	@ pointer to secret sequence
	LDR  R3, R8		@ pointer to guess sequence

	@ you probably need to initialise more values here
	MOV R6, #0 @ exact
	MOV R4, #0 @ counter
	
	B teste
loope: 
	LDR R0, [R2]		@ Get secret element
	LDR R1, [R3]		@ get guess element
	CMP R0, R1			@ compare elements
	BNE skipe			@ if not equal, move to skipe
	ADD R6, #1			@ increment exact value
	LDR R0, =n			@ set r0 to 0 (null value)
	LDR R0, [R0]
	STR R0, [r2]		@ set secret and guess element to null
	STR R0, [r3]
skipe:
	ADD R2, #4	@ move to next elements
	ADD R3, #4
	ADD R4, #1	@ increment counter
teste:
	LDR R0, =LEN	@ set r0 to length of sequence
	CMP R4, R0		@ check counter has not run off of the end
	BLT loope
	MOV R4, #10
	MUL R6, R6, R4		@ move exact counter to tens column
	PUSH {R6}			@ push exact number to stack
	
	
	MOV R4, #0			@ guess counter
	MOV R5, #0			@ secret counter
	MOV R6, #0			@ approx
	
	LDR  R2, R7	@ pointer to secret sequence
	LDR  R3, R8	@ pointer to guess sequence
	
	B testa
loopa:
	LDR R0, [R2]	@ get secret element
	LDR R1, =n
	LDR R1, [R1]	@ set r1 to null
	CMP R0, R1		@ compare secret element to null
	BEQ nullval		@ if null, branch
	LDR R1, [R3]	@ set r1 to guess element
	CMP R0, R1		@ compare secret to guess
	BNE skipa		@ if not equal, move to skipa
	ADD R6, #1		@ increment approx by 1
	LDR R0, =n		@ load null to r0
	LDR R0, [R0]
	STR R0, [r2]	@ set elements to 0 (null value)
	STR R0, [r3]
	B nullval		@ move to nullval, dont bother looping the rest of the guess seq
	
skipa:
	ADD R3, #4	@ move to next guess element
	ADD R4, #1	@ increment counter
	
testa:
	LDR R0, =LEN	@ Check loop counter against seq len
	CMP R4, R0
	BLT loopa
	
nullval:
	MOV R4, #0		@ reset guess counter
	LDR R3, R8	@ reset guess 
	ADD R2, #4	@ move to next secret element
	ADD R5, #1	@ increment secret counter
	LDR R0, =LEN	@ compare the length
	CMP R5, R0
	BLT loopa
	
	MOV R0, R6		@ move approx to r0
	POP {R6}		@ get exact off the stack
	ADD R0, R6		@ add both together into r0
	
exit:
	POP {R5-58}
	MOV 	 R7, #1		@ load system call code
	SWI 	 0		@ return this value

@ show the sequence in R0, use a call to printf in libc to do the printing, a useful function when debugging 
showseq: 			@ Input: R0 = pointer to a sequence of 3 int values to show
	@ COMPLETE THE CODE HERE (OPTIONAL)
	
	
@ =============================================================================

.data

@ constants about the basic setup of the game: length of sequence and number of colors	
.equ LEN, 3
.equ COL, 3
.equ NAN1, 8
.equ NAN2, 9

@ a format string for printf that can be used in showseq
f4str: .asciz "Seq:    %d %d %d\n"

@ a memory location, initialised as 0, you may need this in the matching fct
.align 4
n: .word 0x00
	
@ INPUT DATA for the matching function
.align 4
secret: .word 1 
	.word 3
	.word 2

.align 4
guess:	.word 1 
	.word 2 
	.word 2 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 0 1
.align 4
expect: .byte 0
	.byte 1

.align 4
secret1: .word 1 
	 .word 2 
	 .word 3 

.align 4
guess1:	.word 1 
	.word 1 
	.word 2 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 1 1
.align 4
expect1: .byte 1
	 .byte 1

.align 4
secret2: .word 2 
	 .word 3
	 .word 2 

.align 4
guess2:	.word 3 
	.word 3 
	.word 1 

@ Not strictly necessary, but can be used to test the result	
@ Expect Answer: 1 0
.align 4
expect2: .byte 1
	 .byte 0

