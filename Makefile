CC=gcc
PTHREAD=-lpthread
WARN= -Wall
FLAGS = $(PTHREAD) $(WARN)
TARGET=file_sort
OBJS=merge_sort.o main.o

all: file_sort clean
	
file_sort: merge_sort.o main.o
	$(CC) $(OBJS) -o $(TARGET) $(FLAGS)

main.o: main.c
	$(CC) -c main.c -o main.o $(FLAGS)

merge_sort.o: resources/merge_sort.c
	$(CC) -c resources/merge_sort.c -o merge_sort.o $(WARN)

clean:
	rm -f *.o