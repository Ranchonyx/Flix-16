#include "Flix-16-Machine.hpp"
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

using namespace std::chrono;


std::vector<uint8_t> v_op;
std::vector<uint8_t> v_ra;
std::vector<uint8_t> v_rb;
std::vector<char> program;

std::vector<uint16_t> alu_results;

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
int			delay = 250;

HANDLE debugConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
DWORD debugWritten = 0;

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
		result = reg_a + reg_b;
		break;
	case ALU_OP::SUB:
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
		result = reg_a << 1 | reg_b << 1;
		break;
	case ALU_OP::LSR1:
		result = reg_a >> 1 | reg_b >> 1;
		break;
	}

	flags.reset();
	if (result == 0) {
		flags.set(3);
	}
	if (result < 0) {
		flags.set(1);
	}

	if (alu_results.size() >= 40) {
		alu_results.clear();
	}
	alu_results.push_back(result);
	return result;
}

void handleUserInput() {
	if (GetKeyState((USHORT)'D') & 0x8000) {
		if (debugging) {
			SetConsoleActiveScreenBuffer(mainConsole);
			debugging = false;
		}
		else {
			SetConsoleActiveScreenBuffer(debugConsole);
			debugging = true;
		}
	}
	if (GetKeyState((USHORT)'Y') & 0x8000) {
		paused = true;
	}
	if (GetKeyState((USHORT)'X') & 0x8000) {
		paused = false;
	}
	if (GetKeyState((USHORT)'F') & 0x8000) {
		paused = true;
		clear(mainConsole);
		std::cout << "Enter delay in milliseconds (default = 100ms): ";
		std::cin >> delay;
		paused = false;
		clear(mainConsole);
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
		ROM[i] = Instruction(v_op[i], v_ra[i], v_rb[i]);
	}

	memset(RAM, 0, 0xFFFF);
}

int main(int argc, char* argv[]) {

	init();
	std::cout << "Program loaded. Displaying...\n" << std::endl;

	for (int i = 0; i < v_op.size(); i++) {
		std::cout << "(" << ROM[i].mnemonic() << ") ";
		std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)ROM[i].get_opcode() << " ";
		std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)ROM[i].get_a() << " ";
		std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)ROM[i].get_b() << " ";
		std::cout << std::endl;
	}
	std::cout << std::endl;

	std::cout << "----- Simulation Controls -----" << std::endl;
	std::cout << "Y - Pause execution." << std::endl;
	std::cout << "X - Resume execution." << std::endl;
	std::cout << "D - Switch to debug view." << std::endl;
	std::cout << "F - Adjust simulation clock frequency." << std::endl;
	std::cout << std::endl;

	std::cout << "Press any key to execute..." << std::endl;
	int gb1 = _getch();


	//Interprete Flix-16 Machine Code
	while (!halted)
	{
		while (paused) {
			handleUserInput();
		}
			counter++;
			
			std::string mnemonic = ROM[IP].mnemonic();
			uint8_t reg_a = ROM[IP].get_a();
			uint8_t reg_b = ROM[IP].get_b();
			uint8_t opcode = ROM[IP].get_opcode();
			uint16_t imm16 = ROM[IP].get_value();

			std::cout << "[" << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)IP << "]" << " ";
			std::cout << "Executing ";
			std::cout << "(" << mnemonic << ") ";
			std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char) opcode << " ";
			std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char) reg_a << " ";
			std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char) reg_b << " ";
			std::cout << std::endl;

			//Shitton of code
			switch (ROM[IP].get_opcode()) {
			case 0x00:
				std::this_thread::sleep_for(5ms);
				break;
			case 0x01:
				register_a = imm16;
				break;
			case 0x02:
				register_b = imm16;
				break;
			case 0x03:
				register_c = imm16;
				break;
			case 0x04:
				register_a = RAM[imm16];
				break;
			case 0x05:
				register_b = RAM[imm16];
				break;
			case 0x06:
				register_c = RAM[imm16];
				break;
			case 0x07:
				RAM[imm16] = register_a;
				break;
			case 0x08:
				RAM[imm16] = register_b;
				break;
			case 0x09:
				RAM[imm16] = register_c;
				break;
			case 0x0a:
				//AREGSEL_A = TRUE
				if (reg_a == 0x00 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::ADD, register_a, register_a);
				}
				if (reg_a == 0x00 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::ADD, register_a, register_b);
				}
				if (reg_a == 0x00 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::ADD, register_a, register_c);
				}
				//AREGSEL_B = TRUE
				if (reg_a == 0x01 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::ADD, register_b, register_a);
				}
				if (reg_a == 0x01 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::ADD, register_b, register_b);
				}
				if (reg_a == 0x01 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::ADD, register_b, register_c);
				}
				//AREGSEL_C = TRUE
				if (reg_a == 0x02 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::ADD, register_c, register_a);
				}
				if (reg_a == 0x02 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::ADD, register_c, register_b);
				}
				if (reg_a == 0x02 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::ADD, register_c, register_c);
				}
				break;
			case 0x0b:
				//AREGSEL_A = TRUE
				if (reg_a == 0x00 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::SUB, register_a, register_a);
				}
				if (reg_a == 0x00 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::SUB, register_a, register_b);
				}
				if (reg_a == 0x00 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::SUB, register_a, register_c);
				}
				//AREGSEL_B = TRUE
				if (reg_a == 0x01 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::SUB, register_b, register_a);
				}
				if (reg_a == 0x01 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::SUB, register_b, register_b);
				}
				if (reg_a == 0x01 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::SUB, register_b, register_c);
				}
				//AREGSEL_C = TRUE
				if (reg_a == 0x02 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::SUB, register_c, register_a);
				}
				if (reg_a == 0x02 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::SUB, register_c, register_b);
				}
				if (reg_a == 0x02 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::SUB, register_c, register_c);
				}
				break;
			case 0x0c:
				//AREGSEL_A = TRUE
				if (reg_a == 0x00 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::AND, register_a, register_a);
				}
				if (reg_a == 0x00 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::AND, register_a, register_b);
				}
				if (reg_a == 0x00 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::AND, register_a, register_c);
				}
				//AREGSEL_B = TRUE
				if (reg_a == 0x01 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::AND, register_b, register_a);
				}
				if (reg_a == 0x01 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::AND, register_b, register_b);
				}
				if (reg_a == 0x01 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::AND, register_b, register_c);
				}
				//AREGSEL_C = TRUE
				if (reg_a == 0x02 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::AND, register_c, register_a);
				}
				if (reg_a == 0x02 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::AND, register_c, register_b);
				}
				if (reg_a == 0x02 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::AND, register_c, register_c);
				}
				break;
			case 0x0d:
				//AREGSEL_A = TRUE
				if (reg_a == 0x00 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::OR, register_a, register_a);
				}
				if (reg_a == 0x00 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::OR, register_a, register_b);
				}
				if (reg_a == 0x00 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::OR, register_a, register_c);
				}
				//AREGSEL_B = TRUE
				if (reg_a == 0x01 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::OR, register_b, register_a);
				}
				if (reg_a == 0x01 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::OR, register_b, register_b);
				}
				if (reg_a == 0x01 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::OR, register_b, register_c);
				}
				//AREGSEL_C = TRUE
				if (reg_a == 0x02 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::OR, register_c, register_a);
				}
				if (reg_a == 0x02 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::OR, register_c, register_b);
				}
				if (reg_a == 0x02 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::OR, register_c, register_c);
				}
				break;
			case 0x0e:
				//AREGSEL_A = TRUE
				if (reg_a == 0x00 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::NOT, register_a, register_a);
				}
				if (reg_a == 0x00 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::NOT, register_a, register_b);
				}
				if (reg_a == 0x00 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::NOT, register_a, register_c);
				}
				//AREGSEL_B = TRUE
				if (reg_a == 0x01 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::NOT, register_b, register_a);
				}
				if (reg_a == 0x01 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::NOT, register_b, register_b);
				}
				if (reg_a == 0x01 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::NOT, register_b, register_c);
				}
				//AREGSEL_C = TRUE
				if (reg_a == 0x02 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::NOT, register_c, register_a);
				}
				if (reg_a == 0x02 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::NOT, register_c, register_b);
				}
				if (reg_a == 0x02 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::NOT, register_c, register_c);
				}
				break;
			case 0x0f:
				//AREGSEL_A = TRUE
				if (reg_a == 0x00 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::NOR, register_a, register_a);
				}
				if (reg_a == 0x00 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::NOR, register_a, register_b);
				}
				if (reg_a == 0x00 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::NOR, register_a, register_c);
				}
				//AREGSEL_B = TRUE
				if (reg_a == 0x01 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::NOR, register_b, register_a);
				}
				if (reg_a == 0x01 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::NOR, register_b, register_b);
				}
				if (reg_a == 0x01 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::NOR, register_b, register_c);
				}
				//AREGSEL_C = TRUE
				if (reg_a == 0x02 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::NOR, register_c, register_a);
				}
				if (reg_a == 0x02 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::NOR, register_c, register_b);
				}
				if (reg_a == 0x02 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::NOR, register_c, register_c);
				}
				break;
			case 0x10:
				//AREGSEL_A = TRUE
				if (reg_a == 0x00 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::NAND, register_a, register_a);
				}
				if (reg_a == 0x00 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::NAND, register_a, register_b);
				}
				if (reg_a == 0x00 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::NAND, register_a, register_c);
				}
				//AREGSEL_B = TRUE
				if (reg_a == 0x01 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::NAND, register_b, register_a);
				}
				if (reg_a == 0x01 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::NAND, register_b, register_b);
				}
				if (reg_a == 0x01 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::NAND, register_b, register_c);
				}
				//AREGSEL_C = TRUE
				if (reg_a == 0x02 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::NAND, register_c, register_a);
				}
				if (reg_a == 0x02 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::NAND, register_c, register_b);
				}
				if (reg_a == 0x02 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::NAND, register_c, register_c);
				}
				break;
			case 0x11:
				counter = imm16;
				break;
			case 0x12:
				if (flags.test(3) == true) {
					counter = imm16;
				}
				break;
			case 0x13:
				if (flags.test(2) == true) {
					counter = imm16;
				}
				break;
			case 0x14:
				//AREGSEL_A = TRUE
				if (reg_a == 0x00 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::XOR, register_a, register_a);
				}
				if (reg_a == 0x00 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::XOR, register_a, register_b);
				}
				if (reg_a == 0x00 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::XOR, register_a, register_c);
				}
				//AREGSEL_B = TRUE
				if (reg_a == 0x01 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::XOR, register_b, register_a);
				}
				if (reg_a == 0x01 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::XOR, register_b, register_b);
				}
				if (reg_a == 0x01 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::XOR, register_b, register_c);
				}
				//AREGSEL_C = TRUE
				if (reg_a == 0x02 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::XOR, register_c, register_a);
				}
				if (reg_a == 0x02 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::XOR, register_c, register_b);
				}
				if (reg_a == 0x02 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::XOR, register_c, register_c);
				}
				break;
			case 0x15:
				halted = true;
				break;
			case 0x16:
				register_c = inbus;
				break;
			case 0x17:
				outbus = register_c;
				break;
			case 0x18:
				if (flags.test(3) == false) {
					counter = imm16;
				}
				break;
			case 0x19:
				//AREGSEL_A = TRUE
				if (reg_a == 0x00 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::LSL1, register_a, register_a);
				}
				if (reg_a == 0x00 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::LSL1, register_a, register_b);
				}
				if (reg_a == 0x00 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::LSL1, register_a, register_c);
				}
				//AREGSEL_B = TRUE
				if (reg_a == 0x01 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::LSL1, register_b, register_a);
				}
				if (reg_a == 0x01 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::LSL1, register_b, register_b);
				}
				if (reg_a == 0x01 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::LSL1, register_b, register_c);
				}
				//AREGSEL_C = TRUE
				if (reg_a == 0x02 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::LSL1, register_c, register_a);
				}
				if (reg_a == 0x02 && reg_b == 0x01) {
					register_c = ALU_CALC(ALU_OP::LSL1, register_c, register_b);
				}
				if (reg_a == 0x02 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::LSL1, register_c, register_c);
				}
				break;
			case 0x1a:
				//AREGSEL_A = TRUE
				if (reg_a == 0x00 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::LSR1, register_a, register_a);
				}
				if (reg_a == 0x00 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::LSR1, register_a, register_b);
				}
				if (reg_a == 0x00 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::LSR1, register_a, register_c);
				}
				//AREGSEL_B = TRUE
				if (reg_a == 0x01 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::LSR1, register_b, register_a);
				}
				if (reg_a == 0x01 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::LSR1, register_b, register_b);
				}
				if (reg_a == 0x01 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::LSR1, register_b, register_c);
				}
				//AREGSEL_C = TRUE
				if (reg_a == 0x02 && reg_b == 0x00) {
					register_a = ALU_CALC(ALU_OP::LSR1, register_c, register_a);
				}
				if (reg_a == 0x02 && reg_b == 0x01) {
					register_b = ALU_CALC(ALU_OP::LSR1, register_c, register_b);
				}
				if (reg_a == 0x02 && reg_b == 0x02) {
					register_c = ALU_CALC(ALU_OP::LSR1, register_c, register_c);
				}
				break;
			default:
				std::cout << "Unimplemented instruction!" << std::endl;
				exit(EXIT_FAILURE);
			}
			std::this_thread::sleep_for(milliseconds(delay));

			IP = counter;

			//Write debug stuff to other console
			std::stringstream debugConsoleBuffer;
			old = std::cout.rdbuf(debugConsoleBuffer.rdbuf());

			std::cout << "----- INSTRUCTION -----" << std::endl;
			std::cout << "(" << mnemonic << ") ";
			std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)opcode << " ";
			std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)reg_a << " ";
			std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)reg_b << " ";
			std::cout << std::endl;

			std::cout << "----- RAM -----" << std::endl;
			for (int i = 0; i < 20; i++) {
				if (i % 5 == 0) {
					std::cout << std::endl;
				}
				std::cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)RAM[i] << " ";
			}
			std::cout << std::endl;
			
			std::cout << "----- ALU RESULTS -----" << std::endl;
			for (int i = 0; i < alu_results.size(); i++) {
				if (i % 5 == 0) {
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