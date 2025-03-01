#pragma once
#include "Bit.h"

namespace Emu
{

class Memory
{
public:
						 Memory			();

		Byte			 read			(const Word& addr)									 const;

		bool			 write			(const Word& addr, 
										 const Byte& val);

inline	Byte*			 getBus			()																			{ return m_bus; }

private:
	Byte m_bus[0xFFFF];
};

}