#include "Memory.h"

using namespace Emu;

Memory::Memory()
	: m_bus{ (Byte)0xFF }
{
}

Byte Emu::Memory::read(const Word& addr) const
{
	return Byte();
}

bool Emu::Memory::write(const Word& addr, const Byte& val)
{
	return false;
}
