// Entry point for the banking system simulator.
// - Parses CLI args
// - Loads accounts and transactions
// - Initializes subsystems (bank, buffer pool, timer)
// - Spawns threads for each transaction
// - Collects results and prints metrics
// - Ensures money conservation and cleans up

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "cli_parser.h"
#include "accounts_parser.h"
#include "transactions.h"
#include "timer.h"
#include "bank.h"
#include "lock_mgr.h"
#include "buffer_pool.h"
#include "transactions.h"

int verbose_flag = 0;

/* Print detailed metrics and summary */
static void print_metrics(Transaction *transactions, int tx_count,
                          int initial_total, int final_tick) {
    int committed = 0, aborted = 0, total_wait = 0;

    printf("\nDetailed Metrics\n");
    printf("=== Transaction Performance Metrics ===\n");
    printf("%-6s | %-9s | %-11s | %-4s | %-9s | %s\n",
           "TxID", "StartTick", "ActualStart", "End", "WaitTicks", "Status");
    printf("-------|-----------|-------------|------|-----------|----------\n");

    for (int i = 0; i < tx_count; i++) {
        Transaction *tx = &transactions[i];
        const char *status_str =
            (tx->status == TX_COMMITTED) ? "COMMITTED" :
            (tx->status == TX_ABORTED)   ? "ABORTED"   : "RUNNING";

        printf("%-6s | %-9d | %-11d | %-4d | %-9d | %s\n",
               tx->tx_id, tx->start_tick, tx->actual_start,
               tx->actual_end, tx->wait_ticks, status_str);

        if (tx->status == TX_COMMITTED) committed++;
        else aborted++;
        total_wait += tx->wait_ticks;
    }

    printf("=== Summary ===\n");
    printf("Total transactions: %d\n", tx_count);
    printf("Committed: %d\n", committed);
    printf("Aborted: %d\n", aborted);
    printf("Total ticks: %d\n", final_tick);
#if defined(TSAN_ENABLED)
    printf("ThreadSanitizer: enabled\n");
#else
    printf("ThreadSanitizer: disabled (build with 'make debug')\n");
#endif

    if (tx_count > 0) {
        printf("\nAverage wait time: %.2f ticks\n", (double)total_wait / tx_count);
        printf("Throughput: %d transactions / %d ticks = %.2f tx/tick\n",
               tx_count, final_tick,
               final_tick > 0 ? (double)tx_count / final_tick : 0.0);
    }

    // Money conservation check
    int final_total = total_balance();
    int external_flow = net_external_flow();
    int expected_total = initial_total + external_flow;

    printf("\nInitial total : PHP %d.%02d\n",
           initial_total / 100, initial_total % 100);
    if (external_flow != 0) {
        printf("Expected final: PHP %d.%02d\n",
               expected_total / 100, expected_total % 100);
    }
    printf("Final total   : PHP %d.%02d\n",
           final_total / 100, final_total % 100);
    printf("Conservation  : %s\n",
           (expected_total == final_total) ? "PASSED ✓" : "FAILED ✗");
}

int main(int argc, char *argv[]) {
    // 1. Parse CLI
    CLIParse cli = parse_cli(argc, argv);
    verbose_flag = cli.verbose;

    // 2. Load accounts
    Bank parsed_bank;
    int account_count = parse_accounts(cli.accounts, &parsed_bank);
    if (account_count <= 0) {
        fprintf(stderr, "No accounts loaded. Exiting.\n");
        return 1;
    }
    init_bank(&parsed_bank);

    int initial_total = total_balance();
    printf("Transaction Log\n");
    printf("=== Banking System Execution Log ===\n");
    printf("Loaded %d accounts. Initial total: PHP %d.%02d\n",
           account_count, initial_total / 100, initial_total % 100);
    printf("Deadlock strategy: %s\n", cli.deadlock_mode);
    printf("Tick interval: %d ms\n\n", cli.tick);

    // 3. Load transactions
    Transaction transactions[MAX_TRANSACTIONS];
    int tx_count = parse_transactions(cli.trace, transactions, MAX_TRANSACTIONS);
    if (tx_count <= 0) {
        fprintf(stderr, "No transactions loaded. Exiting.\n");
        return 1;
    }
    printf("Loaded %d transactions.\n\n", tx_count);

    // 4. Init buffer pool
    init_buffer_pool();

    // 5. Start timer
    init_timer(cli.tick);
    printf("Timer thread started (tick interval: %d ms)\n\n", cli.tick);

    // 6. Spawn threads
    init_transaction_barrier(tx_count);
    for (int i = 0; i < tx_count; i++) {
        transactions[i].status     = TX_RUNNING;
        transactions[i].wait_ticks = 0;
        int rc = pthread_create(&transactions[i].thread, NULL,
                                execute_transaction, &transactions[i]);
        if (rc != 0) {
            errno = rc;
            perror("pthread_create");
            return 1;
        }
    }
    release_transaction_barrier();

    // 7. Wait for threads
    for (int i = 0; i < tx_count; i++) {
        int rc = pthread_join(transactions[i].thread, NULL);
        if (rc != 0) {
            errno = rc;
            perror("pthread_join");
            return 1;
        }
    }

    // 8. Stop timer
    stop_timer();
    join_timer();

    // 9. Print metrics
    int final_tick = get_current_tick();
    print_metrics(transactions, tx_count, initial_total, final_tick);

    // 10. Buffer stats
    print_buffer_pool_stats();

    // 11. Cleanup
    destroy_buffer_pool();
    destroy_bank();

    return 0;
}
