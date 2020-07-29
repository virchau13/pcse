#!/bin/bash
CC=(g++ x86_64-w64-mingw32-g++)
for n in ${CC[@]}; do
	CMD="$n -O3 -std=c++17 -Wall -Wpedantic -Wextra -Wno-class-memaccess -Werror=return-type --static src/main.cpp -o pcse"
	echo $CMD
	$CMD
done
