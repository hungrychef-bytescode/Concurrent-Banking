# variables

CC = gcc
CFLAGS = -Iinclude -Wall -pthread

TARGET = bankdb
SOURCES = src/main.c src/cli_parser.c src/accounts_parser.c src/transactions_parser.c
OBJECTS =  $(SOURCES:.c=.o)


all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)