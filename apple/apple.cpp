#include <Windows.h>
#include "emu6502.h"
#include "Bit.h"

using namespace Emu;

#define KEYBOARD_INPUT_REGISTER 0xD010
#define KEYBOARD_CNTRL_REGISTER 0xD011
#define DISPLAY_OUTPUT_REGISTER 0xD012  // Bit 7 = 1 when ready for new char

int main()
{
	emu6502 cpu;
	INPUT_RECORD ir;
	DWORD readCount;
	HANDLE stdInHandle = GetStdHandle(STD_INPUT_HANDLE);
    Byte clearKeyBoardControlRegisterCount = 0;
    bool running = true;

	cpu.loadProgram("roms/gptest.txt", 0xE000);
	cpu.setProgramCounter(0xE000);
    std::cout << "@";

	while (running)
	{
		if (PeekConsoleInput(stdInHandle, &ir, 1, &readCount) && readCount > 0)
		{
			ReadConsoleInput(stdInHandle, &ir, 1, &readCount);
			Byte key = ir.Event.KeyEvent.uChar.AsciiChar;
			if (ir.EventType == KEY_EVENT && ir.Event.KeyEvent.bKeyDown)
			{
				cpu.getBus()[KEYBOARD_INPUT_REGISTER] = key & 0x7F;
                Bits<Byte>::SetBit(cpu.getBus()[KEYBOARD_CNTRL_REGISTER], LastBit<Byte>);
			}
		}

        cpu.fetch_and_execute();

        if (cpu.getInstructionName() == "STA" or
            cpu.getInstructionName() == "STX" or
            cpu.getInstructionName() == "STY")
        {
            if (cpu.getAddressValue() == DISPLAY_OUTPUT_REGISTER)
            {
                Byte outputChar = cpu.getBus()[DISPLAY_OUTPUT_REGISTER];
                //cpu.getBus()[DISPLAY_OUTPUT_REGISTER] &= 0x7F;
                if (outputChar == 13)                            // Carriage return
                {
                    std::cout << std::endl << "@";               // Apple 1 prompt
                }
                else if (outputChar >= 32 && outputChar <= 126)  // Printable ASCII
                {
                    std::cout << static_cast<char>(outputChar);
                }
            }
        }
        else
        if (cpu.getInstructionName() == "LDA")
        {
            if(cpu.getAddressValue() == KEYBOARD_INPUT_REGISTER)
            {
                //Bits<Byte>::ClearBit(cpu.getBus()[KEYBOARD_CNTRL_REGISTER], LastBit<Byte>);
                cpu.getBus()[KEYBOARD_CNTRL_REGISTER] = 0;
            }
        }
        else
        if(cpu.getInstructionName() == "BRK")
            running = false;

                //std::cout << "******************\n"
                //  << "Instruction: " << cpu.getInstructionName()
                //  << "\nAddr value:  " << std::hex 
                //                           << static_cast<int>(cpu.getAddressValue())
                //  << "\nProgram Counter: " << static_cast<int>(cpu.getCPU().p.getCopy())
                //  << "\nA: " <<  static_cast<int>(cpu.getCPU().a.getCopy())
                //  << "\tX: " << static_cast<int>(cpu.getCPU().x.getCopy())
                //  << "\tY: " << static_cast<int>(cpu.getCPU().y.getCopy())
                //  << "\tFlags: " << Bits<Byte>::ToBinaryString(cpu.getCPU().flags.getCopy())
                //  << "\nD010: " << Bits<Byte>::ToBinaryString(cpu.getBus()[0xD010])
                //  << "\tD011: " << Bits<Byte>::ToBinaryString(cpu.getBus()[0xD011])
                //  << "\tD012: " << Bits<Byte>::ToBinaryString(cpu.getBus()[0xD012]) 
                //  << "\n*******************\n" << std::endl;
	}


	return 0;
}