Main: main.cpp print.asm
	nasm -f elf32 print.asm
	g++ -m32 main.cpp print.o -o main -std=c++11
	rm -rf print.o
clean:
	rm -rf main
