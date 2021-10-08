CC=gcc
PTHREAD=-lpthread
WARN= -Wall
FLAGS = $(PTHREAD) $(WARN)
TARGET=file_sort
EXE_GENERATE_IN = ./resources/generate_in
QTY_NUMBERS=0
BLOCK_SIZE=5
QTY_NUMBERS_MOD_10=$(shell echo $(QTY_NUMBERS) % 10 | bc)
QTY_NUMBERS_REMAINDER = $(shell echo $(QTY_NUMBERS) % $(BLOCK_SIZE) | bc)
OBJS=merge_sort.o main.o

ifeq ($(QTY_NUMBERS), 0)
	override BLOCK_SIZE = 5
endif

ifeq ($(QTY_NUMBERS_MOD_10), 0)
	ifneq ($(QTY_NUMBERS_REMAINDER), 0)
		override BLOCK_SIZE = 5
	endif
else
	override BLOCK_SIZE = 5
endif

all: exe_generate_in file_sort clean

exe_generate_in: generate_in

ifeq ($(QTY_NUMBERS_MOD_10), 0)

    ifneq ($(QTY_NUMBERS), 0)
	   $(EXE_GENERATE_IN) $(QTY_NUMBERS) $(BLOCK_SIZE)
    else
	   $(EXE_GENERATE_IN) -1 $(BLOCK_SIZE)
    endif

else
	$(EXE_GENERATE_IN) -1 $(BLOCK_SIZE)
endif
	
file_sort: merge_sort.o main.o 
	$(CC) $(OBJS) -o $(TARGET) $(FLAGS)

main.o: main.c
	$(CC) -c main.c -o main.o $(FLAGS)

merge_sort.o: resources/merge_sort.c
	$(CC) -c resources/merge_sort.c -o merge_sort.o $(WARN)

generate_in: resources/generate_in_file.c
	$(CC) resources/generate_in_file.c -o resources/generate_in $(WARN)

clean:
	rm -f *.o
	rm -f resources/generate_in


