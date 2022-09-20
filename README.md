
<br>

<div align = center>

# Flix-16

*My small 16-bit processor.*

<br>
<br>

[![Button Logism]][Logism]   
[![Button Emulator]][Emulator]   
[![Button Instruction Set]][Instruction Set]

<br>
<br>

## Features

<br>

<kbd> <br><br> **Emulator Implementation** <br><br> </kbd>   
<kbd> <br><br> **Logism-Evolution Files** <br><br> </kbd>   
<kbd> <br> **27 Instructions** <br><br> *Some are variations of others.* <br> </kbd>

<br>
<br>

## ⚠  Beware  ⚠

<br>

Trust me, once you get to the implementation of <br>
the ALU instructions, you will regret looking at this.

<br>

I was tired and lazy, which isn't really an excuse but eh.

***Have fun.***

<br>
<br>

## Showcase

*Diagrams of the **Logism** wiring.*

<br>

### Chip Design

<br>

<img
    src = 'Assets/Chip.png'
    width = 300
/>

<br>

### CPU Internals

<br>

<img
    src = 'Assets/Wiring.png'
    width = 500
/>

<br>

### 16-Bit ALU

*This Arithmetic Logic Unit is implemented using* <br>
*a carry-cascade style array of  `1 Bit`  ALU's.*

<br>

<img
    src = 'Assets/ALU.png'
    width = 500
/>

<br>

### Test Setup

*The program in the ROM chip to the left is a simple countdown from <br>
`0xff - 0x00`  which is being displayed on aforementioned display.*

<br>

<img
    src = 'Assets/Build.png'
    width = 500
/>

</div>

<br>


<!----------------------------------------------------------------------------->

[Instruction Set]: Documentation/Instruction%20Set.md
[Emulator]: Source/Emulator
[Logism]: Source/Logism


<!---------------------------------[ Buttons ]--------------------------------->

[Button Instruction Set]: https://img.shields.io/badge/Instruction_Set-006272?style=for-the-badge&logoColor=white&logo=Buffer
[Button Emulator]: https://img.shields.io/badge/Emulator-00897B?style=for-the-badge&logoColor=white&logo=GNUBash
[Button Logism]: https://img.shields.io/badge/Logism-A22430?style=for-the-badge&logoColor=white&logo=Node-RED