#include "Flix-16.hpp"
#include <chrono>
#include <time.h>
#include <iostream>
#include <thread>
#include <fstream>
#include <vector>
#include <iomanip>
#include <bitset>
#include <conio.h>
#include <Windows.h>
#include <sstream>
#include <set>

#define ESC "\033["
#define RED "196"
#define BLACK "0"
#define RESET "\033[m"

using namespace std::chrono;

std::vector<uint8_t> v_op;
std::vector<uint8_t> v_ra;
std::vector<uint8_t> v_rb;
std::vector<char> program;

std::vector<uint16_t> alu_results;

std::vector<uint16_t> BreakPointIPs;

//Memory
Instruction*	ROM = new Instruction[0xFFFF];
uint16_t		RAM[0xFFFF];

//Registers
uint16_t	register_a;
uint16_t	register_b;
uint16_t	register_c;

//Binary counter feeds into IP
uint16_t	counter = 0;

//Instruction Pointer
uint16_t	IP = 0;

//ALU Flags
std::bitset<4> flags;

//IO Buses
uint16_t outbus, inbus;

//Machine in "HALT" state?
bool		halted = false;

//User input stuff
bool		paused = false;

//Delay after each executed instruction to simulate clock cycle (ms)
int			delay = 2500;

HANDLE debugConsole = CreateConsoleScreenBuffer(GENERIC_WRITE | GENERIC_READ, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
//HANDLE ramConsole = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, PAGE_GRAPHICS_COHERENT, NULL);

DWORD debugWritten = 0;
//DWORD ramWritten = 0;

HANDLE mainConsole = GetStdHandle(STD_OUTPUT_HANDLE);
DWORD mainWritten = 0;

bool debugging = false;
std::streambuf* old;

void clear(HANDLE console) {
	COORD topLeft = { 0, 0 };
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
	);
	SetConsoleCursorPosition(console, topLeft);
}

//ALU Helper functions
uint16_t ALU_CALC(ALU_OP op, uint16_t reg_a, uint16_t reg_b) {
	uint16_t result = 0;
	switch (op) {
	case ALU_OP::ADD:
		flags.reset();
		result = reg_a + reg_b;
		break;
	case ALU_OP::SUB:
		flags.reset();
		result = reg_a - reg_b;
		break;
	case ALU_OP::AND:
		result = reg_a & reg_b;
		break;
	case ALU_OP::OR:
		result = reg_a | reg_b;
		break;
	case ALU_OP::NOT:
		result = (~reg_a | ~reg_b);
		break;
	case ALU_OP::NOR:
		result = ~(reg_a | reg_b);
		break;
	case ALU_OP::NAND:
		result = !(reg_a & reg_b);
		break;
	case ALU_OP::XOR:
		result = reg_a ^ reg_b;
		break;
	case ALU_OP::LSL1:
		result = reg_a << 1;// | reg_b << 1;
		break;
	case ALU_OP::LSR1:
		result = reg_a >> 1;// | reg_b >> 1;
		break;
	}

	if (op == ALU_OP::ADD || op == ALU_OP::SUB) {
		if (result == 0) {
			flags.set(3);
			std::cout << "[" << IP << "]" << "[ALU]->ZF SET" << std::endl;
		}

		if (result < 0) {
			flags.set(1);
			std::cout << "NF SET" << std::endl;
		}
	}

	alu_results.push_back(result);
	return result;
}

void handleUserInput() {
	if (GetKeyState((USHORT)'B') & 0x8000) {
		//Handle breakpoint addition at IP
		paused = true;
		std::cout << "Enter breakpoint IP: ";
		uint16_t brk = 0;
		std::cin >> brk;

		if (std::find(BreakPointIPs.begin(), BreakPointIPs.end(), brk) != BreakPointIPs.end()) {
			//If breakpoint already exists, remove it.

			std::cout << "Removed Breakpoint" << std::endl;
			BreakPointIPs.erase(std::remove(BreakPointIPs.begin(), BreakPointIPs.end(), brk), BreakPointIPs.end());
		}
		else {

			std::cout << "Added Breakpoint" << std::endl;
			BreakPointIPs.push_back(brk);
		}

		paused = false;
	}
	if (GetKeyState((USHORT)'D') & 0x8000) {
		if (debugging) {
			SetConsoleActiveScreenBuffer(mainConsole);
			debugging = false;
		}
		else {
			SetConsoleActiveScreenBuffer(debugConsole);
			debugging = true;
			SetConsoleTitleA("Debugging...");
		}
	}
	if (GetKeyState((USHORT)'Y') & 0x8000) {
		paused = true;
		SetConsoleTitleA("Paused...");
	}
	if (GetKeyState((USHORT)'X') & 0x8000) {
		paused = false;
		SetConsoleTitleA("Unpaused...");
	}
	if (GetKeyState((USHORT)'F') & 0x8000) {
		paused = true;
		std::cout << "Enter delay in nanoseconds (default = 2500ns): ";
		std::cin >> delay;
		paused = false;
	}
	if (GetKeyState((USHORT)'S') & 0x8000) {
		if (paused) {
			paused = false;
			counter++;
			paused = true;
		}
	}
	if (GetKeyState((USHORT)'C') & 0x8000) {
		clear(mainConsole);
		paused = true;
		std::string sel = "";
		std::ofstream file;
		std::cout << "--------- Dumpage menu ---------" << std::endl;
		std::cout << "Register State        (r)" << std::endl;
		std::cout << "RAM State             (rr)" << std::endl;
		std::cout << "ALU Results           (rrr)" << std::endl;
		std::cout << "Full Hardware State   (rrrr)" << std::endl;

		std::cin >> sel;
		if(sel == "r") {

			file.open("regstate.txt");
			file << "----- Registers -----" << std::endl;
			file << "A   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_a << std::endl;
			file << "B   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_b << std::endl;
			file << "C   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_c << std::endl;
			file << "F   " << std::setfill('0') << std::setw(4) << flags << std::endl;
			file << "IP  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)IP << std::endl;
			file << "CTR " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)counter << std::endl;

			file << "----- STATUS -----" << std::endl;
			file << "OB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)outbus << std::endl;
			file << "IB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)inbus << std::endl;
			file << "HLT " << halted << std::endl;
			file.close();

		}
		if (sel == "rr") {

			file.open("ramstate.txt");
			file << "----- RAM -----" << std::endl;
			for (int i = 0; i < 0xffff; i++) {
				if (i % 16 == 0) {
					file << std::endl;
				}
				file << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)RAM[i] << " ";
			}
			file << std::endl;
			file.close();

		}
		if (sel == "rrr") {

			file.open("aluresults.txt");

			file << "----- ALU -----" << std::endl;
			for (int i = 0; i < alu_results.size(); i++) {
				if (i % 16 == 0) {
					file << std::endl;
				}
				file << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)alu_results[i] << " ";
			}
			file << std::endl;
			file.close();

		}
		if (sel == "rrrr") {
			file.open("fullhardwarestate.txt");

			file << "----- Registers -----" << std::endl;
			file << "A   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_a << std::endl;
			file << "B   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_b << std::endl;
			file << "C   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_c << std::endl;
			file << "F   " << std::setfill('0') << std::setw(4) << flags << std::endl;
			file << "IP  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)IP << std::endl;
			file << "CTR " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)counter << std::endl;

			file << "----- STATUS -----" << std::endl;
			file << "OB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)outbus << std::endl;
			file << "IB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)inbus << std::endl;
			file << "HLT " << halted << std::endl;

			file << "----- ALU -----" << std::endl;
			for (int i = 0; i < alu_results.size(); i++) {
				if (i % 16 == 0 && i != 0) {
					file << std::endl;
				}
				file << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)alu_results[i] << " ";
			}
			file << std::endl;

			file << "----- RAM -----" << std::endl;
			for (int i = 0; i < 0xffff; i++) {
				if (i % 16 == 0 && i != 0) {
					file << std::endl;
				}
				file << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)RAM[i] << " ";
			}
			file << std::endl;

		}
		std::cout << "Invalid Input." << std::endl;
		clear(mainConsole);
		paused = false;
	}
}

void handleBreakPointHit() {
	if (std::find(BreakPointIPs.begin(), BreakPointIPs.end(), IP) != BreakPointIPs.end()) {
		//If breakpoint found in list is hit:
		clear(mainConsole);
		std::cout << ESC << BLACK ";" << RED << "m";
		std::cout << "[" << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)IP << "]" << " ";
		std::cout << "Breakpoint hit!" << std::endl;
		std::cout << ROM[IP].toAssembly() << RESET << std::endl;
		paused = true;
	}
}

void init() {

	char byte = 0;
	std::string filename;
	std::cout << "Flix-16 Emulator. https://github.com/Ranchonyx" << std::endl;
	std::cout << "Enter path to a valid Flix-16 Program: ";
	std::cin >> filename;
	std::ifstream input_file(filename);

	if (!input_file.is_open()) {
		std::cerr << "Cannot open file: '" << filename << "'" << std::endl;
		exit(EXIT_FAILURE);
	}

	while (input_file.get(byte)) {
		program.push_back(byte);
	}

	for (int i = 0; i < program.size(); i += 3) {
		v_op.push_back(program[i]);
	}

	for (int i = 1; i < program.size(); i += 3) {
		v_ra.push_back(program[i]);
	}

	for (int i = 2; i < program.size(); i += 3) {
		v_rb.push_back(program[i]);
	}

	for (int i = 0; i < v_op.size(); i++) {
		std::cout << v_op[i] << std::endl;
		std::cout << v_ra[i] << std::endl;
		std::cout << v_rb[i] << std::endl;

		ROM[i] = Instruction(v_op[i], v_ra[i], v_rb[i]);
	}

	memset(RAM, 0, 0xFFFF);
	clear(mainConsole);
}

int main(int argc, char* argv[]) {

	init();
	std::cout << "Program loaded. Displaying...\n" << std::endl;

	for (int i = 0; i < v_op.size(); i++) {
		std::cout << "[" << std::setw(3) << i << "] " << ROM[i].toAssembly() << std::endl;
	}
	std::cout << std::endl;

	std::cout << "----- Simulation Controls -----" << std::endl;
	std::cout << "Y - Pause execution." << std::endl;
	std::cout << "X - Resume execution." << std::endl;
	std::cout << "D - Switch to debug view." << std::endl;
	std::cout << "F - Adjust sim. clock frequency." << std::endl;
	std::cout << "B - Insert breakpoints at IP." << std::endl;
	std::cout << "S - Increment counter by 1." << std::endl;
	std::cout << "C - Open dumpage menu." << std::endl;
	std::cout << std::endl;

	std::cout << "Press any key to execute..." << std::endl;
	int gb1 = _getch();


	//Interprete Flix-16 Machine Code
	while (!halted)
	{
		while (paused) {
			SetConsoleTitleA("Paused...");
			handleUserInput();
		}
			counter++;

			handleBreakPointHit();
			
			std::string disasm = ROM[IP].toAssembly();
			uint8_t reg_a = ROM[IP].get_a();
			uint8_t reg_b = ROM[IP].get_b();
			uint8_t opcode = ROM[IP].get_opcode();
			uint16_t imm16 = ROM[IP].get_value();

			SetConsoleTitleA("Executing...");
			std::cout << "[" << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)IP << "]" << " ";
			std::cout << "Executing ";
			std::cout << "(" << disasm << ")" << std::endl;

			//Shitton of code
			
			switch (ROM[IP].get_opcode()){
				
			case 0x00 : std::this_thread::sleep_for(5ms) ; break ;
			
			case 0x01 : register_a = imm16 ; break ;
			case 0x02 : register_b = imm16 ; break ;
			case 0x03 : register_c = imm16 ; break ;
			
			case 0x04 :	register_a = RAM[imm16] ; break ;
			case 0x05 : register_b = RAM[imm16] ; break ;
			case 0x06 : register_c = RAM[imm16] ; break ;
			
			case 0x07 : RAM[imm16] = register_a ; break ;
			case 0x08 : RAM[imm16] = register_b ; break ;
			case 0x09 :	RAM[imm16] = register_c ; break ;
			case 0x0a :
			
				uint16_t partner;
				
				switch(reg_a){
				case 0x00 : partner = register_a ; break ;
				case 0x01 : partner = register_b ; break ;
				case 0x02 : partner = register_c ; break ;
				}
				
				switch(reg_b){
				case 0x00 : register_a = ALU_CALC(ALU_OP::ADD,partner,register_a) ; break ;
				case 0x01 : register_b = ALU_CALC(ALU_OP::ADD,partner,register_b) ; break ;
				case 0x02 : register_c = ALU_CALC(ALU_OP::ADD,partner,register_c) ; break ;
				}
			
				break;
			case 0x0b :
			
				uint16_t partner;
				
				switch(reg_a){
				case 0x00 : partner = register_a ; break ;
				case 0x01 : partner = register_b ; break ;
				case 0x02 : partner = register_c ; break ;
				}
				
				switch(reg_b){
				case 0x00 : register_a = ALU_CALC(ALU_OP::SUB,partner,register_a) ; break ;
				case 0x01 : register_b = ALU_CALC(ALU_OP::SUB,partner,register_b) ; break ;
				case 0x02 : register_c = ALU_CALC(ALU_OP::SUB,partner,register_c) ; break ;
				}
				
				break;
			case 0x0c :

				uint16_t partner;
				
				switch(reg_a){
				case 0x00 : partner = register_a ; break ;
				case 0x01 : partner = register_b ; break ;
				case 0x02 : partner = register_c ; break ;
				}
				
				switch(reg_b){
				case 0x00 : register_a = ALU_CALC(ALU_OP::AND,partner,register_a) ; break ;
				case 0x01 : register_b = ALU_CALC(ALU_OP::AND,partner,register_b) ; break ;
				case 0x02 : register_c = ALU_CALC(ALU_OP::AND,partner,register_c) ; break ;
				}


				break;
			case 0x0d :

				uint16_t partner;
				
				switch(reg_a){
				case 0x00 : partner = register_a ; break ;
				case 0x01 : partner = register_b ; break ;
				case 0x02 : partner = register_c ; break ;
				}
				
				switch(reg_b){
				case 0x00 : register_a = ALU_CALC(ALU_OP::OR,partner,register_a) ; break ;
				case 0x01 : register_b = ALU_CALC(ALU_OP::OR,partner,register_b) ; break ;
				case 0x02 : register_c = ALU_CALC(ALU_OP::OR,partner,register_c) ; break ;
				}

				break;
			case 0x0e :

				uint16_t partner;
				
				switch(reg_a){
				case 0x00 : partner = register_a ; break ;
				case 0x01 : partner = register_b ; break ;
				case 0x02 : partner = register_c ; break ;
				}
				
				switch(reg_b){
				case 0x00 : register_a = ALU_CALC(ALU_OP::NOT,partner,register_a) ; break ;
				case 0x01 : register_b = ALU_CALC(ALU_OP::NOT,partner,register_b) ; break ;
				case 0x02 : register_c = ALU_CALC(ALU_OP::NOT,partner,register_c) ; break ;
				}

				break;
			case 0x0f :

				uint16_t partner;
				
				switch(reg_a){
				case 0x00 : partner = register_a ; break ;
				case 0x01 : partner = register_b ; break ;
				case 0x02 : partner = register_c ; break ;
				}
				
				switch(reg_b){
				case 0x00 : register_a = ALU_CALC(ALU_OP::NOR,partner,register_a) ; break ;
				case 0x01 : register_b = ALU_CALC(ALU_OP::NOR,partner,register_b) ; break ;
				case 0x02 : register_c = ALU_CALC(ALU_OP::NOR,partner,register_c) ; break ;
				}

				break;
			case 0x10 :

				uint16_t partner;
				
				switch(reg_a){
				case 0x00 : partner = register_a ; break ;
				case 0x01 : partner = register_b ; break ;
				case 0x02 : partner = register_c ; break ;
				}
				
				switch(reg_b){
				case 0x00 : register_a = ALU_CALC(ALU_OP::NAND,partner,register_a) ; break ;
				case 0x01 : register_b = ALU_CALC(ALU_OP::NAND,partner,register_b) ; break ;
				case 0x02 : register_c = ALU_CALC(ALU_OP::NAND,partner,register_c) ; break ;
				}

				break;
			case 0x11 : counter = imm16 ; break;
			case 0x12 :

				if(flags.test(3))
					counter = imm16;

				break;
			case 0x13 :

				if(flags.test(1))
					counter = imm16;
				
				break;
			case 0x14 :

				uint16_t partner;
				
				switch(reg_a){
				case 0x00 : partner = register_a ; break ;
				case 0x01 : partner = register_b ; break ;
				case 0x02 : partner = register_c ; break ;
				}
				
				switch(reg_b){
				case 0x00 : register_a = ALU_CALC(ALU_OP::XOR,partner,register_a) ; break ;
				case 0x01 : register_b = ALU_CALC(ALU_OP::XOR,partner,register_b) ; break ;
				case 0x02 : register_c = ALU_CALC(ALU_OP::XOR,partner,register_c) ; break ;
				}

				break;
			case 0x15 :

				std::cout << "Program Halted, press any key to exit." << std::endl;
			
				gb1 = _getch();
				halted = true;
			
				break;
			case 0x16 :	register_c = inbus ; break ;
			case 0x17 :	outbus = register_c ; break ;
			case 0x18 :

				if(!flags.test(3))
					counter = imm16;

				break;
			case 0x19 :

				uint16_t partner;
				
				switch(reg_a){
				case 0x00 : partner = register_a ; break ;
				case 0x01 : partner = register_b ; break ;
				case 0x02 : partner = register_c ; break ;
				}
				
				switch(reg_b){
				case 0x00 : register_a = ALU_CALC(ALU_OP::LSL1,partner,register_a) ; break ;
				case 0x01 : register_b = ALU_CALC(ALU_OP::LSL1,partner,register_b) ; break ;
				case 0x02 : register_c = ALU_CALC(ALU_OP::LSL1,partner,register_c) ; break ;
				}

				break;
			case 0x1a:

				uint16_t partner;
				
				switch(reg_a){
				case 0x00 : partner = register_a ; break ;
				case 0x01 : partner = register_b ; break ;
				case 0x02 : partner = register_c ; break ;
				}
				
				switch(reg_b){
				case 0x00 : register_a = ALU_CALC(ALU_OP::LSR1,partner,register_a) ; break ;
				case 0x01 : register_b = ALU_CALC(ALU_OP::LSR1,partner,register_b) ; break ;
				case 0x02 : register_c = ALU_CALC(ALU_OP::LSR1,partner,register_c) ; break ;
				}

				break;
			default :

				std::cout << "Unimplemented instruction!" << std::endl;
			
				exit(EXIT_FAILURE);
			}
			
			std::this_thread::sleep_for(nanoseconds(delay));

			IP = counter;

			//Write debug stuff to other console
			std::stringstream debugConsoleBuffer;
			old = std::cout.rdbuf(debugConsoleBuffer.rdbuf());

			std::cout << "----- INSTRUCTION -----" << std::endl;
			std::cout << "(" << disasm << ")" << std::endl;

			std::cout << "----- RAM -----" << std::endl;
			for (int i = 0; i < 20; i++) {
				if (i % 5 == 0) {
					std::cout << std::endl;
				}
				std::cout << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)RAM[i] << " ";
			}
			std::cout << std::endl;
			
			std::cout << "----- ALU RESULTS -----" << std::endl;
				for (int i = 0; i < alu_results.size(); i++) {
					if (i % 25 == 0) {
						std::cout << std::endl;
					}
					std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)alu_results[i] << " ";
				}
				std::cout << std::endl;

			std::cout << "----- Registers -----" << std::endl;
			std::cout << "A   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_a << std::endl;
			std::cout << "B   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_b << std::endl;
			std::cout << "C   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_c << std::endl;
			std::cout << "F   " << std::setfill('0') << std::setw(4) << flags << std::endl;
			std::cout << "IP  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)IP << std::endl;
			std::cout << "CTR " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)counter << std::endl;

			std::cout << "----- STATUS -----" << std::endl;
			std::cout << "OB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)outbus << std::endl;
			std::cout << "IB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)inbus << std::endl;
			std::cout << "HLT " << halted << std::endl;
			std::string text = debugConsoleBuffer.str();
			clear(debugConsole);
			WriteConsoleA(debugConsole, text.c_str(), text.length(), NULL, NULL);

			std::cout.rdbuf(old);
			

		handleUserInput();


		//std::this_thread::sleep_until(system_clock::now() + 1s);
	}
	std::cout.rdbuf(old);
	std::cout << std::endl;
	std::cout << "Execution finished." << std::endl;

	return EXIT_SUCCESS;
}