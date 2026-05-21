# Concurrent Banking System
> CMSC 125 Laboratory Assignment 3

A multi-threaded banking system built in C using POSIX threads (pthreads) that processes concurrent transactions with proper synchronization. Implements reader-writer locks for account isolation, deadlock prevention via lock ordering, and a bounded buffer pool using semaphores.

---

## Group Members

- Angel May B. Janiola
- Myra S. Verde

---

## Task Checklist

### Initial Implementation
    ☑️ CLI input parsing (Myra)
    ☑️ accounts file (initial balance) parsing (Myra)
    ☑️ transactions trace file parsing (Myra)
    ☑️ timer thread (Angel)
    ☑️ fix race conditions (Myra)
    ☑️ lock manager (Myra)
    ☑️ bank operations: deposit, withdraw, transfer, balance (Both)
    ☑️ deadlock prevention via lock ordering (Angel)
    ☑️ bounded buffer pool with semaphores (both)
    ☑️ transaction executor thread (Both)
    ☑️ metrics & reporting (Angel)
    ☑️ test cases & trace files (Both)
    ☑️ design documentation (Angel)
    ☑️ README (Both)
    ☑️ verbose deadlock prevention log (myra)
    ☑️ ThreadSanitizer verification (myra)



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
```bash
# Basic usage
./bankdb --accounts=tests/accounts_simple.txt --trace=tests/trace_simple.txt --deadlock=prevention

# With custom tick interval
./bankdb --accounts=tests/accounts_main.txt --trace=tests/trace_simple.txt --deadlock=prevention --tick-ms=100

# With verbose output
./bankdb --accounts=tests/accounts_main.txt --trace=tests/trace_deadlock.txt --deadlock=prevention --verbose
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
| `--deadlock=prevention` | Deadlock strategy; only prevention is implemented | required |
| `--tick-ms=N` | Milliseconds per simulation tick | 100 |
| `--verbose` | Print detailed operation logs | off |

---

## Implemented Features

### 1. Multi-threaded Transaction Execution
- Each transaction runs in its own `pthread`
- Transactions wait for their scheduled `start_tick` before executing using `pthread_cond_wait`
- Each operation within a transaction also waits for its own `start_tick`
- Supports up to 256 operations per transaction and up to 100 transactions

### 2. Reader-Writer Locks
- Uses `pthread_rwlock_t` per account (one lock per `Account` in `bank.accounts[]`)
- Multiple transactions can read balances simultaneously via `pthread_rwlock_rdlock`
- Write operations (deposit, withdraw, transfer) acquire exclusive access via `pthread_rwlock_wrlock`
- Locks are initialized in `init_bank()` and destroyed in `destroy_bank()`

### 3. Deadlock Prevention via Lock Ordering
- Transfer operations always acquire the two account locks in **ascending order of account ID**
- This breaks the **circular wait** Coffman condition
- No transaction ever blocks indefinitely waiting on another in a cycle
- Verified by `trace_deadlock.txt`: T1 (10→20) and T2 (20→10) both commit successfully

### 4. Bounded Buffer Pool
- Fixed-size pool of 5 buffer slots (`BUFFER_POOL_SIZE`)
- Uses two `sem_t` semaphores: `empty_slots` (initially 5) and `full_slots` (initially 0)
- `load_account()` calls `sem_wait(&empty_slots)` — blocks when the pool is full
- `unload_account()` calls `sem_wait(&full_slots)` — releases the slot and signals `sem_post(&empty_slots)`
- Accounts are loaded immediately before each operation and unloaded immediately after
- A `pthread_mutex_t pool_lock` protects the slots array from concurrent modifications
- Tracks total loads/unloads, peak usage, and blocked operations for reporting

### 5. Timer Thread
- Dedicated thread increments `global_tick` every `tick_interval_ms` milliseconds using `usleep`
- Uses `pthread_cond_broadcast(&tick_changed)` to wake all threads waiting for a new tick
- `wait_until_tick(target)` blocks using `pthread_cond_wait` until `global_tick >= target`
- Stopped via `simulation_running = 0` flag after all transactions complete

### 6. Transaction Operations

| Operation | Description |
|---|---|
| `DEPOSIT account amount` | Adds centavos to account (always succeeds) |
| `WITHDRAW account amount` | Removes centavos; aborts the transaction if balance is insufficient |
| `TRANSFER from to amount` | Moves centavos between accounts using lock ordering (deadlock-safe); aborts if insufficient funds |
| `BALANCE account` | Reads and prints current balance (read lock only) |

### 7. Metrics & Reporting
- Per-transaction: scheduled start tick, actual start tick, end tick, wait ticks, final status
- System-wide: total committed vs aborted count, total ticks elapsed, average wait time, throughput (tx/tick)
- Buffer pool report: total loads/unloads, peak slot usage, blocked operations
- Money conservation check: verifies `initial_total == final_total` after all transactions

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

A transaction is identified by its `TxID`. Multiple lines with the same `TxID` define multiple operations within one transaction. The `StartTick` on the first occurrence sets the transaction's scheduled start; each operation's `StartTick` controls when that specific operation runs.

---

## Test Cases

| Test File | Accounts File | Description | Expected Result |
|---|---|---|---|
| `trace_simple.txt` | `accounts_simple.txt` | T1 performs DEPOSIT → WITHDRAW → BALANCE sequentially | Commits; final balance = initial + deposit − withdrawal |
| `trace_readers.txt` | `accounts_main.txt` | T1–T4 all read account 10 at tick 0 | All four BALANCE ops complete concurrently via rdlock |
| `trace_deadlock.txt` | `accounts_main.txt` | T1: TRANSFER 10→20, T2: TRANSFER 20→10 simultaneously | Both commit via lock ordering; conservation passes |
| `trace_abort.txt` | `accounts_abort.txt` | T1 withdraws more than account 10's balance | T1 aborts with "insufficient funds" |
| `trace_buffer.txt` | `accounts_buffer.txt` | T1–T6 each load a distinct account; pool size is 5 | T6 blocks until a slot is freed; all eventually commit |

---

## Project Structure

```
Concurrent-Banking/
├── include/
│   ├── cli_parser.h        # CLI argument parser header
│   ├── accounts_parser.h   # Accounts file parser header (Account, Bank structs)
│   ├── transactions.h      # Transaction / Operation structs and OpType enum
│   ├── transaction.h       # execute_transaction thread entry point
│   ├── timer.h             # Timer thread, global_tick, wait_until_tick
│   └── buffer_pool.h       # BufferPool struct, load/unload, semaphores
├── src/
│   ├── main.c              # Entry point: CLI, init, thread spawn, reporting
│   ├── cli_parser.c        # --accounts, --trace, --deadlock, --tick-ms, --verbose
│   ├── accounts_parser.c   # Parses accounts.txt into Bank struct
│   ├── transactions_parser.c # Parses trace.txt into Transaction array
│   ├── timer.c             # Timer thread, wait_until_tick, global_tick
│   ├── bank.c              # deposit, withdraw, transfer, get_balance, total_balance
│   ├── buffer_pool.c       # load_account, unload_account, semaphore logic
│   └── transaction.c       # execute_transaction: per-op load/op/unload loop
├── tests/
│   ├── accounts_simple.txt # Accounts for trace_simple
│   ├── accounts_main.txt   # Accounts for trace_readers and trace_deadlock
│   ├── accounts_abort.txt  # Accounts for trace_abort
│   ├── accounts_buffer.txt # Accounts for trace_buffer
│   ├── accounts_test.txt   # General-purpose test accounts
│   ├── trace_simple.txt    # Test 1: Sequential ops, no conflict
│   ├── trace_readers.txt   # Test 2: Concurrent reads
│   ├── trace_deadlock.txt  # Test 3: Opposing transfers
│   ├── trace_abort.txt     # Test 4: Insufficient funds abort
│   └── trace_buffer.txt    # Test 5: Buffer pool saturation
├── docs/
│   └── design.md           # Deadlock strategy, buffer pool, rwlock justification
├── accounts.txt            # Default sample accounts
├── trace.txt               # Default sample trace
├── makefile
└── README.md
```

---

## Task Distribution & Timeline

| Task | Owner | Week |
|---|---|---|
| CLI input parsing | Myra | 1 |
| Accounts file parser | Myra | 1 |
| Transactions trace parser | Myra | 1 |
| Timer thread | Angel | 2 |
| Bank operations (deposit/withdraw/transfer/balance) | Angel | 2 |
| Deadlock prevention (lock ordering) | Angel | 2 |
| Bounded buffer pool (semaphores) | Angel | 2 |
| Transaction executor thread | Angel | 2 |
| Metrics & reporting | Angel | 2–3 |
| Test cases & trace files | Both | 3 |
| `docs/design.md` | Angel | 3 |
| ThreadSanitizer testing & debugging | Both | 4 |
| README & Documentation | Angel | 1–4 |
| Lab Defense | Both | Week 4 (Apr 27–30) |

---

## Known Limitations

- Only deadlock **prevention** (lock ordering) is implemented — detection via wait-for graph is not supported
- Maximum of 100 accounts (`MAX_ACCOUNTS` in `accounts_parser.h`)
- Maximum of 256 operations per transaction (`MAX_OP_PER_TX`)
- Maximum of 100 transactions per trace file (`MAX_TRANSACTIONS`)
- Buffer pool size is fixed at 5 slots (`BUFFER_POOL_SIZE` in `buffer_pool.h`)
- Amounts are stored and reported in centavos (integer arithmetic); no floating-point rounding issues, but input must be in whole centavos
- `--verbose` flag is available for extra start/commit logs

---