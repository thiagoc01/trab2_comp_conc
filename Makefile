CC=gcc
PTHREAD=-lpthread
WARN= -Wall
FLAGS = $(PTHREAD) $(WARN)
TARGET=file_sort
EXE_GENERATE_IN = ./resources/generate_in
OBJS=merge_sort.o main.o

all: exe_generate_in file_sort clean

exe_generate_in: generate_in
	$(EXE_GENERATE_IN)
	
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
