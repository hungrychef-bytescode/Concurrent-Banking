# Design Documentation
> CMSC 125 Laboratory Assignment 3 — Concurrent Banking System

---

## 1. Deadlock Strategy Choice

### Strategy chosen: Prevention via Lock Ordering

We chose **deadlock prevention through lock ordering** over deadlock detection via wait-for graph. The reasoning is straightforward: prevention eliminates the possibility of deadlock entirely rather than reacting to it after the fact. This means no overhead for maintaining a wait-for graph, no periodic cycle detection, and no need to abort and retry transactions that get caught in a detected deadlock.

### Implementation

The `transfer` function in `src/bank.c` always acquires the two account locks in **ascending order of account ID**, regardless of which direction the transfer is going:

```c
int first_idx  = (from_id < to_id) ? from_idx : to_idx;
int second_idx = (from_id < to_id) ? to_idx   : from_idx;

pthread_rwlock_wrlock(&bank.accounts[first_idx].lock);
pthread_rwlock_wrlock(&bank.accounts[second_idx].lock);
```

### Which Coffman Condition Does This Break?

The four Coffman conditions for deadlock are: mutual exclusion, hold and wait, no preemption, and **circular wait**. Lock ordering directly breaks the **circular wait** condition.

Without lock ordering, consider T1 transferring from account 10 → 20 and T2 transferring from account 20 → 10 simultaneously. T1 holds lock on 10 and waits for 20; T2 holds lock on 20 and waits for 10 — a cycle, and a deadlock.

With lock ordering, both T1 and T2 must acquire the lock for account 10 first (lower ID), then account 20. So only one of them can proceed at a time. The other blocks on the first lock, not holding anything. No cycle can form because all threads agree on the same global acquisition order — it is impossible for thread A to be waiting on a lock held by thread B while B is waiting on a lock held by A.

### Proof of Correctness

Assign each account a rank equal to its account ID. Lock ordering requires every thread to acquire locks in strictly increasing rank order. For a circular wait to exist, there would need to be a chain T1 → T2 → ... → T1 where each thread holds a lock that the next one wants. But if every thread holds locks only of increasing rank, and wants a lock of even higher rank, no such cycle can close — the ranks would have to both increase and wrap around, which is impossible with a strict total order.

This is verified by our Test 3 (`trace_deadlock.txt`): T1 transfers 10 → 20 and T2 transfers 20 → 10 concurrently, and both commit successfully with conservation passing.

---

## 2. Buffer Pool Integration

### Strategy: Load per operation, unload per operation

We load the required account(s) into the buffer pool immediately before each operation executes, and unload them immediately after the operation completes. This is implemented in `src/transaction.c`:

```c
// Before each op
load_account(op->account_id);
if (op->type == OP_TRANSFER) load_account(op->target_account);

// ... execute the operation ...

// After each op
unload_account(op->account_id);
if (op->type == OP_TRANSFER) unload_account(op->target_account);
```

### Why Per-Operation?

Loading all accounts a transaction needs upfront (at thread start) would mean holding buffer slots for the entire duration of the transaction — including time spent waiting on `wait_until_tick` for later ops. This wastes pool capacity and increases the chance of blocking other transactions needlessly.

Per-operation loading minimizes how long each slot is held: the slot is occupied only for the duration of one operation, then freed immediately. This gives other concurrent transactions the best chance of finding an available slot.

### What Happens When the Pool Is Full?

`load_account` calls `sem_wait(&buffer_pool.empty_slots)` before taking a slot. If all 5 slots are occupied, `sem_wait` blocks the calling thread until another thread calls `unload_account`, which calls `sem_post(&buffer_pool.empty_slots)` to signal a slot has freed. This is the classical bounded-buffer producer-consumer pattern — the transaction simply waits without spinning, consuming no CPU until a slot becomes available.

### Justification

This design is correct because the account data itself (`bank.accounts[]`) lives in shared memory throughout the program — the buffer pool slot is just a pointer to it, not a copy. There is no risk of operating on stale data. The buffer pool's role here is to simulate the bounded-buffer constraint from the problem specification, and the per-op strategy keeps slot occupancy as short as possible, maximizing throughput under contention.

---

## 3. Reader-Writer Lock Performance

### Why rwlock Helps on Read-Heavy Workloads

`pthread_rwlock_t` distinguishes between read locks (`rdlock`) and write locks (`wrlock`). Multiple threads can hold a read lock on the same account simultaneously — they never block each other. A write lock is exclusive: it blocks all other readers and writers.

With a plain `pthread_mutex_t`, every operation — including `BALANCE` which only reads — acquires an exclusive lock. On a read-heavy workload like `trace_readers.txt` (4 threads all reading account 10), the mutex forces them to serialize one at a time. With `pthread_rwlock_t`, all 4 readers run in parallel.

### Benchmark Results

We ran `trace_readers.txt` (4 concurrent BALANCE operations on account 10) with both lock types at `--tick-ms=10`:

| Lock type | Ticks elapsed | All complete at same tick? |
|---|---|---|
| `pthread_mutex_t` | 1 | Not guaranteed (serialized) |
| `pthread_rwlock_t` | 1 | Yes — all at tick 0 |

The tick elapsed is the same in both cases because the operations are fast enough to finish within one tick interval. The meaningful difference is that with `pthread_rwlock_t`, all four `BALANCE` operations genuinely execute concurrently — observable in the output where all four show `[tick 0]` with no wait ticks. With a mutex, they would queue behind each other even though none of them modify any data.

On longer-running or more numerous reads, the rwlock advantage grows: N concurrent readers take the same wall time as 1, whereas a mutex makes them take N times as long.

### Which Trace Shows the Biggest Improvement?

`trace_readers.txt` — it is entirely composed of read operations, so rwlock provides maximum benefit. Write-heavy traces like `trace_deadlock.txt` see no benefit since writes are exclusive under both schemes.

---

## 4. Timer Thread Design

### Why a Separate Timer Thread Is Necessary

The timer thread is the heartbeat of the simulation. It runs in a loop, sleeping for `tick_interval_ms` milliseconds, then incrementing `global_tick` and broadcasting to all waiting transaction threads:

```c
while (simulation_running) {
    usleep(tick_interval_ms_global * 1000);
    pthread_mutex_lock(&tick_lock);
    global_tick++;
    pthread_cond_broadcast(&tick_changed);
    pthread_mutex_unlock(&tick_lock);
}
```

Transaction threads wait for their scheduled tick using `wait_until_tick`:

```c
pthread_mutex_lock(&tick_lock);
while (global_tick < target_tick) {
    pthread_cond_wait(&tick_changed, &tick_lock);
}
pthread_mutex_unlock(&tick_lock);
```

### What Would Break Without It?

Without a timer thread, the only way to schedule transactions at different ticks would be to process them sequentially — finish one, advance a counter, start the next. This eliminates concurrency entirely. Two transactions scheduled for tick 0 (like T1 and T2 in `trace_deadlock.txt`) would never actually run at the same time, so we could never observe or test the deadlock scenario, the concurrent reader behavior, or buffer pool contention.

The timer thread is what makes transactions truly concurrent: it advances the clock independently of any transaction's progress, and multiple transactions sleeping on `pthread_cond_wait` are all woken simultaneously by `pthread_cond_broadcast`, so they genuinely race each other from that point forward.

### How It Enables True Concurrency Testing

When the timer broadcasts, every transaction waiting on that tick wakes up at the same moment and begins competing for account locks. This creates real lock contention — the kind that exposes race conditions, tests deadlock prevention, and exercises the bounded buffer. Without the timer, you cannot meaningfully test any of these properties because you never have multiple threads active at the same instant.

The `pthread_cond_broadcast` (not `pthread_cond_signal`) is specifically important here: it wakes all waiting threads at once, not just one, ensuring that transactions scheduled for the same tick truly start concurrently rather than one at a time.