//======= PACKAGES =======
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
//========================

#define SEQLEN 3

int matches(int *seq1, int *seq2) {
	int exact = 0;
	int approx = 0;
	int matchVal = 0;
	int *check = malloc(SEQLEN * sizeof(int));
	
	asm(
		"\tMOV R0, #0\n"			//exact
		"\tMOV R1, %[seq1]\n"		//seq1 argument
		"\tMOV R2, %[seq2]\n"		//seq2 argument
		"\tMOV R3, %[check]\n"		//Array to check if elements in seq1 and seq2 hav already been matched
		"\tMOV R4, #0\n"			//counter
		
		//EXACT LOOP - Loop to find exact matches
		"exactLoop:\n"
			"\tLDR R5, [R1], #4\n"	//seq1[i]
			"\tLDR R6, [R2], #4\n"	//seq2[i]
			"\tCMP R5, R6\n"		//Compare seq1[i] and seq2[i]
			"\tBNE check\n"			//Move to 'check' if elements not equal
			"\tBL exactVal\n"		//Else go to 'exactVal'
			
		//CHECK - Update check array
		"check:\n"
			"\tMOV R5, #0\n"		//
			"\tSTR R5, [R3]\n"		//
			"\tADD R3, #4\n"		//
			"\tB increment\n"		//
			
		//INCREMENT - Increment counter and check condition of loop
		"increment:\n"
			"\tADD R4, #1\n"		//Increment counter
			"\tCMP R4, #3\n"		//Comparison to check loop condition
			"\tBLT exactLoop\n"		//Go back to 'exactLoop' if condition true
			"\tB exitExact\n"		//Go to 'exitExact' when loop ends
		
		//EXACT VAL - Update 'exact' in R0
		"exactVal:\n"
			"\tADD R0, #1\n"		//
			"\tMOV R5, #1\n"		//
			"\tSTR R5, [R3]\n"		//
			"\tADD R3, #4\n"		//
			"\tBX LR\n"				//
			
		//EXIT EXACT - Store value in R0 to variable in C
		"exitExact:\n"
			"\tMOV %[exact], R0\n"
		
		: [exact] "=r"(exact)
		: [seq1] "r"(seq1), [seq2] "r"(seq2), [check] "r"(check)
		: "r0" , "r1" , "r2" , "r3" , "r4" , "r5" , "r6" , "cc"
	);
	
	
	if(exact != 3) {
		asm(
			"\tMOV R0, #0\n"			//approx --> R0
			"\tMOV R1, %[seq1]\n"		//seq1 argument --> R1
			"\tMOV R2, %[seq2]\n"		//seq2 argument --> R2
			"\tMOV R5, #0\n"			//counter --> R4
			
			//OUTER LOOP
			"outerLoopA:\n"
				"\tADD R5, #1\n"		//Increment counter
				"\tCMP R5, #4\n"		//Compare counter with 4
				"\tBGE exitApprox\n"	//Go to 'exitApprox' if counter >= 4
				"\tLDR R6, [R1], #4\n"	//seq1[i]
				"\tLDR R7, [R2], #4\n"	//seq2[i]
				"\tCMP R6, R7\n"		//Compare seq1[i] and seq2[i]
				"\tBEQ outerLoopA\n"		//Ignore exact match and go to next iteration
				"\tMOV R3, %[check]\n"	//check array --> R3
				"\tMOV R4, %[seq2]\n"	//seq2 --> R7
				"\tMOV R7, #0\n"		//
				
			//INNER LOOP
			"innerLoopA:\n"
				"\tADD R7, #1\n"
				"\tCMP R7, #4\n"
				"\tBGE outerLoopA\n"
				"\tLDR R8, [R3], #4\n"
				"\tADD R4, #4\n"
				"\tCMP R8, #0\n"
				"\tBEQ condition2\n"
				"\tB innerLoopA\n"
			
			//CONDITION 2
			"condition2:\n"
				"\tCMP R5, R7\n"
				"\tBNE condition3\n"
				"\tB innerLoopA\n"
				
			//CONDITION 3
			"condition3:\n"
				"\tSUB R4, #4\n"
				"\tLDR R8, [R4]\n"
				"\tADD R4, #4\n"
				"\tCMP R6, R8\n"
				"\tBEQ approxVal\n"
				"\tB innerLoopA\n"
				
			//APPROX VAL
			"approxVal:\n"
				"\tADD R0, #1\n"
				"\tMOV R8, #1\n"
				"\tSUB R3, #4\n"
				"\tSTR R8, [R3]\n"
				"\tADD R3, #4\n"
				"\tB outerLoopA\n"
			
			//EXIT APPROX
			"exitApprox:\n"
				"\tMOV %[result], R0\n"
			
			: [result] "=r"(approx)
			: [seq1] "r"(seq1), [seq2] "r"(seq2), [check] "r"(check)
			: "r0" , "r1" , "r2" , "r3" , "r4" , "r5" , "r6" , "r7" , "r8" , "cc"
		);
	}
	
	free(check);
	exact *= 10;
	matchVal = exact + approx;
	return matchVal;			
}
			
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
