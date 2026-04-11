# Concurrent Banking System
> CMSC 125 Laboratory Assignment 3

A multi-threaded banking system built in C using POSIX threads (pthreads) that processes concurrent transactions with proper synchronization. Implements reader-writer locks for account isolation, deadlock prevention via lock ordering, and a bounded buffer pool using semaphores.

---

## Group Members

| Name | Role |
|---|---|
| Angel May B. Janiola | CLI & Input Parsing, Buffer Pool, Metrics, Shared Headers |
| Myra S. Verde | Bank Operations, Transaction Execution, Timer Thread, Lock Manager |

---

## Compilation & Usage

### Requirements
- GCC compiler
- Unix-based system (Linux / macOS) or WSL
- POSIX-compliant environment with pthreads support

### Compile
```
make
```

### Compile with ThreadSanitizer (for race condition checking)
```
make debug
```

### Run
```
# Basic usage
./bankdb --accounts=tests/accounts.txt --trace=tests/trace_simple.txt --deadlock=prevention

# With custom tick interval
./bankdb --accounts=tests/accounts.txt --trace=tests/trace_simple.txt --deadlock=prevention --tick-ms=100

# With verbose output
./bankdb --accounts=tests/accounts.txt --trace=tests/trace_deadlock.txt --deadlock=prevention --verbose
```

### Run all test cases
```
make test
```

### Clean
```
make clean
```

---

## Command-Line Options

| Option | Description | Default |
|---|---|---|
| `--accounts=FILE` | Initial account balances file | required |
| `--trace=FILE` | Transaction workload trace file | required |
| `--deadlock=prevention` | Deadlock strategy (prevention only) | required |
| `--tick-ms=N` | Milliseconds per simulation tick | 100 |
| `--verbose` | Print detailed operation logs | off |

---

## Implemented Features

### 1. Multi-threaded Transaction Execution
- Each transaction runs in its own `pthread`
- Transactions wait for their scheduled `start_tick` before executing
- Supports up to 256 operations per transaction

### 2. Reader-Writer Locks
- Uses `pthread_rwlock_t` per account
- Multiple transactions can read balances simultaneously
- Write operations (deposit, withdraw, transfer) get exclusive access

### 3. Deadlock Prevention via Lock Ordering
- Transfer operations always acquire locks in ascending order of account ID
- Breaks the circular wait Coffman condition
- No transaction ever blocks indefinitely

### 4. Bounded Buffer Pool
- Fixed-size pool of 5 buffer slots (configurable)
- Uses `sem_t` semaphores to coordinate producers and consumers
- Transactions block when pool is full and unblock when a slot is freed

### 5. Timer Thread
- Dedicated thread increments global clock every `tick-ms` milliseconds
- Uses `pthread_cond_broadcast` to wake transactions waiting for their tick
- Enables true concurrent scheduling behavior

### 6. Transaction Operations
| Operation | Description |
|---|---|
| `DEPOSIT account amount` | Add centavos to account |
| `WITHDRAW account amount` | Remove centavos (aborts if insufficient funds) |
| `TRANSFER from to amount` | Move centavos between accounts (deadlock-safe) |
| `BALANCE account` | Read and print current balance |

### 7. Metrics & Reporting
- Per-transaction: start tick, actual start, end tick, wait ticks, status
- System-wide: throughput, average wait time, committed vs aborted count
- Buffer pool: total loads/unloads, peak usage, blocked operations
- Money conservation check: initial total == final total

---

## Input Format

### accounts.txt
```
# AccountID InitialBalanceCentavos
0  10000
1  25000
10 50000
20 30000
```

### trace.txt
```
# TxID StartTick Operation AccountID [Amount] [TargetAccount]
T1 0 DEPOSIT  10 5000
T2 1 WITHDRAW 10 2000
T3 2 TRANSFER 10 20 3000
T4 2 TRANSFER 20 10 1500
T5 5 BALANCE  10
```

---

## Test Cases

| Test File | Description | Expected Result |
|---|---|---|
| `trace_simple.txt` | Sequential operations, no conflicts | All commit, balance correct |
| `trace_readers.txt` | Multiple concurrent balance reads | All complete at same tick |
| `trace_deadlock.txt` | Transfers in opposite directions | Both succeed via lock ordering |
| `trace_abort.txt` | Withdrawal exceeding balance | Transaction aborts cleanly |
| `trace_buffer.txt` | More accounts than buffer slots | T6 blocks until slot freed |

---

## Project Structure

```
bankdb/
├── include/
│   ├── bank.h            # Bank and account structures
│   ├── transaction.h     # Transaction and operation types
│   ├── timer.h           # Timer thread and clock functions
│   ├── lock_mgr.h        # Lock ordering (deadlock prevention)
│   ├── buffer_pool.h     # Buffer pool with semaphores
│   └── metrics.h         # Statistics collection
├── src/
│   ├── main.c            # CLI parsing, initialization       [Angel]
│   ├── utils.c           # Parsing, error handling           [Angel]
│   ├── buffer_pool.c     # Bounded buffer implementation     [Angel]
│   ├── metrics.c         # Metrics calculation and output    [Angel]
│   ├── bank.c            # Account operations                [Myra]
│   ├── transaction.c     # Transaction execution thread      [Myra]
│   ├── timer.c           # Timer thread implementation       [Myra]
│   └── lock_mgr.c        # Deadlock prevention               [Myra]
├── tests/
│   ├── accounts.txt      # Initial account balances
│   ├── trace_simple.txt  # Test 1: No conflicts
│   ├── trace_readers.txt # Test 2: Concurrent readers
│   ├── trace_deadlock.txt# Test 3: Deadlock scenario
│   ├── trace_abort.txt   # Test 4: Insufficient funds
│   └── trace_buffer.txt  # Test 5: Buffer pool saturation
├── docs/
│   └── design.md         # Design justification and benchmarks
├── Makefile
└── README.md
```

---

## Task Distribution & Timeline

| Task | Owner | Week |
|---|---|---|
| Shared headers (`bank.h`, `transaction.h`, etc.) | Both | 1 |
| Makefile & Build System | Both | 1 |
| CLI & Input Parsing (`main.c`, `utils.c`) | Angel | 2 |
| Buffer Pool with Semaphores (`buffer_pool.c`) | Angel | 2–3 |
| Metrics & Reporting (`metrics.c`) | Angel | 3 |
| Bank Account Operations (`bank.c`) | Myra | 2 |
| Transaction Execution Thread (`transaction.c`) | Myra | 2 |
| Timer Thread (`timer.c`) | Myra | 2 |
| Lock Manager — Deadlock Prevention (`lock_mgr.c`) | Myra | 2–3 |
| Test cases & trace files | Both | 3 |
| `docs/design.md` | Both | 3 |
| ThreadSanitizer testing & debugging | Both | 4 |
| README & Documentation | Both | 1–4 |
| Lab Defense | Both | Week 4 (Apr 27–30) |

---

## Known Limitations

- Only deadlock **prevention** (lock ordering) is implemented — detection via wait-for graph is not supported
- Maximum of 100 accounts (`MAX_ACCOUNTS`)
- Maximum of 256 operations per transaction
- Buffer pool size is fixed at 5 slots

---
