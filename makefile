# variables

CC = gcc
CFLAGS = -Wall -Iinclude

TARGET = bankdb
SOURCES = src/main.c src/cli_parser.c
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