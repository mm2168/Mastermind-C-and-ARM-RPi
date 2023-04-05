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



//======= CONFIG =======
#define DEBUG
#undef ASM_CODE
//======================



//====================== TUNABLES ======================
//Pins based on BCM numbering
#define GLED 13 //Green LED
#define RLED 5 //Red LED

#define BUTTON 19 //Button

//LCD Tunables
#define STRB_PIN	24 //Strobe Control PIN
#define RS_PIN		25 //RS Control PIN
#define DATA0_PIN	23 //4th Data PIN
#define DATA1_PIN	10 //5th Data PIN
#define DATA2_PIN	27 //6th Data PIN
#define DATA3_PIN	22 //7th Data PIN

#define DELAY 200 //Delay of 200 miliseconds -> 0.2 seconds
#define TIMEOUT 3000000 //Delay of 3,000,000 microseconds -> 3 seconds
//=====================================================



//====================== GENERAL CONSTANTS ======================
#define COLORS 3 //Number of colours in the sequence
#define SEQLEN 3 //Length of sequence

#define BUFFER 100 //Buffer value for any arbitrary length

#ifndef TRUE
#define TRUE	(1==1)
#define FALSE	(1==2)
#endif

#define PAGE_SIZE		(4*1024)
#define BLOCK_SIZE		(4*1024)

#define INPUT			0
#define OUTPUT			1

#define LOW			    0
#define HIGH			1

#define GPFSEL0		0
#define GPFSEL1		1
#define GPFSEL2		2
#define GPFSEL3		3
#define GPFSEL4		4
#define GPFSEL5		5
#define GPSET0		7
#define GPSET1		8
#define GPCLR0		10
#define GPCLR1		11

static unsigned int gpiobase;
static uint32_t *gpio;
//===============================================================





/* ====================================================================================================== */
/*				    	                 SECTION: Constants and Prototypes			                	  */
/* ====================================================================================================== */

// char Data for the CGRAM (new characters for the display)
static unsigned char newChar [8] = 
{
  0b11111,
  0b10001,
  0b10001,
  0b10101,
  0b11111,
  0b10001,
  0b10001,
  0b11111,
};

static const int colors = COLORS;
static const int seqlen = SEQLEN;
static char* colorNames[] = { "red" , "green" , "blue" };
static int* theSeq = NULL;
static int *seq1, *seq2;
static int *cpy1, *cpy2;
static int timed_out = 0;
static int countVal = 0;
static int buttPress = 0;
static int waitStatus = 0;

static int lcdControl;
struct lcdStruct {
    int bits;
    int rows;
    int cols;
    int rsPin, strbPin;
    int dataPins[8];
    int cx, cy;
};

//INLINED functions from wiringPI for HD44780U Commands
#define	LCD_CLEAR	        0x01
#define	LCD_HOME	        0x02
#define	LCD_ENTRY	        0x04
#define	LCD_CTRL	        0x08
#define	LCD_CDSHIFT	        0x10
#define	LCD_FUNC	        0x20
#define	LCD_CGRAM	        0x40
#define	LCD_DGRAM	        0x80
//Bits in the Entry Register
#define	LCD_ENTRY_SH		0x01
#define	LCD_ENTRY_ID		0x02
// Bits in the Control Register
#define	LCD_BLINK_CTRL		0x01
#define	LCD_CURSOR_CTRL		0x02
#define	LCD_DISPLAY_CTRL	0x04
// Bits in the Function Register
#define	LCD_FUNC_F	        0x04
#define	LCD_FUNC_N	        0x08
#define	LCD_FUNC_DL	        0x10
#define	LCD_CDSHIFT_RL	    0x04
//GPIO Pins Mask
#define PI_GPIO_MASK	(0xFFFFFFC0)

//=========================== PROTOTYPES ============================
//Hardware Interface Prototypes - '*gpio' is the mmaped GPIO base address
void digitalWrite(uint32_t *gpio, int pin, int value); //----------------------> Send a 'value' on pin number 'pin' - For LCD
void pinMode(uint32_t *gpio, int pin, int mode); //----------------------------> Set the 'mode'as INPUT or OUTPUT on pin number 'pin'
void writeLED(uint32_t *gpio, int led, int value); //--------------------------> Send a 'value' on pin number 'led' - For LED
int readButton(uint32_t *gpio, int button); //---------------------------------> Read a 'value' from pin number 'pin' - For BUTTON
void waitForButton (uint32_t *gpio, int button); //----------------------------> Wait for a button input on pin number 'button'

//Game Logic Prototypes
void initSeq(); //----------------------------------------------------------------------------------------> Initialise the Secret Sequence; Should be random
void showSeq(int *seq); //--------------------------------------------------------------------------------> Display sequence on terminal; Use format in Spec Sheet
int countMatches(int *seq1, int *seq2); //---------------------------------------------------------------> Counts matches between @seq1 and @seq2; Returns pointer to pair of values - [Exact, Approx]
void showMatches(int code, /*DEBUGGING*/int *seq1, int *seq2, /*CONTROL LAYOUT*/int lcd_format); //------> Results from countMatches
void readSeq(int *seq, int val); //-----------------------------------------------------------------------> Parse an integer @value into a list of digits, and store in @seq; 012->[0,1,2]
int* readNum(int max);//----------------------------------------------------------------------------------> Read a number from stdin less than @max; Store values in an array

//Time Code Prototypes
uint64_t timeInMicroseconds();
void timer_handler(int signum);
void initITimer(uint64_t timeout); //----------------> Initialise time-stamps; Setup interval timer; install timer_handler callback

//AUX Functions Prototypes
int failure(int fatal, const char *message, ...);
void waitForEnter(void);
void delay(unsigned int howLong);
void delayMicroseconds(unsigned int howLong);

//LCD Function Prototypes - from wiringPi
void strobe(const struct lcdStruct *lcd); //----------------------------------------------> Toggle the strobe pin to the LCD
void sendDataCmd(const struct lcdStruct *lcd, unsigned char data); //---------------------> Send a data or command byte to the LCD device
void lcdPutCommand(const struct lcdStruct *lcd, unsigned char command); //----------------> Send a command byte to the display
void lcdPut4Command(const struct lcdStruct *lcd, unsigned char command); //---------------> Send a command byte to the display
void lcdHome(struct lcdStruct *lcd); //---------------------------------------------------> Home the cursor
void lcdClear(struct lcdStruct *lcd); //--------------------------------------------------> Clear the screen
void lcdPosition(struct lcdStruct *lcd, int x, int y); //---------------------------------> Update the position of the cursor on the display; Ignore invalid positions
void lcdDisplay(struct lcdStruct *lcd, int state); //-------------------------------------> Turn the display on/off
void lcdCursor(struct lcdStruct *lcd, int state); //--------------------------------------> Turn the cursor on/off
void lcdCursorBlink(struct lcdStruct *lcd, int state); //---------------------------------> Turn the cursor blinking on/off
void lcdPutchar(struct lcdStruct *lcd, unsigned char data); //----------------------------> Send a data byte to the display; Display a character; Line wrap, but no scroll
void lcdPuts(struct lcdStruct *lcd, const char *string); //-------------------------------> Send a string to be displayed on the display

//Helper Function Prototypes
void blinkN(uint32_t *gpio, int led, int c); //-------------------------> Blink the LED on pin @led, @c times
void showMatchesLCD(int matches, struct lcdStruct *lcd);
int* splitDigits(int code);
void showGuess(int colorNum, struct lcdStruct *lcd);
int concat(int x, int y);
void reverse(int arr[], int start, int end);
void showSecretLCD(struct lcdStruct *lcd);
//===================================================================





/* ====================================================================================================== */
/*				    	                    SECTION: Hardware Interface			                		  */
/* ====================================================================================================== */

//INPUT and LOW 	0
//OUTPUT and HIGH	1


/*DIGITAL WRITE in C:
    if(value == HIGH){
	gpio[GPSET0] = 1 << pin;
    }
    else if(value == LOW){
	gpio[GPCLR0] = 1 << pin;
    }
*/
void digitalWrite(uint32_t *gpio, int pin, int value) {
    int off, result;
    off = (value == LOW) ? 10 : 7;

    asm volatile(
        "\tLDR R1, %[gpio]\n"
        "\tADD R0, R1, %[off]\n"
        "\tMOV R2, #1\n"
        "\tMOV R1, %[pin]\n"
        "\tAND R1, #31\n"
        "\tLSL R2, R1\n"
        "\tSTR R2, [R0, #0]\n"
        "\tMOV %[result], R2\n"
        : [result] "=r"(result)
        : [pin] "r"(pin) , [gpio] "m"(gpio) , [off] "r"(off * 4)
        : "r0", "r1", "r2", "cc"
    );
}


/*PIN MODE in C:
    int fSel = pin / 10;
    int shift = (pin % 10) * 3;
    *(gpio + fSel) = (*(gpio + fSel) & ~(7 << shift)) || (mode << shift);
*/
void pinMode(uint32_t *gpio, int pin, int mode) {
    int fSel = pin / 10;
    int shift = (pin % 10) * 3;
    int result;

    if(mode == OUTPUT) {
        asm(
        "\tLDR R1, %[gpio]\n"
        "\tADD R0, R1, %[fSel]\n"
        "\tLDR R1, [R0, #0]\n"
        "\tMOV R2, #0b111\n"
        "\tLSL R2, %[shift]\n"
        "\tBIC R1, R1, R2\n"
        "\tMOV R2, #1\n"
        "\tLSL R2, %[shift]\n"
        "\tORR R1, R2\n"
        "\tSTR R1, [R0, #0]\n"
        "\tMOV %[result], R1\n"
        : [result] "=r"(result)
        : [pin] "r"(pin) , [gpio] "m"(gpio) , [fSel] "r"(fSel * 4), [shift] "r"(shift)
        : "r0", "r1", "r2", "cc"
        );
    }

    else if(mode == INPUT) {
        asm(
        "\tLDR R1, %[gpio]\n"
        "\tADD R0, R1, %[fSel]\n"
        "\tLDR R1, [R0, #0]\n"
        "\tMOV R2, #0b111\n"
        "\tLSL R2, %[shift]\n"
        "\tBIC R1, R1, R2\n"
        "\tSTR R1, [R0, #0]\n"
        "\tMOV %[result], R1\n"
        : [result] "=r"(result)
        : [pin] "r"(pin) , [gpio] "m"(gpio) , [fSel] "r"(fSel * 4), [shift] "r"(shift)
        : "r0", "r1", "r2", "cc"
        );
    }

    else {
        fprintf(stderr, "Invalid Mode");
    }
}


/*WRITE LED in C:
    if(value == HIGH) {
	*(gpio + 7) = 1 << (led & 31);
    }
    else if(value == LOW) {
	*(gpio + 10) = 1 << (led & 31);
    }
*/
void writeLED(uint32_t *gpio, int led, int value) {
    int off, result;
    
    if(value == LOW) {
        off = 10;
        asm volatile(
            "\tLDR R1, %[gpio]\n"
            "\tADD R0, R1, %[off]\n"
            "\tMOV R2, #1\n"
            "\tMOV R1, %[pin]\n"
            "\tAND R1, #31\n"
            "\tLSL R2, R1\n"
            "\tSTR R2, [R0, #0]\n"
            "\tMOV %[result], R2\n"
            : [result] "=r"(result)
            : [pin] "r"(led) , [gpio] "m"(gpio) , [off] "r"(off * 4)
            : "r0", "r1", "r2", "cc"
        );
    } else {
        off = 7;
        asm volatile(
            "\tLDR R1, %[gpio]\n"
            "\tADD R0, R1, %[off]\n"
            "\tMOV R2, #1\n"
            "\tMOV R1, %[pin]\n"
            "\tAND R1, #31\n"
            "\tLSL R2, R1\n"
            "\tSTR R2, [R0,#0]\n"
            "\tMOV %[result], R2\n"
            : [result] "=r"(result)
            : [pin] "r"(led), [gpio] "m"(gpio), [off] "r"(off * 4)
            : "r0", "r1", "r2", "cc"
        );
    }
        
}


/*READ BUTTON in C:
    if( (*(gpio + 13) & (1 << button)) != 0) {
    	if(!buttPress) {
    	    buttPress = 1;
    	    countVal++;
    	    return 1;
    	}
    } else {
    	buttPress = 0;
    }
    return 0;
*/
int readButton(uint32_t *gpio, int button) {
    int state = 0;
    asm volatile(
        "MOV R1, %[gpio]\n"
        "LDR R2, [R1, #0x34]\n"
        "MOV R3, %[pin]\n"
        "MOV R4, #1\n"
        "LSL R4, R3\n"
        "AND %[state], R2, R4\n"
        : [state] "=r"(state)
        : [pin] "r"(button) , [gpio] "r"(gpio)
        : "r0", "r1", "r2", "r3", "r4", "cc"
    );
    
    return state > 0;
}


void waitForButton (uint32_t *gpio, int button) {
    for(int count=0 ; count<13 ; count++) {
        if(readButton(gpio, button)) {
            break;
        }
        delay(DELAY);
    }
}





/* ====================================================================================================== */        
/*				    	                         SECTION: Game Logic			            			  */
/* ====================================================================================================== */

void initSeq() {
    theSeq = (int *)malloc(seqlen * sizeof(int));
    if (theSeq == NULL) {
        printf("Array is null i.e., memory not allocated!");
        exit(0);
    } else {
        for (int i = 0; i < seqlen; ++i) {
            theSeq[i] = rand() % 3 + 1;
        }
    }
}


void showSeq(int *seq) {
    printf("The sequence is: ");
    for (int i = 0; i < seqlen; ++i) {
        switch (theSeq[i]) {
        case 1:
            printf("R ");
            break;
        case 2:
            printf("G ");
            break;
        case 3:
            printf("B ");
            break;
        }
    }
    printf("\n");
}

#define NAN1 8
#define NAN2 9



int countMatches(int *seq1, int *seq2) {
    int *check = (int *)malloc(seqlen * sizeof(int));
    int exact = 0;
    int approx = 0;
    
    for (int checkCount=0 ; checkCount<seqlen ; checkCount++) {
        check[checkCount] = 0;
    }
        
    for (int exactCount=0 ; exactCount<seqlen ; exactCount++) {
        if (seq2[exactCount] == seq1[exactCount]) {
            check[exactCount] = 1;
            exact++;
        }
    }

    for (int outerACount=0 ; outerACount<seqlen ; outerACount++) {
        if (seq2[outerACount] == seq1[outerACount]) {
            continue;
        } else {
            for (int innerACount=0 ; innerACount<seqlen ; innerACount++) {
                if (!check[innerACount] && innerACount!=outerACount && seq2[outerACount]==seq1[innerACount]) {
                    approx++;
                    check[innerACount] = 1;
                    break;
                }
            }
        }
    }

    free(check);

    int ret = concat(exact, approx);
    return ret;
}


void showMatches(int code, int *seq1, int *seq2, int lcd_format) {
    int index = 0;
    int *temp = splitDigits(code);
    int approx = temp[0];
    int correct = temp[1];
    printf("%d exact\n", correct);
    printf("%d approximate\n", approx);
    free(temp);
}


void readSeq(int *seq, int val) {
    int index = 0;

    while (val!=0 && index<seqlen) {
        seq[index] = val % 10;
        ++index;
        val /= 10;
    }

    reverse(seq, 0, seqlen - 1);
}


int *readNum(int max) {
    int index = 0;

    int *arr = (int *)malloc(seqlen * sizeof(int));
    if (!arr) {
        return NULL;
    }

    while (max != 0 && index < SEQLEN) {
        arr[index] = max % 10;
        ++index;
        max /= 10;
    }

    reverse(arr, 0, SEQLEN - 1);

    return arr;
}





/* ====================================================================================================== */
/*				    	                     SECTION: Time Code			                	    		  */
/* ====================================================================================================== */

/* 
The functions in this section are derived from the itimer11.c file from the following link:
    http://www.macs.hw.ac.uk/~hwloidl/Courses/F28HS/srcs/itimer11.c
*/

static uint64_t startT, stopT; //Timestamps for timeout period

uint64_t timeInMicroseconds(){
  struct timeval tv, tNow, tLong, tEnd ;
  uint64_t now ;
  gettimeofday (&tv, NULL) ;
  now  = (uint64_t)tv.tv_sec * (uint64_t)1000000 + (uint64_t)tv.tv_usec ; // in us
}


void timer_handler(int signum) {
    static int count = 0;
    stopT = timeInMicroseconds();
    count++;
    //fprintf(stderr, "Timer expired %d times; (measured interval %f sec)\n", count, (stopT-startT)/1000000.0);
    startT = timeInMicroseconds();
    timed_out = 1;
}


void initITimer(uint64_t timeout) {
    struct sigaction sigAct;
    struct itimerval iTimer;

    /* Install timer_handler as the signal handler for SIGALRM. */
    memset (&sigAct, 0, sizeof(sigAct));
    sigAct.sa_handler = &timer_handler;

    sigaction (SIGALRM, &sigAct, NULL);

    /* Configure the timer to expire after 250 msec... */
    iTimer.it_value.tv_sec = timeout;
    iTimer.it_value.tv_usec = 0;
    /* ... and every 250 msec after that. */
    iTimer.it_interval.tv_sec = 0;
    iTimer.it_interval.tv_usec = 0;
    setitimer (ITIMER_REAL, &iTimer, NULL);
    
    startT = timeInMicroseconds();

    /* Do busy work. */
}





/* ====================================================================================================== */
/*				    	                     SECTION: AUX Functions			            	    		  */
/* ====================================================================================================== */

int failure (int fatal, const char *message, ...) {
    va_list argp ;
    char buffer [1024] ;

    if (!fatal)
        return -1 ;

    va_start (argp, message) ;
    vsnprintf (buffer, 1023, message, argp) ;
    va_end (argp) ;

    fprintf (stderr, "%s", buffer) ;
    exit (EXIT_FAILURE) ;

    return 0 ;
}


void waitForEnter (void) {
    printf ("Press ENTER to continue: ") ;
    (void)fgetc (stdin) ;
}


void delay (unsigned int howLong) {
    struct timespec sleeper, dummy ;

    sleeper.tv_sec  = (time_t)(howLong / 1000) ;
    sleeper.tv_nsec = (long)(howLong % 1000) * 1000000 ;

    nanosleep (&sleeper, &dummy) ;
}


void delayMicroseconds(unsigned int howLong) {
    struct timespec sleeper ;
    unsigned int uSecs = howLong % 1000000 ;
    unsigned int wSecs = howLong / 1000000 ;

        if (howLong ==   0)
            return ;
    #if 0
        else if (howLong  < 100)
            delayMicrosecondsHard (howLong) ;
    #endif
        else {
            sleeper.tv_sec  = wSecs ;
            sleeper.tv_nsec = (long)(uSecs * 1000L) ;
            nanosleep (&sleeper, NULL) ;
        }
}





/* ====================================================================================================== */
/*				    	                     SECTION: LCD Functions			            	    		  */
/* ====================================================================================================== */

void strobe(const struct lcdStruct *lcd) {
    digitalWrite (gpio, lcd->strbPin, 1) ; delayMicroseconds (50) ;
    digitalWrite (gpio, lcd->strbPin, 0) ; delayMicroseconds (50) ;
}


void sendDataCmd(const struct lcdStruct *lcd, unsigned char data) {
    register unsigned char myData = data ;
    unsigned char          i, d4 ;

    if (lcd->bits == 4) {
        d4 = (myData >> 4) & 0x0F;
        for (i = 0 ; i < 4 ; ++i) {
            digitalWrite (gpio, lcd->dataPins [i], (d4 & 1)) ;
            d4 >>= 1 ;
        }
        strobe (lcd) ;

        d4 = myData & 0x0F ;
        for (i = 0 ; i < 4 ; ++i) {
            digitalWrite (gpio, lcd->dataPins [i], (d4 & 1)) ;
            d4 >>= 1 ;
            }
    }
    else {
        for (i = 0 ; i < 8 ; ++i) {
            digitalWrite (gpio, lcd->dataPins [i], (myData & 1)) ;
            myData >>= 1 ;
        }
    }

    strobe (lcd) ;
}


void lcdPutCommand(const struct lcdStruct *lcd, unsigned char command) {
    //#ifdef DEBUG
        //fprintf(stderr, "lcdPutCommand: digitalWrite(%d,%d) and sendDataCmd(%d,%d)\n", lcd->rsPin,   0, lcd, command);
    //#endif
    digitalWrite (gpio, lcd->rsPin,   0) ;
    sendDataCmd  (lcd, command) ;
    delay (2) ;
}


void lcdPut4Command(const struct lcdStruct *lcd, unsigned char command) {
    register unsigned char myCommand = command ;
    register unsigned char i ;

    digitalWrite (gpio, lcd->rsPin,   0) ;

    for (i = 0 ; i < 4 ; ++i) {
        digitalWrite (gpio, lcd->dataPins [i], (myCommand & 1)) ;
        myCommand >>= 1 ;
    }

    strobe (lcd) ;
}


void lcdHome(struct lcdStruct *lcd) {
    #ifdef DEBUG
        fprintf(stderr, "lcdHome: lcdPutCommand(%d,%d)\n", lcd, LCD_HOME);
    #endif
        lcdPutCommand (lcd, LCD_HOME) ;
        lcd->cx = lcd->cy = 0 ;
        delay (5) ;
}


void lcdClear(struct lcdStruct *lcd) {
    //#ifdef DEBUG
        //fprintf(stderr, "lcdClear: lcdPutCommand(%d,%d) and lcdPutCommand(%d,%d)\n", lcd, LCD_CLEAR, lcd, LCD_HOME);
    //#endif
    lcdPutCommand (lcd, LCD_CLEAR) ;
    lcdPutCommand (lcd, LCD_HOME) ;
    lcd->cx = lcd->cy = 0 ;
    delay (5) ;
}


void lcdPosition(struct lcdStruct *lcd, int x, int y) {
    // struct lcdDataStruct *lcd = lcd [fd] ;
    if ((x > lcd->cols) || (x < 0))
        return ;
    if ((y > lcd->rows) || (y < 0))
        return ;

    lcdPutCommand (lcd, x + (LCD_DGRAM | (y>0 ? 0x40 : 0x00)  /* rowOff [y] */  )) ;

    lcd->cx = x ;
    lcd->cy = y ;
}


void lcdDisplay(struct lcdStruct *lcd, int state) {
    if (state)
        lcdControl |=  LCD_DISPLAY_CTRL ;
    else
        lcdControl &= ~LCD_DISPLAY_CTRL ;

    lcdPutCommand (lcd, LCD_CTRL | lcdControl) ; 
}


void lcdCursor(struct lcdStruct *lcd, int state) {
    if (state)
        lcdControl |=  LCD_CURSOR_CTRL ;
    else
        lcdControl &= ~LCD_CURSOR_CTRL ;

    lcdPutCommand (lcd, LCD_CTRL | lcdControl) ; 
}


void lcdCursorBlink(struct lcdStruct *lcd, int state) {
    if (state)
        lcdControl |=  LCD_BLINK_CTRL ;
    else
        lcdControl &= ~LCD_BLINK_CTRL ;

    lcdPutCommand (lcd, LCD_CTRL | lcdControl) ; 
}


void lcdPutchar(struct lcdStruct *lcd, unsigned char data) {
    digitalWrite (gpio, lcd->rsPin, 1) ;
    sendDataCmd  (lcd, data) ;

    if (++lcd->cx == lcd->cols) {
        lcd->cx = 0 ;
        if (++lcd->cy == lcd->rows)
            lcd->cy = 0 ;
    
        //inline computation of address and eliminate rowOff
        lcdPutCommand (lcd, lcd->cx + (LCD_DGRAM | (lcd->cy>0 ? 0x40 : 0x00) /*rowOff [lcd->cy]*/)) ;
    }
}


void lcdPuts(struct lcdStruct *lcd, const char *string) {
    while (*string)
        lcdPutchar (lcd, *string++) ;
}





/* ====================================================================================================== */
/*				    	                  SECTION: Helper Functions			            	    		  */
/* ====================================================================================================== */

void blinkN(uint32_t *gpio, int led, int c) {
    for(int count=0 ; count<c ; count++) {
        writeLED(gpio, led, HIGH);
        delay(700);
        writeLED(gpio, led, LOW);
        delay(700);
    }
}


int* splitDigits(int code) {
    int index = 0;
    int *temp = malloc(2 * sizeof(int));
    while(code != 0 && index < seqlen) {
        temp[index] = code % 10;
        ++index;
        code /= 10;
    }
    
    return temp;
}


void showMatchesLCD(int matches, struct lcdStruct *lcd) {
    int* code = splitDigits(matches);
    int exact = code[1];
    int approx = code[0];
    char *temp = malloc(3);

    lcdPosition(lcd, 0, 1);
        lcdPuts(lcd, "Exact: ");
    lcdPosition(lcd, 6, 1);
        sprintf(temp, "%d", exact);
        lcdPuts(lcd, temp);
    
    blinkN(gpio, GLED, exact);
    
    blinkN(gpio, RLED, 1);
    
    lcdPosition(lcd, 8, 1);
        lcdPuts(lcd, "Approx: ");
    lcdPosition(lcd, 15, 1);
        sprintf(temp, "%d", approx);
        lcdPuts(lcd, temp);
    
    blinkN(gpio, GLED, approx);
    
    free(temp);
}


void showGuess(int colorNum, struct lcdStruct *lcd) {
    switch (colorNum)
    {
    case 1:
        lcdPuts(lcd, " R");
        fprintf(stderr, " R");
        break;
    case 2:
        lcdPuts(lcd, " G");
        fprintf(stderr, " G");
        break;
    case 3:
        lcdPuts(lcd, " B");
        fprintf(stderr, " B");
        break;
    }
}


int concat(int x, int y) {
    int temp = y;
    do
    {
        x *= 10;
        y /= 10;
    } while (y != 0);
    
    return x + temp;
}


void reverse(int arr[], int start, int end) {
    int temp;
    while (start < end)
    {
        temp = arr[start];
        arr[start] = arr[end];
        arr[end] = temp;
        start++;
        end--;
    }
}


void showSecretLCD(struct lcdStruct *lcd) {
    int pos = 5;
    for (int i = 0; i < seqlen; ++i) {
        lcdPosition(lcd, pos, 1);
        switch (theSeq[i]) {
        case 1:
            lcdPuts(lcd, "R ");
            pos += 2;
            break;
        case 2:
            lcdPuts(lcd, "G ");
            pos += 2;
            break;
        case 3:
            lcdPuts(lcd, "B ");
            pos += 2;
            break;
        }
    }
}
