
# Instruction Set

*Flix-16, a 16-Bit CPU capable of basic operations.*

<br>

## Important

Append each ALU operation with a NOP!

So instead of

```Assembly
0a 01 00
do
0a 01 00 00 00 00
```

The CPU has no stack, so no subroutines / functions.

<br>
<br>

## Flags

| Flag | Set if ALU operation result |
|:----:|:------------|
| CF | caused a carry out
| VF | overflowed
| NF | was negative
| ZF | was zero

<br>
<br>

## Instructions

*Processor instruction mappings.*

<br>

### Info

| Instruction | Value |
|:------------|:------|
| `IMM  := 0000 - ffff` | Immediate Value
| `RAM  := 0000 - ffff` | RAM Address
| `REG  := 00 - 02`     | Register-ID (00 = A, 01 = B, 02 = C)
| `ADDR := 0000 - ffff` | ROM Address

<br>

## Miscellaneous

| Instruction | Machine Code | Description |
|:-----------:|:------------:|:------------|
| **NOP** | `00 00 00` | No operation
| **HLT** | `15 00 00` | Waits until **INT1** is pulled high

<br>

### Loading

| Instruction | Machine Code | Description |
|:-----------:|:------------:|:------------|
| **LDAI** | `01 XX XX` | Load A with immediate `XX XX`
| **LDBI** | `02 XX XX` | Load B with immediate `XX XX`
| **LDCI** | `03 XX XX` | Load C with immediate `XX XX`

<br>

| Instruction | Machine Code | Description |
|:-----------:|:------------:|:------------|
| **LDAR** | `04 XX XX` | Load A with RAM `XX XX`
| **LDBR** | `05 XX XX` | Load B with RAM `XX XX`
| **LDCR** | `06 XX XX` | Load C with RAM `XX XX`

<br>

### Storing

| Instruction | Machine Code | Description |
|:-----------:|:------------:|:------------|
| **STA** | `07 XX XX` | Store A in RAM `XX XX`
| **STB** | `08 XX XX` | Store B in RAM `XX XX`
| **STC** | `09 XX XX` | Store C in RAM `XX XX`

<br>

### ALU

<!-- NOR wrong instruction description? -->

| Instruction | Machine Code | Description |
|:-----------:|:------------:|:------------|
| **ADD**  | `0A XX XX` | ` RA  +  RB ➞ RB`
| **SUB**  | `0B XX XX` | ` RA  -  RB ➞ RB`
| **AND**  | `0C XX XX` | ` RA  &  RB ➞ RB`
| **OR**   | `0D XX XX` | ` RA \|  RB ➞ RB`
| **NOT**  | `0E XX XX` | `~RA \| ~RB ➞ RB` <br> *XX must be constant.*
| **NOR**  | `0F XX XX` | `~RA \| ~RB ➞ RB`
| **NAND** | `10 XX XX` | `~RA  & ~RB ➞ RB`
| **XOR**  | `14 XX XX` | ` RA  ^  RB ➞ RB`

<br>

### Jumps

| Instruction | Machine Code | Description |
|:-----------:|:------------:|:------------|
| **JMP** | `11 XX XX` | Jump to `XX XX` on next cycle
| **JZ**  | `12 XX XX` | Jump to `XX XX` on next cycle if ZF is set
| **JN**  | `13 XX XX` | Jump to `XX XX` on next cycle if NF is set
| **JNZ** | `18 XX XX` | Jump to `XX XX` on next cycle if ZF is NOT set

<br>

### IO

| Instruction | Machine Code | Description |
|:-----------:|:------------:|:------------|
| **IN_C**  | `16` | Load C with data on the input bus
| **OUT_C** | `17` | Output C on the output bus

<br>

### Bit Shifting

| Instruction | Machine Code | Description |
|:-----------:|:------------:|:------------|
| **LSL1** | `19 XX XX` | Logical shift left  by 1 <br> `XX` must be constant.
| **LSR1** | `1A XX XX` | Logical shift right by 1 <br> `XX` must be constant.

<br>
<br>

## 128 x 128 Display Driver

| Instruction | Machine Code |
|:-----------:|:------------:|
| **NOP**         | `0000 0000 0000 0000`
| **LDX**         | `0000 0000 0000 0001`
| **LDY**         | `0000 0000 0000 0010`
| **DATA / PLOT** | `0000 0000 0000 0011`

<br>
