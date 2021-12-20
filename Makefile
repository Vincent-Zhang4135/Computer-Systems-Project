#
# Student makefile for cs154 Project 3
#
# You should not modify this Makefile, or any file other than
# csim.c: this file alone contain all your work.
#
# For this project we require that your code compiles
# cleanly (without warnings), hence the -Werror option
CFLAGS = -g -Wall -Werror -std=c99
CC = gcc

all: csim

csim: csim.c cachelab.c cachelab.h
	$(CC) $(CFLAGS) -o csim csim.c cachelab.c -lm

#
# Clean the src directory
#
clean:
	rm -rf *.o
	rm -f csim
