# Mastermind in C and ARM using the Raspberry Pi
This program uses the Raspberry Pi along with 2 LEDs, a Button, and an LCD Display to implement the functionality of the game Mastermind written in C and inline ARM Assembly.
___
The program has multiple run options:

> 	sudo ./cw2

This runs the program normally on all the hardware on the Raspberry Pi
____

> sudo ./cw2 -h

This is the help option of the program
___
> sudo ./cw2 -v -d

This is the debugging option of the program. This implements the Mastermind game logic in the command-line.
___
> sudo ./cw2 -v -u seq1 seq2

This option deploys a unit test for the matches function to get the matches between `<seq1>` and `<seq2>` 
___
> sudo ./cw2 -s seq

This option runs the program in any specified mode (normal or debugging) using a secret sequence defined in `<seq>`
___
