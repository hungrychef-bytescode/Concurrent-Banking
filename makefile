CC      = gcc
CFLAGS  = -Wall -Wextra -Iinclude -pthread

TARGET  = bankdb
SOURCES = src/main.c \
          src/cli_parser.c \
          src/accounts_parser.c \
          src/transactions_parser.c \
          src/timer.c \
          src/bank.c \
          src/buffer_pool.c \
          src/transaction.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -g -fsanitize=thread
debug: $(TARGET)

test: $(TARGET)
	@echo "=== Test 1: No Conflicts ==="
	./$(TARGET) --accounts=tests/accounts_simple.txt --trace=tests/trace_simple.txt --deadlock=prevention
	@echo ""
	@echo "=== Test 2: Concurrent Readers ==="
	./$(TARGET) --accounts=tests/accounts_main.txt --trace=tests/trace_readers.txt --deadlock=prevention
	@echo ""
	@echo "=== Test 3: Deadlock Scenario ==="
	./$(TARGET) --accounts=tests/accounts_main.txt --trace=tests/trace_deadlock.txt --deadlock=prevention
	@echo ""
	@echo "=== Test 4: Insufficient Funds ==="
	./$(TARGET) --accounts=tests/accounts_abort.txt --trace=tests/trace_abort.txt --deadlock=prevention
	@echo ""
	@echo "=== Test 5: Buffer Pool Saturation ==="
	./$(TARGET) --accounts=tests/accounts_buffer.txt --trace=tests/trace_buffer.txt --deadlock=prevention

clean:
	rm -f $(OBJECTS) $(TARGET)
