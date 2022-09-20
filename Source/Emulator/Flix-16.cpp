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
using std::cout;
using std::endl;
using std::vector;


vector<uint8_t>
	v_op , v_ra , v_rb;

vector<char> program;

vector<uint16_t>
	BreakPointIPs ,
	alu_results ;


//Memory
Instruction*	ROM = new Instruction[0xFFFF];
uint16_t		RAM[0xFFFF];

//Registers
uint16_t
	register_a ,
	register_b ,
	register_c ;

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


/**
 *  @brief ALU operator application
 */

auto ALU_CALC(ALU_OP op, uint16_t reg_a, uint16_t reg_b) -> uint16_t {

    uint16_t result = 0;

    switch(op){
	case ALU_OP::ADD : flags.reset() ; result = reg_a + reg_b ; break ;
	case ALU_OP::SUB : flags.reset() ; result = reg_a - reg_b ; break ;

	case ALU_OP::NAND : result = !( reg_a &  reg_b ) ; break ;
	case ALU_OP::AND  : result =    reg_a &  reg_b   ; break ;
	case ALU_OP::NOT  : result =   ~reg_a | ~reg_b   ; break ;
	case ALU_OP::NOR  : result = ~( reg_a |  reg_b ) ; break ;
	case ALU_OP::XOR  : result =    reg_a ^  reg_b   ; break ;
	case ALU_OP::OR   : result =    reg_a |  reg_b   ; break ;

    case ALU_OP::LSL1 : result = reg_a << 1 ; break ;
	case ALU_OP::LSR1 :	result = reg_a >> 1 ; break ;
	}

	if(op == ALU_OP::ADD || op == ALU_OP::SUB)

		if(result == 0){
			flags.set(3);
			cout << "[" << IP << "]" << "[ALU]->ZF SET" << endl;
		} else
		if(result < 0){
			flags.set(1);
			cout << "NF SET" << endl;
		}
	}

	alu_results.push_back(result);
	return result;
}


void handleUserInput() {
	if (GetKeyState((USHORT)'B') & 0x8000) {
		//Handle breakpoint addition at IP
		paused = true;
		cout << "Enter breakpoint IP: ";
		uint16_t brk = 0;
		std::cin >> brk;

		if (std::find(BreakPointIPs.begin(), BreakPointIPs.end(), brk) != BreakPointIPs.end()) {
			//If breakpoint already exists, remove it.

			cout << "Removed Breakpoint" << endl;
			BreakPointIPs.erase(std::remove(BreakPointIPs.begin(), BreakPointIPs.end(), brk), BreakPointIPs.end());
		}
		else {

			cout << "Added Breakpoint" << endl;
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
		cout << "Enter delay in nanoseconds (default = 2500ns): ";
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
		cout << "--------- Dumpage menu ---------" << endl;
		cout << "Register State        (r)" << endl;
		cout << "RAM State             (rr)" << endl;
		cout << "ALU Results           (rrr)" << endl;
		cout << "Full Hardware State   (rrrr)" << endl;

		std::cin >> sel;
		if(sel == "r") {

			file.open("regstate.txt");
			file << "----- Registers -----" << endl;
			file << "A   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_a << endl;
			file << "B   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_b << endl;
			file << "C   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_c << endl;
			file << "F   " << std::setfill('0') << std::setw(4) << flags << endl;
			file << "IP  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)IP << endl;
			file << "CTR " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)counter << endl;

			file << "----- STATUS -----" << endl;
			file << "OB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)outbus << endl;
			file << "IB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)inbus << endl;
			file << "HLT " << halted << endl;
			file.close();

		}
		if (sel == "rr") {

			file.open("ramstate.txt");
			file << "----- RAM -----" << endl;
			for (int i = 0; i < 0xffff; i++) {
				if (i % 16 == 0) {
					file << endl;
				}
				file << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)RAM[i] << " ";
			}
			file << endl;
			file.close();

		}
		if (sel == "rrr") {

			file.open("aluresults.txt");

			file << "----- ALU -----" << endl;
			for (int i = 0; i < alu_results.size(); i++) {
				if (i % 16 == 0) {
					file << endl;
				}
				file << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)alu_results[i] << " ";
			}
			file << endl;
			file.close();

		}
		if (sel == "rrrr") {
			file.open("fullhardwarestate.txt");

			file << "----- Registers -----" << endl;
			file << "A   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_a << endl;
			file << "B   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_b << endl;
			file << "C   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_c << endl;
			file << "F   " << std::setfill('0') << std::setw(4) << flags << endl;
			file << "IP  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)IP << endl;
			file << "CTR " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)counter << endl;

			file << "----- STATUS -----" << endl;
			file << "OB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)outbus << endl;
			file << "IB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)inbus << endl;
			file << "HLT " << halted << endl;

			file << "----- ALU -----" << endl;
			for (int i = 0; i < alu_results.size(); i++) {
				if (i % 16 == 0 && i != 0) {
					file << endl;
				}
				file << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)alu_results[i] << " ";
			}
			file << endl;

			file << "----- RAM -----" << endl;
			for (int i = 0; i < 0xffff; i++) {
				if (i % 16 == 0 && i != 0) {
					file << endl;
				}
				file << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)RAM[i] << " ";
			}
			file << endl;

		}
		cout << "Invalid Input." << endl;
		clear(mainConsole);
		paused = false;
	}
}


void handleBreakPointHit() {

    if(!std::find(BreakPointIPs.begin(), BreakPointIPs.end(), IP) != BreakPointIPs.end())
        return;

    //If breakpoint found in list is hit:

    clear(mainConsole);

    cout
        << ESC << BLACK ";" << RED << "m"
        << "[" << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)IP << "]" << " "
        << "Breakpoint hit!" << endl
        << ROM[IP].toAssembly() << RESET << endl;

    paused = true;
}


void init() {

	char byte = 0;

	cout << "Flix-16 Emulator. https://github.com/Ranchonyx" << endl;
	cout << "Enter path to a valid Flix-16 Program: ";

	std::string filename;
	std::cin >> filename;
	std::ifstream input_file(filename);

	if(!input_file.is_open()){
		std::cerr << "Cannot open file: '" << filename << "'" << endl;
		exit(EXIT_FAILURE);
	}


	while(input_file.get(byte))
		program.push_back(byte);


	for(int i = 0; i < program.size(); i += 3){
		v_op.push_back(program[i + 0]);
		v_ra.push_back(program[i + 1]);
		v_rb.push_back(program[i + 2]);

        cout << v_op[i] << endl;
		cout << v_ra[i] << endl;
		cout << v_rb[i] << endl;

		ROM[i] = Instruction(v_op[i],v_ra[i],v_rb[i]);
    }

	memset(RAM,0,0xFFFF);
	clear(mainConsole);
}


ALU_OP opCodeToALUInstruction ( uint8_t opcode ){

    switch(opcode){
    case 0x0a : return ALU_OP::ADD  ;
    case 0x0b : return ALU_OP::SUB  ;
    case 0x0c : return ALU_OP::AND  ;
    case 0x0d : return ALU_OP::OR   ;
    case 0x0e : return ALU_OP::NOT  ;
    case 0x0f : return ALU_OP::NOR  ;
    case 0x10 : return ALU_OP::NAND ;
    case 0x14 : return ALU_OP::XOR ;
    case 0x19 : return ALU_OP::LSL1 ;
    case 0x1a : return ALU_OP::LSR1 ;
    default:

        cout << "Not an ALU instruction!" << endl;

        exit(EXIT_FAILURE);
    }
}


int main(int argc,char * argv[]){

	init();

	cout << "Program loaded. Displaying...\n" << endl;

	for(int i = 0; i < v_op.size(); i++)
		cout << "[" << std::setw(3) << i << "] " << ROM[i].toAssembly() << endl;

	cout
        << endl << "----- Simulation Controls -----"
        << endl << "Y - Pause execution."
        << endl << "X - Resume execution."
        << endl << "D - Switch to debug view."
        << endl << "F - Adjust sim. clock frequency."
        << endl << "B - Insert breakpoints at IP."
        << endl << "S - Increment counter by 1."
        << endl << "C - Open dumpage menu."
        << endl
        << endl << "Press any key to execute..."
        << endl ;

    int gb1 = _getch();


	//Interprete Flix-16 Machine Code

	while(!halted){

		while(paused){
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
        cout << "[" << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)IP << "]" << " ";
        cout << "Executing ";
        cout << "(" << disasm << ")" << endl;

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
        case 0x0b :
        case 0x0c :
        case 0x0d :
        case 0x0e :
        case 0x0f :
        case 0x10 :
        case 0x14 :
        case 0x19 :
        case 0x1a:


            uint16_t partner;

            switch(reg_a){
            case 0x00 : partner = register_a ; break ;
            case 0x01 : partner = register_b ; break ;
            case 0x02 : partner = register_c ; break ;
            }

            switch(reg_b){
            case 0x00 : register_a = ALU_CALC(operation,partner,register_a) ; break ;
            case 0x01 : register_b = ALU_CALC(operation,partner,register_b) ; break ;
            case 0x02 : register_c = ALU_CALC(operation,partner,register_c) ; break ;
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
        case 0x15 :

            cout << "Program Halted, press any key to exit." << endl;

            gb1 = _getch();
            halted = true;

            break;
        case 0x16 :	register_c = inbus ; break ;
        case 0x17 :	outbus = register_c ; break ;
        case 0x18 :

            if(!flags.test(3))
                counter = imm16;

            break;
        default :

            cout << "Unimplemented instruction!" << endl;

            exit(EXIT_FAILURE);
        }

        std::this_thread::sleep_for(nanoseconds(delay));

        IP = counter;

        //Write debug stuff to other console
        std::stringstream debugConsoleBuffer;
        old = cout.rdbuf(debugConsoleBuffer.rdbuf());

        cout << "----- INSTRUCTION -----" << endl;
        cout << "(" << disasm << ")" << endl;

        cout << "----- RAM -----" << endl;
        for (int i = 0; i < 20; i++) {
            if (i % 5 == 0) {
                cout << endl;
            }
            cout << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)RAM[i] << " ";
        }
        cout << endl;

        cout << "----- ALU RESULTS -----" << endl;
            for (int i = 0; i < alu_results.size(); i++) {
                if (i % 25 == 0) {
                    cout << endl;
                }
                cout << std::hex << std::setfill('0') << std::setw(2) << (int)(unsigned char)alu_results[i] << " ";
            }
            cout << endl;

        cout << "----- Registers -----" << endl;
        cout << "A   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_a << endl;
        cout << "B   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_b << endl;
        cout << "C   " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)register_c << endl;
        cout << "F   " << std::setfill('0') << std::setw(4) << flags << endl;
        cout << "IP  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)IP << endl;
        cout << "CTR " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)counter << endl;

        cout << "----- STATUS -----" << endl;
        cout << "OB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)outbus << endl;
        cout << "IB  " << std::hex << std::setfill('0') << std::setw(4) << (int)(unsigned char)inbus << endl;
        cout << "HLT " << halted << endl;
        std::string text = debugConsoleBuffer.str();
        clear(debugConsole);
        WriteConsoleA(debugConsole, text.c_str(), text.length(), NULL, NULL);

        cout.rdbuf(old);


		handleUserInput();


		//std::this_thread::sleep_until(system_clock::now() + 1s);
	}

	cout.rdbuf(old);

	cout
		<< endl;
		<< "Execution finished."
		<< endl;

	return EXIT_SUCCESS;
}
