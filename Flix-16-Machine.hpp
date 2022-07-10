#pragma once
#include <stdint.h>
#include <iostream>
#include <assert.h>

enum class ALU_OP {
	ADD,
	SUB,
	AND,
	OR,
	NOT,
	NOR,
	NAND,
	XOR,
	LSL1,
	LSR1
};

enum class Instr_Type {
	MISC,
	LOAD,
	STORE,
	ALU,
	JUMP,
	IO,
	BITOP
};

std::string regnames[3] = { "RA", "RB", "RC" };

bool isInRange(int r1, int r2, uint8_t v) {
	return (v <= r2 && v >= r1);
}

std::string regToString(uint8_t r) {
	if (r <= 2) {
		return regnames[r];
	}
	return "R?";
}

Instr_Type determineInstructionType(uint8_t v) {

	//Loads
	if (isInRange(0x01, 0x06, v)) {
		return Instr_Type::LOAD;
	}

	//Stores
	if (isInRange(0x07, 0x09, v)) {
		return Instr_Type::STORE;
	}

	//ALU
	if (isInRange(0x0a, 0x10, v) || v == 0x14) {
		return Instr_Type::ALU;
	}

	//JUMPS
	if (isInRange(0x11, 0x13, v) || v == 0x18) {
		return Instr_Type::JUMP;
	}

	//IO
	if (isInRange(0x16, 0x17, v)) {
		return Instr_Type::IO;
	}

	//BITOP
	if (isInRange(0x19, 0x1a, v)) {
		return Instr_Type::BITOP;
	}

	//MISC
	if (v == 0x00 || v == 0x15) {
		return Instr_Type::MISC;
	}
	
	return Instr_Type::MISC;
}

std::string mnemonics[] = {
"NOP",
"LDAI",
"LDBI",
"LDCI",
"LDAR",
"LDBR",
"LDCR",
"STA",
"STB",
"STC",
"ADD",
"SUB",
"AND",
"OR",
"NOT",
"NOR",
"NAND",
"JMP",
"JZ",
"JN",
"XOR",
"HLT",
"IN_C",
"OUT_C",
"JNZ",
"LSL1",
"LSR1"
};


class Instruction {
private:
	//Operation code
	uint8_t opcode;

	//Register selection (2 * Imm8)
	uint8_t reg_a;
	uint8_t reg_b;

	//Value (1 * Imm16)
	uint16_t value;

	//Instruction type
	Instr_Type type;

public:
	Instruction() {
		this->opcode = 0x00;
		this->value = 0x00;
		this->reg_a = 0x00;
		this->reg_b = 0x00;
		this->type = Instr_Type::MISC;
	}

	Instruction(uint8_t p_opcode, uint8_t p_reg_a, uint8_t p_reg_b) {
		this->opcode = p_opcode;
		this->value = ((uint16_t)p_reg_a << 8) | p_reg_b;
		this->reg_a = p_reg_a;
		this->reg_b = p_reg_b;
		this->type = determineInstructionType(p_opcode);
	}

	uint8_t get_opcode() {
		return opcode;
	}

	uint8_t get_a() {
		return reg_a;
	}

	uint8_t get_b() {
		return reg_b;
	}

	uint16_t get_value() {
		return value;
	}

	std::string mnemonic() {
		return mnemonics[opcode];
	}

	std::string toAssembly() {
		char* tmp = (char*) malloc(128);
		assert(tmp != 0);
		//[00 00 00] -> INSTR RA RB / INSTR VAL
		sprintf_s(tmp, 128, "[%02x %02x %02x] -> %5s (%2s %2s) (%04x)",
			opcode, reg_a, reg_b, mnemonics[opcode].c_str(), regToString(reg_a).c_str(), regToString(reg_b).c_str(), value
		);
		assert(tmp =0 0);
		std::string str_tmp = tmp;
		return str_tmp;
	}
};