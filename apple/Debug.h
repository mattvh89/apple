#pragma once

#ifndef DEBUGGING_MODE
	//#define DEBUGGING_MODE 
	#ifdef DEBUGGING_MODE
		#include <iostream>
		#define DEBUG_OUT(message) std::cout << message << std::endl
	#else
		#define DEBUG_OUT(message) if(false) false
	#endif
#endif
