#include <stdio.h>
#include <stdlib.h>
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
			print_program(); 
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

	printf("|		CYCLE			|\n");

	if(CURRENT_STATE.PC >= 0x00400010)
	{
		printf("*******************\n");	
		WB();
		INSTRUCTION_COUNT++;
	}

	if(CURRENT_STATE.PC >= 0x0040000C)
	{
		printf("*******************\n");	
		MEM();
	}

	if(CURRENT_STATE.PC >= 0x00400008)
	{
		printf("*******************\n");	
		EX();
	}

	if(CURRENT_STATE.PC >= 0x00400004)
	{
		printf("*******************\n");	
		ID();
	}

	printf("*******************\n");	  
	IF();

	printf("*******************\n \n");	
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB()
{
	/*IMPLEMENT THIS*/
	printf("-Writeback- \n");

	strcpy(MEM_WB.instructionType, "rr"); // for testing if statements

	if (strcmp(MEM_WB.instructionType, "rr") == 0)
	{															//register-register instructions 
		CURRENT_STATE.REGS[MEM_WB.A] = MEM_WB.ALUOutput;		//assuming A is rd 
		printf("Register-Register Instruction\n");
		printf("Result: 0x%08X \n ", CURRENT_STATE.REGS[MEM_WB.A]);

	}	 														
	else if (strcmp(MEM_WB.instructionType, "ri") == 0)
	{															//register-immediate instructions 
		CURRENT_STATE.REGS[MEM_WB.B] = MEM_WB.ALUOutput; 		//assuming B is rt
		printf("Register-Immediate Instruction \n");
		printf("Result: 0x%08X \n ", CURRENT_STATE.REGS[MEM_WB.B]);
	}	
	else if (strcmp(MEM_WB.instructionType, "ls") == 0)
	{															//load/store instructions
		CURRENT_STATE.REGS[MEM_WB.B] = MEM_WB.LMD; 				//assuming B is rt
		printf("Load/Store Instruction \n");
		printf("Result: 0x%08X \n", CURRENT_STATE.REGS[MEM_WB.B]);	
	}
	
	INSTRUCTION_COUNT = INSTRUCTION_COUNT + 4;
	printf("****Instruction Complete****\n");
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
	/*W.I.P.*/
	printf("-Memory Access- \n");
	MEM_WB.IR = EX_MEM.IR;

	// int testAddr = 0x10010000;

	//char instructionType= "l";

	// load
	if(EX_MEM.instruction[0] == 'l')
	{
		// MEM_WB.LMD = mem_read_32(EX_MEM.ALUOutput);
		// MEM_WB.LMD = mem_read_32(testAddr);
	}
	// store
	else if(EX_MEM.instruction[0] == 's')
	{
		mem_write_32(EX_MEM.ALUOutput, EX_MEM.B);	
		// mem_write_32(testAddr, EX_MEM.B);	
	}
	// just for testing...
	else
		printf("Skipping MEM: \n");
	

}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
	/*W.I.P.*/
	printf("-Execution- \n");
	EX_MEM.IR = ID_EX.IR;
	// holds instruction type *using ls as testing*
	uint32_t opcode, function, data;

	// getting necessary pieces of the instruction for execution
	opcode = (ID_EX.IR & 0xFC000000) >> 26;
	function = ID_EX.IR & 0x0000003F;


	if(opcode == 0x00)
	{
		switch(function)
		{
			//ADD
			case 0x20:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;

			//ADDU
			case 0x21:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;			
			
			//NOR
			case 0x27:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;
			//SLT
			case 0x2A:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;	

			//SLL
			case 0x00:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;	
			//SUB
			case 0x22:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;

			//SUBU
			case 0x23:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;

			//MULT
			case 0x18:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;

			//MULTU
			case 0x19:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;

			//DIV
			case 0x1A:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;

			//DIVU
			case 0x1B:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;		

			//AND
			case 0x24:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;

			//OR
			case 0x25:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;

			//XOR
			case 0x26:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;	
			//SRL
			case 0x02:
					printf("Result: 0x%08X \n", EX_MEM.ALUOutput);
			break;
			//SRA
			case 0x03:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;	

			//MFHI
			case 0x10:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;
			//MFLO
			case 0x12:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;
			//MTHI
			case 0x11:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;
			//MTLO
			case 0x13:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;
		}
	}
	else
	{
		switch (opcode)
		{
			// ADDI
			case 0x08:
				strcpy(EX_MEM.instruction,"addi");
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;
			case 0x09: //ADDIU
				strcpy(EX_MEM.instruction,"addiu");
				EX_MEM.ALUOutput = ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF));
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);
				break;
			case 0x0A: //SLTI
				if ( (  (int32_t)ID_EX.A - (int32_t)( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF))) < 0){
					EX_MEM.ALUOutput = 0x1;
				}else{
					EX_MEM.ALUOutput = 0x0;
				}
				break;
			case 0x0C: //ANDI
				EX_MEM.ALUOutput = ID_EX.A & (ID_EX.imm & 0x0000FFFF);
				break;
			case 0x0D: //ORI
				EX_MEM.ALUOutput = ID_EX.A | (ID_EX.imm & 0x0000FFFF);
				break;
			case 0x0E: //XORI
				EX_MEM.ALUOutput = ID_EX.A ^ (ID_EX.imm & 0x0000FFFF);
				break;
			case 0x0F: //LUI
				strcpy(EX_MEM.instruction,"lui");
				CURRENT_STATE.REGS[ID_EX.B] = ID_EX.imm << 16;
				EX_MEM.ALUOutput = CURRENT_STATE.REGS[ID_EX.B];
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);
				break;
			case 0x20: //LB
				data = mem_read_32( ID_EX.A + ( (ID_EX.imm & 0x8000) > 0 ? (ID_EX.imm | 0xFFFF0000) : (ID_EX.imm & 0x0000FFFF)) );
				EX_MEM.ALUOutput = ((data & 0x000000FF) & 0x80) > 0 ? (data | 0xFFFFFF00) : (data & 0x000000FF);
				break;

			//SW
			case 0x2B:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;
			//SB
			case 0x28:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;
			//SH
			case 0x29:
				printf("Result: 0x%08X \n", EX_MEM.ALUOutput);

				break;


			default:
				printf("Instruction not implemented! \n");
				break;
		}
	}
	

}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{
	/*W.I.P.*/
	printf("-Instruction Decode- \n");
	// *** IS THIS NECESSARY??? *** Yes, we need to pass the instruction along to the next stage every cycle.
	ID_EX.IR = IF_ID.IR;
	
	uint32_t rs, rt, immediate;
	// opcode = (IF_ID.IR & 0xFC000000) >> 26;
	// function = IF_ID.IR & 0x0000003F;
	rs = (IF_ID.IR & 0x03E00000) >> 21;
	rt = (IF_ID.IR & 0x001F0000) >> 16;
	// rd = (IF_ID.IR & 0x0000F800) >> 11;
	// sa = (IF_ID.IR & 0x000007C0) >> 6;
	immediate = IF_ID.IR & 0x0000FFFF;
	// target = IF_ID.IR & 0x03FFFFFF;
	printf("rs: 0x%08x rt: 0x%08x \n", rs, rt);
	
	//I added this. seems redundant but wanted to utilize actual mips registers
	ID_EX.A = rs;
	CURRENT_STATE.REGS[rt] = rt;


	// read from register file
	ID_EX.A = CURRENT_STATE.REGS[rs]; //probably could just do ID_EX.A = (IF_ID.IR & 0xFC000000) >> 26
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
	/* VERIFY LATER THAT SIGN EXTENDING IS WORKING */
	printf("Immediate Signed: 0x%08x \n", ID_EX.imm);
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{
	printf("-Instruction Fetch- \n");

	IF_ID.PC = CURRENT_STATE.PC;
	IF_ID.IR = mem_read_32(CURRENT_STATE.PC);
	printf("Current Instruction: 0x%08X \n", IF_ID.IR);
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;
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
void print_program(){
	/*IMPLEMENT THIS*/
}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	/*IMPLEMENT THIS*/
	printf("\n---Pipeline Contents---\n");

	//IF/ID pipeline register
	printf("PC: 0x%08X \n", CURRENT_STATE.PC);
	printf("IF/ID.IR 0x%08X \n", IF_ID.IR);
	printf("IF/ID.PC 0x%08X \n", IF_ID.PC);
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
		printf("Error: No input file provided.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
