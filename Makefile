CC = g++
CFLAGS = -Wall -g
EXEC = master
OBJS = bin_adder.o master.o

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJS)

bin_adder.o: bin_adder.cpp shared.h
	$(CC) $(CFLAGS) -c bin_adder.cpp

master.o: master.h process.cpp shared.h
	$(CC) $(CFLAGS) -c process.cpp

clean:
	rm -f $(EXEC) $(OBJS)
