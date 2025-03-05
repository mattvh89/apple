#include "Apple1.h"
#include "emu6502.h"
#include <iostream>
#include <fstream>
#ifdef _WIN32
    #include <Windows.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <sys/ioctl.h>
#endif
#include <ctime>
#include <chrono>
#include <thread>

#ifdef __linux__
    #include <termios.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <fcntl.h>
    void moveCursor()
#endif

Emu::Apple1::Apple1()
    : m_cpu(new Emu::emu6502()), 
      m_cursorPos{ 0, 0 },
      m_running(true), m_onStartup(true), m_throttled(true)
{
    #ifdef _WIN32
        setUpWindows();
    #elif defined(__linux__)
        setUpLinux();
    #endif


    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    // The apple 1 starts with random values of the display buffer, I'm just going to simulate this because there's nothing
    // added by emulating this buffer. It's essentially the buffer of the console.
    for (size_t y = 0; y <= SCREEN_CHAR_HEIGHT; ++y)
    {
        for (size_t x = 0; x < SCREEN_CHAR_WIDTH; ++x)
        {
            // Generate a random printable ASCII character (uppercase letters, numbers, and some symbols)
            char randomChar = std::toupper((std::rand() % (126 - 32) + 32)); // Roughly simulates Apple 1 video memory noise
            std::cout << randomChar;
        }
    }
    //std::cout << Apple1_Title::A1_LINE1 << Apple1_Title::A1_LINE2 << Apple1_Title::A1_LINE3 << Apple1_Title::A1_LINE4 << Apple1_Title::A1_LINE5 << Apple1_Title::A1_LINE6 << Apple1_Title::A1_LINE7 << Apple1_Title::A1_LINE8 << Apple1_Title::A1_LINE9;
}

Emu::Apple1::~Apple1()
{
    delete m_cpu;
}

#ifdef _WIN32
void Emu::Apple1::setUpWindows()
{
    //m_cpu.denatureHexText("roms/_volks_forth.txt", "roms/volks_forth.txt");
    m_stdInHandle = GetStdHandle(STD_INPUT_HANDLE);
    m_stdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    DWORD mode;
    GetConsoleMode(m_stdInHandle, &mode);
    mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT); // Disable Enter buffering & echo
    mode &= ~ENABLE_PROCESSED_INPUT;                  // Prevent special key processing (Ctrl+C, etc.)
    SetConsoleMode(m_stdInHandle, mode);

    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    if (GetCurrentConsoleFontEx(m_stdOutHandle, FALSE, &cfi))
    {
        cfi.dwFontSize.X = 24;
        cfi.dwFontSize.Y = 24;
        wcscpy_s(cfi.FaceName, L"Lucida Console");  // Set the font to Lucida Console
        SetCurrentConsoleFontEx(m_stdOutHandle, FALSE, &cfi);
    }

    // Hide the cursor (Apple 1 had no blinking cursor)
    CONSOLE_CURSOR_INFO cci;
    GetConsoleCursorInfo(m_stdOutHandle, &cci);
    cci.bVisible = false;
    SetConsoleCursorInfo(m_stdOutHandle, &cci);

    // Set console screen buffer size to match Apple 1 (40x24)
    COORD bufferSize;// = { SCREEN_CHAR_WIDTH, SCREEN_CHAR_HEIGHT };
    bufferSize.X = SCREEN_CHAR_WIDTH;
    bufferSize.Y = SCREEN_CHAR_HEIGHT;
    SetConsoleScreenBufferSize(m_stdOutHandle, bufferSize);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(m_stdOutHandle, &csbi);

    // Set window size to 40x24 (Apple 1 display size)
    SMALL_RECT windowSize = { 0, 0, csbi.dwSize.X, csbi.dwSize.Y };
    SetConsoleWindowInfo(m_stdOutHandle, TRUE, &windowSize);

    // Set text color to green (Apple 1 used monochrome green monitors)
    SetConsoleTextAttribute(m_stdOutHandle, FOREGROUND_GREEN);
}

char Emu::Apple1::readKeyboardWindows()
{
    INPUT_RECORD ir;
    DWORD readCount;
    static bool eProgram = false;

    if (PeekConsoleInput(m_stdInHandle, &ir, 1, &readCount) && readCount > 0)
    {
        ReadConsoleInput(m_stdInHandle, &ir, 1, &readCount);
        CHAR key = ir.Event.KeyEvent.uChar.AsciiChar & 0x7F;
        WORD vKey = ir.Event.KeyEvent.wVirtualKeyCode;
        if (ir.EventType == KEY_EVENT and ir.Event.KeyEvent.bKeyDown)
        {                                                                                   // First check if it was an F key, because those are special. Otherwise we write the key to a register
            switch (vKey)                                                                   // Clear screen button
            {
            case VK_F1:
                clearScreen();                                                             
                return VK_F1;
            case VK_F2:                                                                     // Reset button                                                            // The reset button on the Apple 1 does not clear the ram.
                std::cout << ' ';                                                           // clear the cursor if it's there or it will be left on the screen
                m_onStartup = false;                                                        // if this is the first time starting, this will stop the program blocking
                m_cpu->reset();                                                             // reset the cpu
                m_cpu->loadProgramHex(BASIC_ROM, BASIC_ENTRY);
                m_cpu->loadProgram2(WOZACI_ROM,  WOZACI_ENTRY);
                m_cpu->loadProgram2(WOZMON_ROM,  WOZMON_ENTRY);                             // program counter is successfully reset from the reset vector set by the wozmon, 
                m_cpu->loadProgram2(PUZZ15_ROM,  GAME_ENTRY);                               // so calling m_cpu.reset() properly sets the program counter. Now just reset the cursor
                m_cursorPos.X = 0;
                m_cursorPos.Y = 0;
                SetConsoleCursorPosition(m_stdOutHandle, m_cursorPos);
                return VK_F2;
            case VK_F3:                                                                     // throttling
                m_throttled = !m_throttled;
                return VK_F3;
            case VK_F4:                                                                     // swap basic and assembler
                if (eProgram)
                    m_cpu->loadProgramHex(BASIC_ROM, BASIC_ENTRY);
                else
                    m_cpu->loadProgram2(A1ASM_ROM, BASIC_ENTRY);
                eProgram = !eProgram;
                return VK_F4;
            case VK_F5:
                saveState();
                return VK_F5;
            case VK_F6:
                loadState();
                return VK_F6;
            case VK_F7:
                m_cpu->loadProgram2("roms/chcckers_4A_FF.txt", 0x004A);
                m_cpu->loadProgram2("roms/checkers_0300_0FFF.txt", 0x0300);
                return VK_F7;
            case VK_F8:
                m_cpu->loadProgram2(FORTH_ROM, FORTH_ENTRY);
                return VK_F8;
            case VK_F12:                                                                    // Quit button
                m_running = false;
                return VK_F12;
            }
            m_cpu->busWrite(KEYBOARD_INPUT_REGISTER, static_cast<Byte>(std::toupper(key)) | 0x80);                   // We have to write the key to the keyboard input register with the last bit set.

            Bits<Byte>::SetBit(m_cpu->getBus()[KEYBOARD_CNTRL_REGISTER], LastBit<Byte>);                             // Set the keyboard control register so the wozmon knows there's a key ready to be read
        }
}
    return 0x00;
}
#endif

/* @Todo:
    implement
 */
//std::cout << "\033[2J\033[H" << std::flush; <----- clear screen for linux
#ifdef __linux__
void Emu::Apple1::setUpLinux()
{
    static bool eProgram = false;
    struct termios newt;                                                                                            // set up non blocking input
    char ch;
    tcgetattr(STDIN_FILENO, &newt);
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    struct winsize w;                                                                                               // set console width and height
    w.ws_row = SCREEN_CHAR_WIDTH;
    w.ws_col = SCREEN_CHAR_HEIGHT;
    ioctl(STDOUT_FILENO, TIOCSWINSZ, &w);

    std::cout << "\033[32mGreen Text\033[0m" << std::endl;                                                          // set text color green
}

char Emu::Apple1::readKeyboardLinux()
{
    char key;
    if (read(STDIN_FILNEO, &key, 1) < 1)
        return (char)0x00;
    if (key == 27)                                                                                                  // Escape key detected
    { 
        char seq[10] = { 0 };
        if (read(STDIN_FILENO, &seq[0], 1) < 1) return 27;                                                          // Just ESC key pressed
        if (read(STDIN_FILENO, &seq[1], 1) < 1) return 27;                                                          // Partial sequence

        // Check if it's an F key
        if (seq[0] == '[') 
        {
            switch (seq[1]) 
            {
            case 'O': // VT100-style F1-F4
                read(STDIN_FILENO, &seq[2], 1);
                switch (seq[2]) 
                {
                case 'P': 
                    clearScreen();
                    return '1';
                case 'Q': 
                    std::cout << ' ';                                                           // clear the cursor if it's there or it will be left on the screen
                    m_onStartup = false;                                                        // if this is the first time starting, this will stop the program blocking
                    m_cpu->reset();                                                              // reset the cpu
                    m_cpu->loadProgramHex(BASIC_ROM, BASIC_ENTRY);
                    m_cpu->loadProgram2(A1ASM_ROM, ASM_ENTRY);                            // restore the programs incase they were over written
                    m_cpu->loadProgram2(WOZACI_ROM, WOZACI_ENTRY);
                    m_cpu->loadProgram2(WOZMON_ROM, WOZMON_ENTRY);                       // program counter is successfully reset from the reset vector set by the wozmon, 
                    m_cpu->loadProgram2(PUZZ15_ROM, GAME_ENTRY);                          // so calling m_cpu.reset() properly sets the program counter. Now just reset the cursor
                    m_cursorPos.X = 0;
                    m_cursorPos.Y = 0;
                    return '2';
                case 'R': 
                    m_throttled = !m_throttled;
                    return '3';
                case 'S': 
                    if (eProgram)
                        m_cpu->loadProgramHex(BASIC_ROM, BASIC_ENTRY);
                    else
                        m_cpu->loadProgram2(A1ASM_ROM, BASIC_ENTRY);
                    eProgram = !eProgram;
                    return '4';
                }

                break;
            case '1': // Linux console / xterm F5-F12
                read(STDIN_FILENO, &seq[2], 1);
                if (seq[2] == '~') 
                {
                    switch (seq[1]) 
                    {
                    case '5':
                        saveState();
                        return '5';
                    case '7': 
                        loadState();
                        return '6';
                    case '8': 
                        return '7';
                    case '9': 
                        m_cpu->loadProgram2(FORTH_ROM, FORTH_ENTRY);
                        return '8';
                    }
                }
                else 
                if (seq[2] == '0' && read(STDIN_FILENO, &seq[3], 1) > 0 && seq[3] == '~') 
                {
                    switch (seq[1]) 
                    {
                    case '1': 
                        return '9';
                    case '2': 
                        return 'A';
                    case '3': 
                        return 'B';
                    case '4': 
                        m_running = false;
                        return 'C';
                    }
                }
                break;
            }
        }
        return 0; // Unknown escape sequence
    }

    m_cpu->busWrite(KEYBOARD_INPUT_REGISTER, static_cast<Byte>(std::toupper(key)) | 0x80);                   // We have to write the key to the keyboard input register with the last bit set.

    Bits<Byte>::SetBit(m_cpu->getBus()[KEYBOARD_CNTRL_REGISTER], LastBit<Byte>);                             // Set the keyboard control register so the wozmon knows there's a key ready to be read
    return key;
}

void Emu::Apple1::moveCursor(const unsigned short& x, const unsigned short& y)
{
    std::cout << "\033[" << (int)y << ";" << (int)x << "H" << std::flush;
}
#endif

int Emu::Apple1::run()
{
    std::chrono::steady_clock::time_point start, displayFlagStart, cpuStart;
    Byte clockCount = 0;
    bool cursorFlag = false;

    start = std::chrono::steady_clock::now();
    start = std::chrono::high_resolution_clock::now();
    cpuStart = displayFlagStart = start;

    while (m_running)
    {

        // Need to check for input first so we can reset after start up
        #ifdef _WIN32
            this->readKeyboardWindows();
        #elif defiend(__linux__)
            this->readKeyboardLinux();
        #endif
        // if the apple1 has been started but not reset yet it can't do anything
        if (m_onStartup)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        // clock the cpu and process mmio registers
        m_cpu->fetch_and_execute();
        this->mmioRegisterMonitor();
        clockCount += m_cpu->getCycles();

        auto now = std::chrono::steady_clock::now();
        clockCount += m_cpu->getCycles();

        // Do timer stuff for Flashing the cursor and throttling the display rate
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
        auto displayFlagElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - displayFlagStart);
        auto cpuElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - cpuStart);

        // The bottlneck of the Apple 1 was the monitor, so we can emulate the speed of monitor by constantly flipping the last bit in the display output register
        // This is how the Apple 1 monitor actually worked too so this is good emulation. F4 toggles this on and off. If off, we need to keep that bit cleared
        if (m_throttled)
        {   // the display ready flag bit should be ready about 10x a minute, so flip it every 5 hundreths of a second
            if (displayFlagElapsed.count() >= 50)
            {
                Bits<Byte>::ToggleBit(m_cpu->getBus()[DISPLAY_OUTPUT_REGISTER], LastBit<Byte>);  // Toggle the last bit of the display output register. This controls whether the monitor is available or not
                displayFlagStart = now;
            }
            // throttle the cpu to approximately 1000 clock cycles per second, or 100 cycles per .1 seconds
            if (cpuElapsed.count() >= 200)
            if (displayFlagElapsed.count() > 50)
            {
                Bits<Byte>::ToggleBit(m_cpu->getBus()[DISPLAY_OUTPUT_REGISTER], LastBit<Byte>);  // Toggle the last bit of the display output register. This controls whether the monitor is available or not
                displayFlagStart = std::chrono::high_resolution_clock::now();
            }
            // throttle the cpu to approximately 1000 clock cycles per second, or 100 cycles per .1 seconds
            if (cpuElapsed.count() > 200)
            {
                if (clockCount >= 200)
                    std::this_thread::sleep_for(std::chrono::milliseconds((clockCount - 200)));
                clockCount = 0;
                cpuStart = now;
                cpuStart = std::chrono::high_resolution_clock::now();
            }
        }
        else
            Bits<Byte>::ClearBit(m_cpu->getBus()[DISPLAY_OUTPUT_REGISTER], LastBit<Byte>);


        if (elapsed.count() >= 500) // half a second has passed
        {
            if (cursorFlag) std::cout << ' ';
            else            std::cout << "@";
            cursorFlag = !cursorFlag;
            #ifdef _WIN32
                SetConsoleCursorPosition(m_stdOutHandle, m_cursorPos);
            #elif defined(__linux__)
                moveCursor(m_cursorPos.X, m_cursorPos.Y);
            #endif

            // Reset the start time.
            start = now;
            start = std::chrono::high_resolution_clock::now();
        }
    }

    return 0;
}


void Emu::Apple1::mmioRegisterMonitor()
{
    if (m_cpu->getInstructionName() == "STA" and m_cpu->getAddressValue() == DISPLAY_OUTPUT_REGISTER)                 // the wozmon echo routine at FFEF stores the character at the display output register from the accumulator using STA
    {
        char outputChar = std::toupper(static_cast<char>(m_cpu->busRead(DISPLAY_OUTPUT_REGISTER) & 0x7F));           // read the key from the display output register, but we don't want the last bit

        if (outputChar == CR)                                                                                       // if it's carriage return 0x8D
        {
            std::cout << ' ';                                                                                       // erase a possible ghost @ cursor
            if (++m_cursorPos.Y >= SCREEN_CHAR_HEIGHT) m_cursorPos.Y = 0;                                           // make sure we're within height of the screen
            m_cursorPos.X = 0;                                                                                      // reset x coord
        }
        else
            if (outputChar >= 32 and outputChar <= 126)                                                             // else it's a printable character so print it
            {                                                                                                       // no need to erase the cursor because our character will
                std::cout << outputChar;
                if (++m_cursorPos.X > SCREEN_CHAR_WIDTH - 1) m_cursorPos.X = 0;                                     // make sure we're within the width of the screen
            }
        #ifdef _WIN32
            SetConsoleCursorPosition(m_stdOutHandle, m_cursorPos);                                                  // set the cursor the cursor pos
        #elif defined(__linux__)
            moveCursor(m_cursorPos.X, m_cursorPos.Y);
        #endif

        Bits<Byte>::ClearBit(m_cpu->getBus()[KEYBOARD_CNTRL_REGISTER], LastBit<Byte>);                               // clear the key board control register so the wozmon knows the character has been processed

        m_cpu->busWrite(KEYBOARD_INPUT_REGISTER, 0x00);
    }
}

bool Emu::Apple1::saveState()
{
    std::ofstream saveFile(SAVE_FILE, std::ios::binary);
    if (saveFile.fail()) return false;
    saveFile.write((const char*)m_cpu->getBus(), 0xFFFF);
    return true;
}

bool Emu::Apple1::loadState()
{
    std::ifstream loadFile(SAVE_FILE, std::ios::binary);
    if (loadFile.fail()) return false;
    loadFile.read((char*)m_cpu->getBus(), 0xFFFF);
    return true;
}

void Emu::Apple1::clearScreen()
{
    for (Byte y = 0; y < SCREEN_CHAR_HEIGHT; ++y)
        for (Byte x = 0; x < SCREEN_CHAR_WIDTH; ++x)
            std::cout << ' ';

}
