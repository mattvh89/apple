cmake_minimum_required(VERSION 3.10)

project(Apple1)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
	main.cpp
	emu6502.cpp
	Apple1.cpp

add_executable(Apple1 ${SOURCES})