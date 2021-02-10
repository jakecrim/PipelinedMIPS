# PipelinedMIPS
This program simulates the functionality of a processor running MIPS assembly instructions. An input file containing a program entirely written in MIPS is the input,
and the C program 'mu-mips.c' will run through the MIPS assembly instructions simulating their functionality.
Not only does it simulate each instruction, but it actually simulates a 4 stage pipeline. Meaning it has to handle the data hazards and branch/jump predictions 
that come along with pipelining.
