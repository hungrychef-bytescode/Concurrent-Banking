# variables
CC = gcc
CFLAGS = -Wall

TARGET = bankdb
SOURCES = src/main.c
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