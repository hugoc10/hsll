ss: ss.c makefile
	gcc ss.c -o ss -lffi -ggdb -lm

run: ss
	./ss
