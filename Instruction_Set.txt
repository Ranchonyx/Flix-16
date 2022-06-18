Flix-16, a 16-Bit CPU capable of basic operations.
Important:
Append each ALU operation with a NOP!
	So instead of
	0a 01 00
	do
	0a 01 00 00 00 00
The CPU has no stack, so no subroutines / functions.

FLAGS:
CF	Set if last ALU operation result caused a carry out
VF	Set if last ALU operation result overflowed
NF	Set if last ALU operation result was negative
ZF	Set if last ALU operation result was zero

Flix-16 Instruction Set

-- INFO --
IMM  := 0000-ffff	Immediate Value
RAM	 := 0000-ffff	RAM Address
REG  := 00-02		Register-ID (00 = A, 01 = B, 02 = C)
ADDR := 0000-ffff	ROM Address

-- MISCELLANEOUS --
1: NOP 00 00 00						No operation
2: HLT 15 00 00						Waits until INT1 is pulled high

-- LOADS --
			IMM
3: LDAI	01 XX XX					Load A with immediate XX XX
4: LDBI	02 XX XX					Load B with immediate XX XX
5: LDCI	03 XX XX					Load C with immediate XX XX

			RAM
6: LDAR	04 AA AA					Load A with RAM value
7: LDBR	05 AA AA					Load B with RAM value
8: LDCR	06 AA AA					Load C with RAM value

-- STORES --
			RAM
9:  STA	07 AA AA					Store A in RAM at AA AA
10: STB	08 AA AA					Store B in RAM at AA AA					
11: STC	09 AA AA					Store C in RAM at AA AA

-- ALU --
			 REG
12: ADD	 0a RR RR					Add RR1 + RR2,		store in RR2
13: SUB	 0b RR RR					Subtract RR1 - RR2,	store in RR2
14: AND	 0c RR RR					AND := RR1 & RR2,	store in RR2
15: OR	 0d RR RR					OR	:= RR1 | RR2,	store in RR2
16: NOT	 0e RR RR					NOT	:= ~RR1 | ~RR2  store in RR2 (constant RR)
17: NOR	 0f RR RR					NOR	:= ~RR1 | ~RR2,	store in RR2
18: NAND 10 RR RR					NAND:= ~RR1 & ~RR2,	store in RR2
19: XOR  14 RR RR					XOR	:= RR1 ^ RR2  ,	store in RR2

-- JUMPS --
		    ADDR
20: JMP	11 AA AA					Jump to AA AA on next cycle
21: JZ	12 AA AA					Jump to AA AA on next cycle if ZF is set
22: JN	13 AA AA					Jump to AA AA on next cycle if NF is set
23: JNZ 18 AA AA					Jump to AA AA on next cycle if ZF is NOT set

-- IO --
24: IN_C  16						Load C with data on the input bus
25: OUT_C 17						Output C on the output bus

-- Bitshifts --
			REG
26: LSL1 19 RR RR					Logical shift left  by 1	RR MUST BE CONSTANT
27: LSR1 1A RR RR					Logical shift right by 1	RR MUST BE CONSTANT

128x128 Improvised Display Driver Instruction Set
00: NOP ->			0000 0000 0000 0000
01: LDX ->			0000 0000 0000 0001
10: LDY ->			0000 0000 0000 0010
11: DATA/PLOT ->	0000 0000 0000 0011
