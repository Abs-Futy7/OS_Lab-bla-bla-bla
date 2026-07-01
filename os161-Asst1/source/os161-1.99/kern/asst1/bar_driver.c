/* Driver for the ASST1 bar synchronization problem. */
#include <types.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <test.h>
#include "bar_driver.h"

static struct semaphore *bar_finished; /* main runbar waits for all threads */
static int bottle_count[NBOTTLES + 1]; /* printed bottle usage statistics */
static struct lock *stats_lock;        /* protects bottle_count updates */

static void
record_bottle_use(const int requested[DRINK_SIZE])
{
        int i;

        lock_acquire(stats_lock); /* many customers may update stats */
        for (i = 0; i < DRINK_SIZE; i++) {
                if (requested[i] != NO_INGREDIENT) {
                        KASSERT(requested[i] >= 1);
                        KASSERT(requested[i] <= NBOTTLES);
                        bottle_count[requested[i]]++; /* one dose served */
                }
        }
        lock_release(stats_lock);
}

static void
customer(void *unused, unsigned long customer_id)
{
        int i, j;
        struct bar_order order;
        char semname[32];

        (void)unused;

        snprintf(semname, sizeof(semname), "customer %lu", customer_id);
        order.finished = sem_create(semname, 0); /* private wait point */
        KASSERT(order.finished != NULL);

        for (i = 0; i < DRINKS_PER_CUSTOMER; i++) {
                order.valid = 1; /* real drink order, not closing signal */
                order.customer_id = customer_id;
                for (j = 0; j < DRINK_SIZE; j++) {
                        order.requested[j] = NO_INGREDIENT; /* clear request */
                        order.contents[j] = NO_INGREDIENT;  /* clear glass */
                }

                order.requested[0] = BEER; /* this driver orders one beer */
                bar_customer_order(&order); /* blocks until drink is served */

                for (j = 0; j < DRINK_SIZE; j++) {
                        if (order.contents[j] != order.requested[j]) {
                                panic("customer %lu received wrong drink\n",
                                      customer_id);
                        }
                }
                record_bottle_use(order.requested); /* update served doses */
        }

        sem_destroy(order.finished); /* no more orders from this customer */
        bar_customer_done();         /* may trigger bartender shutdown */
        V(bar_finished);             /* tell runbar this customer is done */
}

static void
staff(void *unused, unsigned long staff_id)
{
        struct bar_order *order;
        int mixed;

        (void)unused;
        mixed = 0; /* local count, no shared lock needed */

        while (1) {
                order = bar_get_order(); /* blocks until customer/close order */
                if (!order->valid) {
                        break; /* invalid order means bar is closing */
                }
                bar_fill_order(order); /* mix drink and wake customer */
                mixed++;
        }

        kprintf("S %lu going home after mixing %d drinks\n", staff_id, mixed);
        V(bar_finished); /* tell runbar this staff thread is done */
}

int
runbar(int nargs, char **args)
{
        int i, error;

        (void)nargs;
        (void)args;

        bar_finished = sem_create("bar finished", 0); /* joins staff/customers */
        KASSERT(bar_finished != NULL);

        stats_lock = lock_create("bar stats lock"); /* protects bottle_count */
        KASSERT(stats_lock != NULL);

        for (i = 1; i <= NBOTTLES; i++) {
                bottle_count[i] = 0; /* reset nightly bottle statistics */
        }

        bar_open(); /* initialize queue, customer count, bottle locks */

        for (i = 0; i < BAR_STAFF; i++) {
                error = thread_fork("bar staff", NULL, staff, NULL, i);
                if (error) {
                        panic("bar staff: thread_fork failed: %s\n",
                              strerror(error));
                }
        }

        for (i = 0; i < BAR_CUSTOMERS; i++) {
                error = thread_fork("bar customer", NULL, customer, NULL, i);
                if (error) {
                        panic("bar customer: thread_fork failed: %s\n",
                              strerror(error));
                }
        }

        for (i = 0; i < BAR_STAFF + BAR_CUSTOMERS; i++) {
                P(bar_finished); /* wait for every staff/customer thread */
        }

        for (i = 1; i <= NBOTTLES; i++) {
                kprintf("Bottle %d used for %d doses\n", i, bottle_count[i]);
        }

        bar_close(); /* destroy bar synchronization objects */
        lock_destroy(stats_lock);
        sem_destroy(bar_finished);

        return 0;
}
