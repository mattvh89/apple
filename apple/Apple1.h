#pragma once

#ifdef _WIN32
	#include <Windows.h>
#endif

#define KEYBOARD_INPUT_REGISTER 0xD010
#define KEYBOARD_CNTRL_REGISTER 0xD011
#define DISPLAY_OUTPUT_REGISTER 0xD012

#define WOZMON_ENTRY			0xFF00
#define BASIC_ENTRY				0xE000		// this is really the A1 assembler
#define ASM_ENTRY				0X9000
#define WOZACI_ENTRY			0xC100
#define GAME_ENTRY				0x0300
#define FORTH_ENTRY				0x1000

#define SCREEN_CHAR_WIDTH  40
#define SCREEN_CHAR_HEIGHT 24

#define ESC 0x1B
#define CR  0x0D
#define TAB '\t'
#define BS  0X08

#define LOG_FILE  "logs/log.dat"
#define SAVE_FILE "save.dat"

#define BASIC_ROM  "roms/basic.bin"
#define WOZMON_ROM "roms/wozmon1.txt"
#define WOZACI_ROM "roms/wozaci1.txt"
#define A1ASM_ROM  "roms/a1asm.txt"
#define PUZZ15_ROM "roms/puzz15.txt"
#define FORTH_ROM  "roms/volks_forth.txt"


// Forward declare the processor class
namespace Emu
{
	class emu6502;
}



namespace Emu
{

class Apple1
{
public:
									Apple1								();

									~Apple1								();

			int						run									();

protected:
			void					mmioRegisterMonitor					();

			bool					saveState							();

			bool					loadState							();

			void					clearScreen							();

		#ifdef _WIN32

			void					setUpWindows						();

			char					readKeyboardWindows					();

		#elif defined(__linux__)

			void					setUpLinux							();

			char					readKeyboardLinux					();

			void					moveCursor							(const unsigned short& x, 
																		 const unsigned short& y);

			typedef struct
			{
				Byte X, Y;
			}COORD;

		#endif
private:
	Emu::emu6502* m_cpu;
	COORD		  m_cursorPos;
	bool		  m_running,
				  m_onStartup,
				  m_throttled;

#ifdef _WIN32
	HANDLE		 m_stdOutHandle, 
				 m_stdInHandle;
#endif
};

}
