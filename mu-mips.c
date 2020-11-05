#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		CURRENT_STATE = NEXT_STATE;
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		cycle();
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("# Cycles Executed\t: %u\n", CYCLE_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll(); 
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(CURRENT_STATE.PC); 
			break;
		case 'F':
		case 'f':
			if(scanf("%d", &ENABLE_FORWARDING) != 1)
				break;
			ENABLE_FORWARDING == 0 ? printf("Forwarding OFF\n") : printf("Forwarding ON\n");
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */ 
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */	
	printf("|		cycle			|\n");
	printf("*******************\n");	
	WB();
	printf("*******************\n");	
	MEM();
	printf("*******************\n");	
	EX();
	printf("*******************\n");	
	ID();
	printf("*******************\n");	
	IF();
	printf("*******************\n");	
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB()
{
	/*IMPLEMENT THIS*/
	printf("-Write Back- \n");	
	uint32_t rt, rd, opcode;
	// getting necessary pieces of the instruction for execution
	opcode = (MEM_WB.IR & 0xFC000000) >> 26;
	rt = (MEM_WB.IR & 0x001F0000) >> 16;
	rd = (MEM_WB.IR & 0x0000F800) >> 11;

	//simulating same cycle writeback capability 
	writeBackValue = MEM_WB.ALUOutput;

				printf("ALUOUTPUT: 0x%08X \n", MEM_WB.ALUOutput);

	if(MEM_WB.IR != 0)
	{
		// If opcode = 0, its a register register writeback scenario
		if(opcode == 0x0)
		{
			printf("register to register writeback \n");
			NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
		}
		else
		{
			switch (opcode)
			{
				case 0x08:  // ADDI
					NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
					printf("ADDI WRITEBACK \n");
					break;
				case 0x09: //ADDIU
					NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
					printf("ADDIU WRITEBACK \n");
					printf("ALUOUTPUT: 0x%08X \n", MEM_WB.ALUOutput);
					printf("Writing 0x%08X to register 0x%08X \n", MEM_WB.ALUOutput, rt);
					break;
				case 0x0E: //XORI
					NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
					printf("XORI WRITEBACK \n");
					break;
				case 0x0F: //LUI 
					NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
					printf("LUI WRITEBACK \n");
					break;
				case 0x23: //LW
					NEXT_STATE.REGS[rt] = MEM_WB.LMD;
					writeBackValue = MEM_WB.LMD; //simulating same cycle writeback capability with LW
					printf("LW WRITEBACK \n");
					break;

			}
		}
	}

}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
	/*IMPLEMENT THIS*/
	printf("-Memory Access- \n");

	// Retrieve the incomming instruction's REG_WRITE status
	// 	before it is set back to true by defaultin EX() stage	
	REG_WRITE_MEM_WB = REG_WRITE_EX_MEM;

	MEM_WB.IR = EX_MEM.IR;
	MEM_WB.A = EX_MEM.A;
	MEM_WB.B = EX_MEM.B;
	MEM_WB.ALUOutput = EX_MEM.ALUOutput;



	if(EX_MEM.loadFlag)
	{
		printf("Memory Load \n");
		MEM_WB.LMD = mem_read_32(EX_MEM.ALUOutput);
		printf("MEMREAD(): 0x%08X \n", mem_read_32(EX_MEM.ALUOutput));
		printf("MEM_WB.LMD: 0x%08X \n", MEM_WB.LMD);
	}
	else if(EX_MEM.storeFlag)
	{
		printf("Memory Store \n");
		mem_write_32(EX_MEM.ALUOutput, EX_MEM.B);
	}
	

}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
	/*IMPLEMENT THIS*/
	printf("-Execution- \n");

	// only retreive new instruction if we aren't stalling
	EX_MEM.IR = ID_EX.IR;
		
	EX_MEM.A = ID_EX.A;
	EX_MEM.B = ID_EX.B;

	uint32_t opcode, function;

	// set to false by default 
	EX_MEM.loadFlag = false;
	EX_MEM.storeFlag = false;

	// set true by default
	REG_WRITE_EX_MEM = true;

	// getting necessary pieces of the instruction for execution
	opcode = (ID_EX.IR & 0xFC000000) >> 26;
	function = ID_EX.IR & 0x0000003F;


	if(opcode == 0x00)
	{
		printf("Function Code: 0x%08X \n", function);
		switch(function)
		{
			case 0x20: // ADD
				EX_MEM.ALUOutput = ID_EX.A + ID_EX.B;
				printf("ADD Result: 0x%08X \n", EX_MEM.ALUOutput);
				break;
			case 0x24: // AND
				EX_MEM.ALUOutput = ID_EX.A & ID_EX.B;
				printf("AND Result: 0x%08X \n", EX_MEM.ALUOutput);
				break;
			case 0x25: //OR
				EX_MEM.ALUOutput = ID_EX.A | ID_EX.B;
				printf("OR Result: 0x%08X \n", EX_MEM.ALUOutput);
				break;
			case 0x26: // XOR
				EX_MEM.ALUOutput = ID_EX.A ^ ID_EX.B;
				printf("XOR Result: 0x%08X \n", EX_MEM.ALUOutput);
				break;	
			case 0x0C: // SYSCALL
				printf("SYSCALL \n");
				RUN_FLAG = false;
				REG_WRITE_EX_MEM = false;
				break;	
		}
	}
	else
	{
		switch(opcode)
		{
			case 0x08:  // ADDI
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);
				break;
			case 0x09: //ADDIU
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);
				break;
			case 0x0E: //XORI
				EX_MEM.ALUOutput = ID_EX.A ^ (ID_EX.imm & 0x0000FFFF);
				break;
			case 0x0F: //LUI 
				EX_MEM.ALUOutput = ID_EX.imm << 16;
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);
				break;
			case 0x23: //LW
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				printf("Base: 0x%08x \n", ID_EX.A);
				EX_MEM.loadFlag = true;
				break;
			case 0x2B: //SW
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);
				EX_MEM.storeFlag = true;
				REG_WRITE_EX_MEM = false;
				break;
		}
	}

	

}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{
/*IMPLEMENT THIS*/
	printf("-Instruction Decode- \n");

	uint32_t rs = 0, rt = 0, opcode_EX_MEM = 0, opcode_MEM_WB = 0, rd_EX_MEM = 0, rd_MEM_WB = 0, immediate = 0, opcode = 0;

	if(stallCounter != 0)
		stallCounter--;

	/* STALLING SOLUTION WHEN FORWARDING DISABLED */
	if(!ENABLE_FORWARDING)
	{
		printf("Forwarding NOT Enabled \n");
		if(stallCounter == 0)
		{
			ID_EX.IR = IF_ID.IR;
			printf("- DEBUG PRINTS - \n");
			printf("Instruction ID: 0x%08X \n", IF_ID.IR);
			rs = (IF_ID.IR & 0x03E00000) >> 21;
			rt = (IF_ID.IR & 0x001F0000) >> 16;
			// opcode = (ID_EX.IR & 0xfc000001) >> 26;
			opcode = (IF_ID.IR & 0xFC000000) >> 26;

			immediate = IF_ID.IR & 0x0000FFFF;

			// used for finding data hazards
			opcode_EX_MEM = (EX_MEM.IR & 0xFC000000) >> 26;
			opcode_MEM_WB = (MEM_WB.IR & 0xFC000000) >> 26;

			// If its an immediate instruction, "rd" is really rt 
			if(opcode_EX_MEM == 0x0)
			{
				rd_EX_MEM = (EX_MEM.IR & 0x0000F800) >> 11;
			}
			else
			{
				rd_EX_MEM = (EX_MEM.IR & 0x001F0000) >> 16;
			}

			if(opcode_MEM_WB == 0x0)
			{
				rd_MEM_WB = (MEM_WB.IR & 0x0000F800) >> 11;
			}
			else
			{
				rd_MEM_WB = (MEM_WB.IR & 0x001F0000) >> 16;
			}



			/* DATA HAZARD CHECK*/
			// 1 instruction before
			if((REG_WRITE_EX_MEM != 0) && (rd_EX_MEM != 0))
			{
				// if the destination register of ex_mem is the same as the current 
				// rs or rt register then we have a data hazard
				printf("rd_EX_MEM: 0x%08X \n", rd_EX_MEM);
				printf("rd_MEM_WB: 0x%08X \n", rd_MEM_WB);
				printf("rs: 0x%08X \n", rs);
				printf("rt: 0x%08X \n", rt);
				if(rd_EX_MEM == rs)
				{
					// stall twice
					rsHazardType1 =  true;
					stallCounter = 2;
				}
				// if(rd_EX_MEM == rt) AND this is a register - register instruction, OR a load or store instrucion
				 		// otherwise we don't care if rt finds a match with immediate instructions) 
				// if((rd_EX_MEM == rt) && (opcode == 0x0))
				if((rd_EX_MEM == rt) && ((0x0F < opcode) || opcode == 0x0))
				{
					// stall twice
					rtHazardType1 = true;
					stallCounter = 2;
				}
			}
			// 2 instructions before
			if((REG_WRITE_MEM_WB != 0) && (rd_MEM_WB != 0))
			{
				// if the destination register of mem_wb is the same as the current 
				// rs or rt register then we have a data hazard 
				if(rd_MEM_WB == rs)
				{
					// stall once if stallCounter equal 0, if its already 2 then keep it at 2!!!
					rsHazardType2 = true;
					if(stallCounter == 0)
						stallCounter = 1;
				}
				
				// if((rd_MEM_WB == rt) && (opcode == 0x0))
				// Again, excluding immediate instructions, as we dont care if there is an rt match with immediate instructions
				if((rd_MEM_WB == rt) && ((0x0F < opcode) || opcode == 0x0))
				{
					// stall once if stallCounter equal 0, if its already 2 then keep it at 2!!!
					rtHazardType2 = true;
					if(stallCounter == 0)
						stallCounter = 1;
				}
			}

			// If there is no data hazard, pass on the register readings and immediate 
			if(stallCounter == 0)
			{
				ID_EX.A = CURRENT_STATE.REGS[rs]; 
				ID_EX.B = CURRENT_STATE.REGS[rt];

				// if we are one cycle after the stall counter is done
				// 	we have to simulate having access to the WB() stage data through the 
				// 		writeBackValue variable, the rs or rt hazard still needs handled
				// 			this first cycle after being done stalling...
				if(rsHazardType1 || rtHazardType1 || rsHazardType2 || rtHazardType2)
				{
					if(rsHazardType1 || (rsHazardType2 && !rtHazardType1))
					{
						printf("WriteBack into A 0x%08X \n", writeBackValue);
						// ID_EX.A = writeBackValue;
						ID_EX.A = NEXT_STATE.REGS[rs]; 
						rsHazardType1 = false;
						rsHazardType2 = false;
					}
					if(rtHazardType1 || (rtHazardType2 && !rsHazardType1))
					{
						printf("WriteBack into B 0x%08X \n", writeBackValue);
						// ID_EX.B = writeBackValue;
						ID_EX.B = NEXT_STATE.REGS[rt]; 
						rtHazardType1 = false;
						rtHazardType2 = false;
					}
				}

				printf("rs: 0x%08X \n", rs);
				printf("rt: 0x%08X \n", rt);
				printf("reg file print rs: 0x%08X \n", CURRENT_STATE.REGS[rs]);
				printf("reg file print rt: 0x%08X \n", CURRENT_STATE.REGS[rt]);

				// sign extend immediate
				// if the 16th bit is set, sign extend	
				if( immediate & 0x00008000)
				{
					printf("SET \n");
					ID_EX.imm = immediate | 0xFFFF0000;
				}
				else
					ID_EX.imm = immediate;

			}
			// otherwise only pass on zeros
			else
			{
				printf("Hazard Detected \n");
				ID_EX.IR = 0;
				ID_EX.A = 0;
				ID_EX.B = 0;
				ID_EX.imm = 0;
			}

		}
	}


	/* FORWARDING SECTION*/
	if(ENABLE_FORWARDING)
	{
		bool forwardFlag = false;

		ID_EX.IR = IF_ID.IR;
		rs = (IF_ID.IR & 0x03E00000) >> 21;
		rt = (IF_ID.IR & 0x001F0000) >> 16;

		// opcode = (ID_EX.IR & 0xfc000001) >> 26;
		opcode = (IF_ID.IR & 0xFC000000) >> 26;

		immediate = IF_ID.IR & 0x0000FFFF;

		// used for finding data hazards
		opcode_EX_MEM = (EX_MEM.IR & 0xFC000000) >> 26;
		opcode_MEM_WB = (MEM_WB.IR & 0xFC000000) >> 26;
		// If its an immediate instruction, "rd" is really rt 
		if(opcode_EX_MEM == 0x0)
		{
			rd_EX_MEM = (EX_MEM.IR & 0x0000F800) >> 11;
		}
	 	//really rt_EX_MEM
		else
		{
			rd_EX_MEM = (EX_MEM.IR & 0x001F0000) >> 16;
		}

		if(opcode_MEM_WB == 0x0)
		{
			rd_MEM_WB = (MEM_WB.IR & 0x0000F800) >> 11;
		}
		else
		{
			rd_MEM_WB = (MEM_WB.IR & 0x001F0000) >> 16;
		}

		// Normal Case (Lab 3 stuff)
		ID_EX.A = CURRENT_STATE.REGS[rs]; 
		ID_EX.B = CURRENT_STATE.REGS[rt];

		// sign extend immediate
		// if the 16th bit is set, sign extend	
		if( immediate & 0x00008000)
		{
			printf("SET \n");
			ID_EX.imm = immediate | 0xFFFF0000;
		}
		else
			ID_EX.imm = immediate;


		// FORWARDING SECTION
		//forward from EX stage									//rs_ID_EX
		if((REG_WRITE_EX_MEM != 0) && (rd_EX_MEM != 0) && (rd_EX_MEM == rs))
		{	
			//Forward A = 0x10
			ID_EX.A = EX_MEM.ALUOutput;
			printf("rs-rd collision from EX_MEM \n");
			printf(" %08x \n", EX_MEM.ALUOutput);
			printf(" %08x \n", ID_EX.A);

			forwardFlag = true;

		}							   							//rt_ID_EX
		if((REG_WRITE_EX_MEM != 0) && (rd_EX_MEM != 0) && (rd_EX_MEM == rt))
		{
			if((0x0F < opcode) || (opcode == 0x0))
			{
				//ForwardB = 0x10
				ID_EX.B = EX_MEM.ALUOutput;
				printf("rt-rd collision from EX_MEM \n");
				printf(" %08x \n", EX_MEM.ALUOutput);

				forwardFlag = true;

			}
		}

		//forward from MEM stage											//rs_ID_EX		//rs_ID_EX
		if((REG_WRITE_MEM_WB != 0) && (rd_MEM_WB != 0) && !((REG_WRITE_EX_MEM != 0) && (rd_EX_MEM != 0) && (rd_EX_MEM == rs)) && (rd_MEM_WB == rs))
		{
			//ForwardA = 0x01
			ID_EX.A = MEM_WB.ALUOutput;
			//might need to stall for WB
			printf("rs-rd collision from MEM_WB \n");

			forwardFlag = true;
		}														//rt_ID_EX		//rt_ID_EX
		if((REG_WRITE_MEM_WB != 0) && (rd_MEM_WB != 0) && !((REG_WRITE_EX_MEM != 0) && (rd_EX_MEM != 0) && (rd_EX_MEM == rt)) && (rd_MEM_WB == rt))
		{
			if((0x0F < opcode) || (opcode == 0x0))
			{
				//ForwardB = 0x01
				ID_EX.B = MEM_WB.ALUOutput;
				printf("rt-rd collision from MEM_WB \n");

				forwardFlag = true;

			}
		}

		if(forwardFlag)
		{
			printf("Forwarding... \n");
			forwardFlag = false;
		}
		// otherwise only pass on zeros
		//
		if(stallCounter != 0 )
		{
			printf("Hazard Detected \n");
			ID_EX.IR = 0;
			ID_EX.A = 0;
			ID_EX.B = 0;
			ID_EX.imm = 0;

		}
	}


}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{
	if(stallCounter == 0)
	{
		IF_ID.IR = mem_read_32(CURRENT_STATE.PC);
		IF_ID.PC = CURRENT_STATE.PC + 4;
		printf("Current Instruction: 0x%08X \n", IF_ID.IR);

		// increment PC?
		NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	}
	// else
	// {
	// 	stallCounter--;
	// 	if(stallCounter == 0)
	// 	{
	// 		printf("~done stalling ~ \n");
	// 		ID_EX.hazardFlag = false;
	// 	}
	// 	else
	// 		printf("~stalling~ \n");
	// }
	
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(uint32_t addr){
	/*IMPLEMENT THIS*/
	uint32_t instruction, opcode, function, rs, rt, rd, sa, immediate, target;
	
	instruction = mem_read_32(addr);
	
	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	rd = (instruction & 0x0000F800) >> 11;
	sa = (instruction & 0x000007C0) >> 6;
	immediate = instruction & 0x0000FFFF;
	target = instruction & 0x03FFFFFF;
	
	if(opcode == 0x00){
		/*R format instructions here*/
		
		switch(function){
			case 0x00:
				printf("SLL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x02:
				printf("SRL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x03:
				printf("SRA $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x08:
				printf("JR $r%u\n", rs);
				break;
			case 0x09:
				if(rd == 31){
					printf("JALR $r%u\n", rs);
				}
				else{
					printf("JALR $r%u, $r%u\n", rd, rs);
				}
				break;
			case 0x0C:
				printf("SYSCALL\n");
				break;
			case 0x10:
				printf("MFHI $r%u\n", rd);
				break;
			case 0x11:
				printf("MTHI $r%u\n", rs);
				break;
			case 0x12:
				printf("MFLO $r%u\n", rd);
				break;
			case 0x13:
				printf("MTLO $r%u\n", rs);
				break;
			case 0x18:
				printf("MULT $r%u, $r%u\n", rs, rt);
				break;
			case 0x19:
				printf("MULTU $r%u, $r%u\n", rs, rt);
				break;
			case 0x1A:
				printf("DIV $r%u, $r%u\n", rs, rt);
				break;
			case 0x1B:
				printf("DIVU $r%u, $r%u\n", rs, rt);
				break;
			case 0x20:
				printf("ADD $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x21:
				printf("ADDU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x22:
				printf("SUB $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x23:
				printf("SUBU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x24:
				printf("AND $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x25:
				printf("OR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x26:
				printf("XOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x27:
				printf("NOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x2A:
				printf("SLT $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:
				if(rt == 0){
					printf("BLTZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				else if(rt == 1){
					printf("BGEZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				break;
			case 0x02:
				printf("J 0x%x\n", (addr & 0xF0000000) | (target<<2));
				break;
			case 0x03:
				printf("JAL 0x%x\n", (addr & 0xF0000000) | (target<<2));
				break;
			case 0x04:
				printf("BEQ $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x05:
				printf("BNE $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x06:
				printf("BLEZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x07:
				printf("BGTZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x08:
				printf("ADDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x09:
				printf("ADDIU $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0A:
				printf("SLTI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0C:
				printf("ANDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0D:
				printf("ORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0E:
				printf("XORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0F:
				printf("LUI $r%u, 0x%x\n", rt, immediate);
				break;
			case 0x20:
				printf("LB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x21:
				printf("LH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x23:
				printf("LW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x28:
				printf("SB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x29:
				printf("SH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x2B:
				printf("SW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	printf("\n---Pipeline Contents---\n");
	printf("Cycle Count: %d \n", CYCLE_COUNT);
	//IF/ID pipeline register
	printf("PC: 0x%08X \n", CURRENT_STATE.PC);
	printf("IF/ID.IR 0x%08X \n", IF_ID.IR);
	printf("IF/ID.PC 0x%08X \n", IF_ID.PC);
	print_program(IF_ID.PC - 4); // -4 because this PC val has already been incremented

	printf("\n");

	//ID/EX pipeline register
	printf("ID/EX.IR 0x%08X \n", ID_EX.IR);
	printf("ID/EX.A 0x%08X \n", ID_EX.A);
	printf("ID/EX.B 0x%08X \n", ID_EX.B);
	printf("ID/EX.imm 0x%08X \n", ID_EX.imm);
	printf("\n");

	//EX/MEM pipeline register
	printf("EX/MEM.IR 0x%08X \n", EX_MEM.IR);
	printf("EX/MEM.A 0x%08X \n", EX_MEM.A);
	printf("EX/MEM.B 0x%08X \n", EX_MEM.B);
	printf("EX/MEM.ALUOutput 0x%08X \n", EX_MEM.ALUOutput);
	printf("\n");

	//MEM/WB pipeline register
	printf("MEM/WB.IR 0x%08X \n", MEM_WB.IR);
	printf("MEM/WB.ALUOutput 0x%08X \n", MEM_WB.ALUOutput);
	printf("MEM/WB.LMD 0x%08x \n", MEM_WB.LMD);
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	// default this to zero
	ENABLE_FORWARDING = 0;
	stallCounter = 0;

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
