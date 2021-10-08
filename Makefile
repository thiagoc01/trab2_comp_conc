CC=gcc
PTHREAD=-lpthread
WARN= -Wall
FLAGS = $(PTHREAD) $(WARN)
TARGET=file_sort
OBJS=merge_sort.o main.o

all: generate_in file_sort clean

generate_in: generate_in.o 
	./generate_in
	
file_sort: merge_sort.o main.o 
	$(CC) $(OBJS) -o $(TARGET) $(FLAGS)

main.o: main.c
	$(CC) -c main.c -o main.o $(FLAGS)

merge_sort.o: resources/merge_sort.c
	$(CC) -c resources/merge_sort.c -o merge_sort.o $(WARN)

generate_in.o: resources/generate_in_file.c
	$(CC) resources/generate_in_file.c -o generate_in $(WARN)

clean:
	rm -f *.o
	rm -f generate_in