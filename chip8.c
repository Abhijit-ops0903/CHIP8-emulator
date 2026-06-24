#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "raylib.h"

#define INSTRUCTIONS_PER_FRAME 9

#define FONT_START_ADDRESS 0x050
#define START_ADDRESS 0x200

#define VIDEO_SCALE 20u
#define VIDEO_WIDTH 64u
#define VIDEO_HEIGHT 32u

typedef struct chip8
{
    uint8_t Register[16];
    uint8_t ROM[4096];
    uint16_t Index;
    uint16_t PC;
    uint8_t SP;
    uint16_t stack[16];
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint8_t keypad[16];
    uint32_t display[64*32];
    uint16_t opcode;

}chip;

const unsigned int Totalbytes = 80;

chip* loadfonts(chip* font_reason){
    uint8_t fontset[80] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

    for (int i = 0; i<Totalbytes; i++){
        font_reason->ROM[FONT_START_ADDRESS + i] = fontset[i];
    }
    return font_reason;
}

uint8_t RNG(){
    uint8_t min = 0;
    uint8_t max = 255;
    uint8_t random = (rand() % (max - min + 1)) + min;

    return random;
}

//--------------------------------------------------------------

//00E0: CLS clear the display
chip* OP_00E0(chip* IP){
    memset(IP->display, 0, sizeof(IP->display));

    return IP;
}

//00EE RET return from a subroutine
chip* OP_00EE(chip* IP){
    IP->SP -= 1;
    IP->PC = IP->stack[IP->SP];

    return IP;
}

//1nnn JP addr (jump to location nnn and the interpreter sets the program counter to nnn)
chip* OP_1nnn(chip* IP){
    uint16_t address = IP->opcode & 0x0FFFu;
    IP->PC = address;

    return IP;
}

//2nnn CALL addr (call subroutine at nnn)
chip* OP_2nnn(chip* IP){
    uint16_t address = IP->opcode & 0x0FFFu;
    if (IP->SP < 16){
        IP->stack[IP->SP] = IP->PC;
    }

    IP->SP++;
    IP->PC = address;

    return IP;
}

//3xkk SE Vx, byte (skip next instruction if Vx = kk)
chip* OP_3xkk(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t byte = IP->opcode & 0x00FFu;

    if (IP->Register[Vx] == byte){
        IP->PC += 2;
    }

    return IP;
}

//4xkk SNE Vx, byte (skip next instruction if Vx != kk)
chip* OP_4xkk(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t byte = IP->opcode & 0x00FFu;

    if (IP->Register[Vx] != byte){
        IP->PC += 2;
    }

    return IP;
}

//5xy0 SE Vx, Vy (skip next instruction if Vx = Vy)
chip* OP_5xy0(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (IP->opcode & 0x00F0u) >> 4u;

	if (IP->Register[Vx] == IP->Register[Vy])
	{
		IP->PC += 2;
	}

    return IP;
}

//6xkk LD Vx, byte (set Vx = kk)
chip* OP_6xkk(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t byte = IP->opcode & 0x00FFu;

    IP->Register[Vx] = byte;

    return IP;
}

//7xkk ADD Vx, byte (set Vx = Vx + kk)
chip* OP_7xkk(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t byte = IP->opcode & 0x00FFu;

    IP->Register[Vx] += byte;

    return IP;
}

//8xy0 LD Vx, Vy (set Vx = Vy)
chip* OP_8xy0(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (IP->opcode & 0x00F0u) >> 4u;

    IP->Register[Vx] = IP->Register[Vy];

    return IP;
}

//8xy1 OR Vx, Vy (set Vx = Vx or Vy)
chip* OP_8xy1(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (IP->opcode & 0x00F0u) >> 4u;

    IP->Register[Vx] |= IP->Register[Vy];

    return IP;
}

//8xy2 AND Vx, Vy (set Vx = Vx and Vy)
chip* OP_8xy2(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (IP->opcode & 0x00F0u) >> 4u;

    IP->Register[Vx] &= IP->Register[Vy];

    return IP;
}

//8xy3 XOR Vx, Vy (set Vx = Vx xor Vy)
chip* OP_8xy3(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (IP->opcode & 0x00F0u) >> 4u;

    IP->Register[Vx] ^= IP->Register[Vy];

    return IP;
}

//8xy4 ADD Vx, Vy (set Vx = Vx + Vy and set VF as the carry)
chip* OP_8xy4(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (IP->opcode & 0x00F0u) >> 4u;

    uint16_t sum = IP->Register[Vx] + IP->Register[Vy];

    if (sum > 255u){
        IP->Register[0xF] = 1;
    }
    else{
        IP->Register[0xF] = 0;
    }

    IP->Register[Vx] = sum & 0xFFu;
    return IP;
}

//8xy5 SUB Vx, Vy (set Vx = Vx - Vy and set VF  = NOT BORROW)
chip* OP_8xy5(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (IP->opcode & 0x00F0u) >> 4u;

    if (IP->Register[Vx] > IP->Register[Vy]){
        IP->Register[0xF] = 1;
    }
    else{
        IP->Register[0xF] = 0;
    }

    IP->Register[Vx] -= IP->Register[Vy];
    return IP;
}

//8xy6 SHR Vx (Set Vx = Vx SHR 1)
//(If the least-significant bit of Vx is 1, then VF is set to 1, 
//otherwise 0. Then Vx is divided by 2.)
chip* OP_8xy6(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;

    IP->Register[0xF] = IP->Register[Vx] & 0x1u;
    IP->Register[Vx] >>= 1;

    return IP;
}

//8xy7 SUBN Vx, Vy (set Vx = Vy - Vx, set VF = NOT BORROW)
chip* OP_8xy7(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (IP->opcode & 0x00F0u) >> 4u;

    if (IP->Register[Vx] > IP->Register[Vy]){
        IP->Register[0xF] = 1;
    }
    else{
        IP->Register[0xF] = 0;
    }

    IP->Register[Vx] = IP->Register[Vy] - IP->Register[Vx];
    return IP;
}

//8xyE SHL Vx { , Vy} (set Vx = Vx SHL 1)
//(If the least-significant bit of Vx is 1, then VF is set to 1, 
//otherwise 0. Then Vx is multiplied by 2.)
chip* OP_8xyE(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    IP->Register[0xF] = (IP->Register[Vx] & 0x80u) >> 7u;
    IP->Register[Vx] <<= 1;

    return IP;
}

//9xy0 SNE Vx, Vy (skip next instruction if Vx != Vy)
chip* OP_9xy0(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t Vy = (IP->opcode & 0x00F0u) >> 4u;

    if (IP->Register[Vx] != IP->Register[Vy]){
        IP->PC += 2;
    }

    return IP;
}

//Annn LD I, addr (set I = nnn)
chip* OP_Annn(chip* IP){
    uint16_t address = IP->opcode & 0x0FFFu;

    IP->Index = address;
    return IP;
}

//Bnnn JP V0, addr (jump to location nnn + V0)
chip* OP_Bnnn(chip* IP){
    uint16_t address = IP->opcode & 0x0FFFu;
    IP->PC = IP->Register[0] + address;

    return IP;
}

//Cxkk RND Vx, byte (set random byte AND kk)
chip* OP_Cxkk(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t byte = IP->opcode & 0x00FFu;

    IP->Register[Vx] = RNG() & byte;

    return IP;
}

//Dxyn DRW Vx, Vy, nibble (set )
//Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision.
chip* OP_Dxyn(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
	uint8_t Vy = (IP->opcode & 0x00F0u) >> 4u;
	uint8_t height = IP->opcode & 0x000Fu;

	// Wrap if going beyond screen boundaries
	uint8_t xPos = IP->Register[Vx] % VIDEO_WIDTH;
	uint8_t yPos = IP->Register[Vy] % VIDEO_HEIGHT;

	IP->Register[0xF] = 0;

	for (unsigned int row = 0; row < height; ++row)
	{
		uint8_t spriteByte = IP->ROM[IP->Index + row];

		for (unsigned int col = 0; col < 8; ++col)
		{
			uint8_t spritePixel = spriteByte & (0x80u >> col);
			uint32_t* screenPixel = &IP->display[((yPos + row) % VIDEO_HEIGHT) * VIDEO_WIDTH + (xPos + col) % VIDEO_WIDTH];

			// Sprite pixel is on
			if (spritePixel)
			{
				// Screen pixel also on - collision
				if (*screenPixel == 0xFFFFFFFF)
				{
					IP->Register[0xF] = 1;
				}

				// Effectively XOR with the sprite pixel
				*screenPixel ^= 0xFFFFFFFF;
			}
		}
	}

    return IP;
}

//Ex9E SKP Vx (skip the next instruction if key with the value Vx is pressed)
chip* OP_Ex9E(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t key = IP->Register[Vx];

    if (IP->keypad[key]){
        IP->PC += 2;
    }

    return IP;
}

//ExA1 - SKNP Vx (skip the next instruction if key with the value Vx is not pressed)
chip* OP_ExA1(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t key = IP->Register[Vx];

    if (!IP->keypad[key]){
        IP->PC += 2;
    }

    return IP;
}

//Fx07 LD Vx, DT (set Vx = delay timer value)
chip* OP_Fx07(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    IP->Register[Vx] = IP->delay_timer;

    return IP;
}

//Fx0A LD Vx, K (wait for a keypress and store the value in Vx)
chip* OP_Fx0A(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;

	if (IP->keypad[0])
	{
		IP->Register[Vx] = 0;
	}
	else if (IP->keypad[1])
	{
		IP->Register[Vx] = 1;
	}
	else if (IP->keypad[2])
	{
		IP->Register[Vx] = 2;
	}
	else if (IP->keypad[3])
	{
		IP->Register[Vx] = 3;
	}
	else if (IP->keypad[4])
	{
		IP->Register[Vx] = 4;
	}
	else if (IP->keypad[5])
	{
		IP->Register[Vx] = 5;
	}
	else if (IP->keypad[6])
	{
		IP->Register[Vx] = 6;
	}
	else if (IP->keypad[7])
	{
		IP->Register[Vx] = 7;
	}
	else if (IP->keypad[8])
	{
		IP->Register[Vx] = 8;
	}
	else if (IP->keypad[9])
	{
		IP->Register[Vx] = 9;
	}
	else if (IP->keypad[10])
	{
		IP->Register[Vx] = 10;
	}
	else if (IP->keypad[11])
	{
		IP->Register[Vx] = 11;
	}
	else if (IP->keypad[12])
	{
		IP->Register[Vx] = 12;
	}
	else if (IP->keypad[13])
	{
		IP->Register[Vx] = 13;
	}
	else if (IP->keypad[14])
	{
		IP->Register[Vx] = 14;
	}
	else if (IP->keypad[15])
	{
		IP->Register[Vx] = 15;
	}
	else
	{
		IP->PC -= 2;
	}

    return IP;
}

//Fx15 LD DT, Vx (set delay timer = Vx)
chip* OP_Fx15(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    IP->delay_timer = IP->Register[Vx];

    return IP;
}

//Fx18 LD ST, Vx (set sound timer = Vx)
chip* OP_Fx18(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    IP->sound_timer = IP->Register[Vx];

    return IP;
}

//Fx1E ADD I, Vx (set I = I + Vx)
chip* OP_Fx1E(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    IP->Index += IP->Register[Vx];

    return IP;
}

//Fx29 LD F, Vx (set I = location of sprite for digit Vx)
chip* OP_Fx29(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t digit = IP->Register[Vx];

    IP->Index = FONT_START_ADDRESS + (5 * digit);

    return IP;
}

//Fx33 LD B, Vx (Store BCD representation of Vx in memory locations I, I+1, and I+2)
//The interpreter takes the decimal value of Vx, and places the hundreds digit in 
//memory at location in I, the tens digit at location I+1, and the ones digit at location I+2.
chip* OP_Fx33(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;
    uint8_t value = IP->Register[Vx];

    IP->ROM[IP->Index + 2] = value % 10;
    value /= 10;

    IP->ROM[IP->Index + 1] = value % 10;
    value /= 10;

    IP->ROM[IP->Index] = value % 10;

    return IP;
}

//Fx55 LD[I], Vx (store registers from V0 to Vx in memory starting at location I)
chip* OP_Fx55(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i<=Vx; i++){
        IP->ROM[IP->Index + i] = IP->Register[i];
    }

    return IP;
}

//Fx65 LD Vx, [I] (read registers from V0 to Vx from memory starting at location I)
chip* OP_Fx65(chip* IP){
    uint8_t Vx = (IP->opcode & 0x0F00u) >> 8u;

    for (uint8_t i = 0; i<=Vx; i++){
        IP->Register[i] = IP->ROM[IP->Index + i];
    }

    return IP;
}

//--------------------------------------------------------------


chip* Read_Rom(char* filepath, chip* memory_purpose){
    FILE *file = fopen(filepath, "rb");
    if (file == NULL){
        perror("Error opening the file");
        return NULL;
    }

    uint8_t buffer[4096];
    size_t reading = fread(buffer, sizeof(char), 4096, file);
    fclose(file);

    if (reading > sizeof(memory_purpose->ROM)/sizeof(memory_purpose->ROM[0]) - START_ADDRESS){
        perror("The file is too big to be loaded into the ROM.\n OPERATION TERMINATED");
        return NULL;
    }

    printf("The bytes that were successfully read are %zu\n", reading);
    //read the bytes or show them.
    for (size_t i = 0; i<reading; i++){
        memory_purpose->ROM[START_ADDRESS+i] = buffer[i];
    }
    return memory_purpose;

}


//--------------------------------------------
typedef chip* (*opcodefunc)(chip*);
opcodefunc table[16];
opcodefunc table0[16];
opcodefunc table8[16];
opcodefunc tableE[16];
opcodefunc tableF[256];

chip* Table0(chip* IP)
{
	return table0[IP->opcode & 0x000Fu](IP);
}
chip* Table8(chip* IP)
{
	return table8[IP->opcode & 0x000Fu](IP);
}
chip* TableE(chip* IP)
{
	return tableE[IP->opcode & 0x000Fu](IP);
}
chip* TableF(chip* IP)
{
	return tableF[IP->opcode & 0x00FFu](IP);
}
chip* OP_NULL(chip* IP){
    return IP;
}

void tableload(){
    table[0x0] = Table0;
    table[0x1] = OP_1nnn;
    table[0x2] = OP_2nnn;
    table[0x3] = OP_3xkk;
    table[0x4] = OP_4xkk;
    table[0x5] = OP_5xy0;
    table[0x6] = OP_6xkk;
    table[0x7] = OP_7xkk;
    table[0x8] = Table8;
    table[0x9] = OP_9xy0;
    table[0xA] = OP_Annn;
    table[0xB] = OP_Bnnn;
    table[0xC] = OP_Cxkk;
    table[0xD] = OP_Dxyn;
    table[0xE] = TableE;
    table[0xF] = TableF;
}

void table0load(){
    for (size_t i = 0; i<=0xE; i++){
        table0[i] = OP_NULL;
    }
    table0[0x0] = OP_00E0;
    table0[0xE] = OP_00EE;
}

void table8load(){
    for (size_t i = 0; i<=0xE; i++){
        table8[i] = OP_NULL;
    }
    table8[0x0] = OP_8xy0;
    table8[0x1] = OP_8xy1;
    table8[0x2] = OP_8xy2;
    table8[0x3] = OP_8xy3;
    table8[0x4] = OP_8xy4;
    table8[0x5] = OP_8xy5;
    table8[0x6] = OP_8xy6;
    table8[0x7] = OP_8xy7;
    table8[0xE] = OP_8xyE;
}

void tableEload(){
    for (size_t i = 0; i<=0xE; i++){
        tableE[i] = OP_NULL;
    }
    tableE[0x1] = OP_ExA1;
    tableE[0xE] = OP_Ex9E;
}

void tableFload(){
    for (size_t i = 0; i <= 256; i++)
    {
    	tableF[i] = OP_NULL;
    }
    tableF[0x07] = OP_Fx07;
    tableF[0x0A] = OP_Fx0A;
    tableF[0x15] = OP_Fx15;
    tableF[0x18] = OP_Fx18;
    tableF[0x1E] = OP_Fx1E;
    tableF[0x29] = OP_Fx29;
    tableF[0x33] = OP_Fx33;
    tableF[0x55] = OP_Fx55;
    tableF[0x65] = OP_Fx65;
}
//--------------------------------------------

//--------------------------------------
chip* execute_instruction(chip* IP){
    IP->opcode = (IP->ROM[IP->PC] << 8u ) | IP->ROM[IP->PC + 1];
    IP->PC += 2;

    table[(IP->opcode & 0xF000u) >> 12u](IP);

    return IP;
}

chip* tick_timers(chip* IP){
    if (IP->delay_timer > 0){
        IP->delay_timer-=1;
    }
    if (IP->sound_timer > 0){
        IP->sound_timer-=1;
    }

    return IP;
}
//--------------------------------------


int main(int argc, char* argv[]){

    //---------------------------------
    if (argc < 2){
        fprintf(stderr, "Usage: %s <path_to_rom.ch8>\n", argv[0]);
        return EXIT_FAILURE;
    }

    srand(time(NULL));
    chip *CHIP8 = calloc(1, sizeof(chip));

    if (CHIP8 == NULL){
        perror("The chip is undefined");
        return EXIT_FAILURE;
    }

    CHIP8 = Read_Rom(argv[1], CHIP8);

    if (CHIP8 == NULL){
        perror("Undefined ROM behaviour");
        free(CHIP8);
        return EXIT_FAILURE;
    }

    table0load();
    tableload();
    table8load();
    tableEload();
    tableFload();

    CHIP8 = loadfonts(CHIP8);
    CHIP8->PC = START_ADDRESS;

    InitWindow(VIDEO_WIDTH * VIDEO_SCALE, VIDEO_HEIGHT * VIDEO_SCALE, "Chip8 emulator");
    SetTargetFPS(60);
    bool noclosewindow = true;

    while(noclosewindow){

        for (uint8_t i = 0; i<INSTRUCTIONS_PER_FRAME; i++){
            execute_instruction(CHIP8);
        }
        tick_timers(CHIP8);

        //-------------------------------------------------
        if (IsKeyDown(KEY_X)){
            CHIP8->keypad[0x0] = 1;
        }
        else {
            CHIP8->keypad[0x0] = 0;
        }
        if (IsKeyDown(KEY_KP_1)){
            CHIP8->keypad[0x1] = 1;
        }
        else {
            CHIP8->keypad[0x1] = 0;
        }
        if (IsKeyDown(KEY_KP_2)){
            CHIP8->keypad[0x2] = 1;
        }
        else{
            CHIP8->keypad[0x2] = 0;
        }
        if (IsKeyDown(KEY_KP_3)){
            CHIP8->keypad[0x3] = 1;
        }
        else{
            CHIP8->keypad[0x3] = 0;
        }
        if (IsKeyDown(KEY_Q)){
            CHIP8->keypad[0x4] = 1;
        }
        else{
            CHIP8->keypad[0x4] = 0;
        }
        if (IsKeyDown(KEY_W)){
            CHIP8->keypad[0x5] = 1;
        }
        else{
            CHIP8->keypad[0x5] = 0;
        }
        if (IsKeyDown(KEY_E)){
            CHIP8->keypad[0x6] = 1;
        }
        else{
            CHIP8->keypad[0x6] = 0;
        }
        if (IsKeyDown(KEY_A)){
            CHIP8->keypad[0x7] = 1;
        }
        else{
            CHIP8->keypad[0x7] = 0;
        }
        if (IsKeyDown(KEY_S)){
            CHIP8->keypad[0x8] = 1;
        }
        else{
            CHIP8->keypad[0x8] = 0;
        }
        if (IsKeyDown(KEY_D)){
            CHIP8->keypad[0x9] = 1;
        }
        else{
            CHIP8->keypad[0x9] = 0;
        }
        if (IsKeyDown(KEY_Z)){
            CHIP8->keypad[0xA] = 1;
        }
        else{
            CHIP8->keypad[0xA] = 0;
        }
        if (IsKeyDown(KEY_C)){
            CHIP8->keypad[0xB] = 1;
        }
        else{
            CHIP8->keypad[0xB] = 0;
        }
        if (IsKeyDown(KEY_KP_4)){
            CHIP8->keypad[0xC] = 1;
        }
        else{
            CHIP8->keypad[0xC] = 0;
        }
        if (IsKeyDown(KEY_R)){
            CHIP8->keypad[0xD] = 1;
        }
        else{
            CHIP8->keypad[0xD] = 0;
        }
        if (IsKeyDown(KEY_F)){
            CHIP8->keypad[0xE] = 1;
        }
        else{
            CHIP8->keypad[0xE] = 0;
        }
        if (IsKeyDown(KEY_V)){
            CHIP8->keypad[0xF] = 1;
        }
        else{
            CHIP8->keypad[0xF] = 0;
        }
        if (IsKeyPressed(KEY_ESCAPE)){
            noclosewindow = false;
        }
        //--------------------------------------------------

        BeginDrawing();
        ClearBackground(BLACK);


        for(int y = 0; y < VIDEO_HEIGHT; y++)
        {
            for(int x = 0; x < VIDEO_WIDTH; x++)
            {

                uint32_t pixel =
                CHIP8->display[y * VIDEO_WIDTH + x];


                if(pixel)
                {
                    DrawRectangle(
                        x * VIDEO_SCALE,
                        y * VIDEO_SCALE,
                        VIDEO_SCALE,
                        VIDEO_SCALE,
                        WHITE
                    );
                }

            }
        }
        EndDrawing();
    }
    CloseWindow();
    free(CHIP8);

    return 0;

}