all: meltdown.c meltlib.h
	gcc meltdown.c -O0 -o demo
clean:
	rm demo
