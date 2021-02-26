CC = g++
CFLAGS = -Wall -g
EXEC = master
OBJS = bin_adder.o main.o

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

bin_adder.o: bin_adder.cpp shared.h
	$(CC) $(CFLAGS) -c bin_adder.cpp

master.o: master.cpp master.h process.cpp shared.h
	$(CC) $(CFLAGS) -c master.cpp process.cpp

clean:
	rm -f $(EXEC) $(OBJS)
