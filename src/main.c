#include <stdio.h>
#include <pthread.h>
#include "cli_parser.h"
#include "accounts_parser.h"
#include "transactions.h"
#include "timer.h"
#include "bank.h"
#include "buffer_pool.h"
#include "transaction.h"

int main(int argc, char *argv[]) {

    // ── 1. Parse CLI arguments ───────────────────────────────────────
    CLIParse cli = parse_cli(argc, argv);

    // ── 2. Load accounts ─────────────────────────────────────────────
    Bank parsed_bank;
    int account_count = parse_accounts(cli.accounts, &parsed_bank);
    if (account_count <= 0) {
        fprintf(stderr, "No accounts loaded. Exiting.\n");
        return 1;
    }

    init_bank(&parsed_bank);

    int initial_total = total_balance();

    printf("=== Banking System Execution Log ===\n");
    printf("Loaded %d accounts. Initial total: PHP %d.%02d\n",
           account_count, initial_total / 100, initial_total % 100);
    printf("Deadlock strategy: %s\n", cli.deadlock_mode);
    printf("Tick interval: %d ms\n\n", cli.tick);

    // ── 3. Parse transactions ─────────────────────────────────────────
    Transaction transactions[MAX_TRANSACTIONS];
    int tx_count = parse_transactions(cli.trace, transactions, MAX_TRANSACTIONS);
    if (tx_count <= 0) {
        fprintf(stderr, "No transactions loaded. Exiting.\n");
        return 1;
    }
    printf("Loaded %d transactions.\n\n", tx_count);

    // ── 4. Init buffer pool ───────────────────────────────────────────
    init_buffer_pool();

    // ── 5. Start timer thread ─────────────────────────────────────────
    init_timer(cli.tick);
    printf("Timer thread started (tick interval: %d ms)\n\n", cli.tick);

    // ── 6. Spawn one pthread per transaction ──────────────────────────
    for (int i = 0; i < tx_count; i++) {
        transactions[i].status     = TX_RUNNING;
        transactions[i].wait_ticks = 0;
        pthread_create(&transactions[i].thread, NULL,
                       execute_transaction, &transactions[i]);
    }

    // ── 7. Wait for all transactions to finish ────────────────────────
    for (int i = 0; i < tx_count; i++) {
        pthread_join(transactions[i].thread, NULL);
    }

    // ── 8. Stop timer ─────────────────────────────────────────────────
    stop_timer();
    join_timer();

    // ── 9. Print results ──────────────────────────────────────────────
    int committed = 0, aborted = 0;
    int total_wait = 0;

    printf("\n=== Transaction Performance Metrics ===\n");
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

    printf("\nTotal transactions : %d\n", tx_count);
    printf("Committed          : %d\n", committed);
    printf("Aborted            : %d\n", aborted);
    printf("Total ticks elapsed: %d\n", global_tick);
    if (tx_count > 0) {
        printf("Average wait time  : %.2f ticks\n", (double)total_wait / tx_count);
        printf("Throughput         : %d tx / %d ticks = %.2f tx/tick\n",
               tx_count, global_tick,
               global_tick > 0 ? (double)tx_count / global_tick : 0.0);
    }

    // ── 10. Money conservation check ─────────────────────────────────
    int final_total = total_balance();
    printf("\nInitial total : PHP %d.%02d\n",
           initial_total / 100, initial_total % 100);
    printf("Final total   : PHP %d.%02d\n",
           final_total / 100, final_total % 100);
    printf("Conservation  : %s\n",
           (initial_total == final_total) ? "PASSED ✓" : "FAILED ✗");

    // ── 11. Buffer pool stats ─────────────────────────────────────────
    print_buffer_pool_stats();

    // ── 12. Cleanup ───────────────────────────────────────────────────
    destroy_buffer_pool();
    destroy_bank();

    return 0;
}