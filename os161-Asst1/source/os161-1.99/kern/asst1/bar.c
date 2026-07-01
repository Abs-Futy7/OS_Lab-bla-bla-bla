/* Bar synchronization solution. */
#include <types.h>
#include <lib.h>
#include <synch.h>
#include "bar_driver.h"

static struct bar_order *order_buffer[ORDER_BUFFER_SIZE];
static int order_in;
static int order_out;
static int customers_left;

static struct semaphore *empty_orders;
static struct semaphore *full_orders;
static struct lock *order_lock;
static struct lock *bottle_locks[NBOTTLES + 1];

static void
enqueue_order(struct bar_order *order)
{
        P(empty_orders);
        lock_acquire(order_lock);
        order_buffer[order_in] = order;
        order_in = (order_in + 1) % ORDER_BUFFER_SIZE;
        lock_release(order_lock);
        V(full_orders);
}

static struct bar_order *
dequeue_order(void)
{
        struct bar_order *order;

        P(full_orders);
        lock_acquire(order_lock);
        order = order_buffer[order_out];
        order_out = (order_out + 1) % ORDER_BUFFER_SIZE;
        lock_release(order_lock);
        V(empty_orders);

        return order;
}

void
bar_open(void)
{
        int i;

        order_in = 0;
        order_out = 0;
        customers_left = BAR_CUSTOMERS;

        empty_orders = sem_create("bar empty orders", ORDER_BUFFER_SIZE);
        KASSERT(empty_orders != NULL);

        full_orders = sem_create("bar full orders", 0);
        KASSERT(full_orders != NULL);

        order_lock = lock_create("bar order lock");
        KASSERT(order_lock != NULL);

        for (i = 1; i <= NBOTTLES; i++) {
                bottle_locks[i] = lock_create("bar bottle lock");
                KASSERT(bottle_locks[i] != NULL);
        }
}

void
bar_close(void)
{
        int i;

        for (i = 1; i <= NBOTTLES; i++) {
                lock_destroy(bottle_locks[i]);
        }

        lock_destroy(order_lock);
        sem_destroy(full_orders);
        sem_destroy(empty_orders);

        kprintf("The bar is closed, bye!!!\n");
}

void
bar_customer_order(struct bar_order *order)
{
        enqueue_order(order);
        P(order->finished);
}

void
bar_customer_done(void)
{
        int i;
        int last_customer;
        static struct bar_order closing_orders[BAR_STAFF];

        lock_acquire(order_lock);
        customers_left--;
        last_customer = customers_left == 0;
        lock_release(order_lock);

        if (last_customer) {
                for (i = 0; i < BAR_STAFF; i++) {
                        closing_orders[i].valid = 0;
                        enqueue_order(&closing_orders[i]);
                }
        }
}

struct bar_order *
bar_get_order(void)
{
        return dequeue_order();
}

static void
sort_bottles(int bottles[], int nbottles)
{
        int i, j, tmp;

        for (i = 0; i < nbottles; i++) {
                for (j = i + 1; j < nbottles; j++) {
                        if (bottles[j] < bottles[i]) {
                                tmp = bottles[i];
                                bottles[i] = bottles[j];
                                bottles[j] = tmp;
                        }
                }
        }
}

void
bar_fill_order(struct bar_order *order)
{
        int bottles[DRINK_SIZE];
        int nbottles;
        int i;

        nbottles = 0;
        for (i = 0; i < DRINK_SIZE; i++) {
                if (order->requested[i] != NO_INGREDIENT) {
                        KASSERT(order->requested[i] >= 1);
                        KASSERT(order->requested[i] <= NBOTTLES);
                        bottles[nbottles++] = order->requested[i];
                }
        }

        sort_bottles(bottles, nbottles);

        for (i = 0; i < nbottles; i++) {
                if (i == 0 || bottles[i] != bottles[i - 1]) {
                        lock_acquire(bottle_locks[bottles[i]]);
                }
        }

        for (i = 0; i < DRINK_SIZE; i++) {
                order->contents[i] = order->requested[i];
        }

        for (i = nbottles - 1; i >= 0; i--) {
                if (i == 0 || bottles[i] != bottles[i - 1]) {
                        lock_release(bottle_locks[bottles[i]]);
                }
        }

        V(order->finished);
}
