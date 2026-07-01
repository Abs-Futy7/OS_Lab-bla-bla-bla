# OS/161 Assignment 1: Synchronization

This README explains the final Assignment 1 implementation: what files were
added or changed, how menu commands `1a`, `1b`, and `1c` are connected, how the
code works, how to build/test, and common viva questions.

## Final Implemented Parts

| Menu | Problem | Main implementation |
| --- | --- | --- |
| `1a` | Concurrent Mathematics | `os161-1.99/kern/asst1/math.c` |
| `1b` | Bounded-buffer Producer/Consumer | `os161-1.99/kern/asst1/producerconsumer.c` |
| `1c` | Bar Synchronization | `os161-1.99/kern/asst1/bar.c` |

Part 3 is **Bar Synchronization**, so `1c` runs `runbar()`.

## Files Added

Assignment framework and solution files:

```txt
os161-1.99/kern/asst1/math.c
os161-1.99/kern/asst1/math_tester.c
os161-1.99/kern/asst1/math_tester.h
os161-1.99/kern/asst1/producerconsumer.c
os161-1.99/kern/asst1/producerconsumer_driver.c
os161-1.99/kern/asst1/producerconsumer_driver.h
os161-1.99/kern/asst1/bar.c
os161-1.99/kern/asst1/bar.h
os161-1.99/kern/asst1/bar_driver.c
os161-1.99/kern/asst1/bar_driver.h
```

Documentation:

```txt
ASSIGNMENT1_README.md
Assignment1_Report.tex
Assignment1_Report.pdf
os161-1.99/design.txt
screenshots/
```

## Files Changed

### `build-and-run-kernel.sh`

Changed:

```sh
ASSIGNMENT=ASST0
```

to:

```sh
ASSIGNMENT=ASST1
```

Reason: the helper script now builds the ASST1 kernel.

### `root/sys161.conf`

Changed RAM to 2 MiB:

```txt
31 mainboard  ramsize=2097152  cpus=1
```

Reason: the assignment recommends at least 2 MiB for ASST1 tests.

### `os161-1.99/kern/conf/conf.kern`

Added ASST1 files under `synchprobs`:

```txt
optfile   synchprobs  asst1/bar.c
optfile   synchprobs  asst1/bar_driver.c
optfile   synchprobs  asst1/math.c
optfile   synchprobs  asst1/math_tester.c
optfile   synchprobs  asst1/producerconsumer.c
optfile   synchprobs  asst1/producerconsumer_driver.c
```

Reason: OS/161 only compiles files that are listed in the kernel config system.

### `os161-1.99/kern/include/test.h`

Added prototypes:

```c
int maths(int, char **);
int run_producerconsumer(int, char **);
int runbar(int, char **);
```

Reason: `menu.c` calls these functions.

### `os161-1.99/kern/startup/menu.c`

Added menu text:

```c
"[1a] Simple math synchronisation    ",
"[1b] Producer/consumer problem      ",
"[1c] Bar Synchronization            ",
```

Added command mappings:

```c
{ "1a", maths },
{ "1b", run_producerconsumer },
{ "1c", runbar },
```

Reason: this is how typing `1a`, `1b`, or `1c` at the OS/161 prompt calls the
assignment code.

### `os161-1.99/kern/include/synch.h` and `os161-1.99/kern/thread/synch.c`

Completed OS/161 locks and condition variables. This was necessary because the
assignment solutions use locks.

## How Menu Commands Work

The menu command path is:

1. `kern/conf/ASST1` enables:

```txt
options synchprobs
```

2. `kern/conf/conf.kern` lists the ASST1 source files as `optfile synchprobs`.

3. `kern/startup/menu.c` maps text commands to C functions:

```c
{ "1a", maths },
{ "1b", run_producerconsumer },
{ "1c", runbar },
```

So when the user types:

```txt
OS/161 kernel [? for menu]: 1c
```

the dispatcher calls:

```c
runbar(nargs, args);
```

## Build

Start Docker Desktop. Then from PowerShell:

```powershell
.\start-interactive-cs350-shell.ps1
```

Inside Docker:

```sh
cd /root/cs350-os161
bash build-and-run-kernel.sh
```

Manual build:

```sh
cd /root/cs350-os161/os161-1.99
./configure --ostree=/root/cs350-os161/root --toolprefix=cs350-
cd kern/conf
./config ASST1
cd ../compile/ASST1
bmake depend
bmake
bmake install
```

Build status: the ASST1 kernel compiled successfully after adding Bar
Synchronization.

## Run Tests

If using GDB/tmux, type `c` in the GDB pane first. Then go to the OS/161 pane:

```txt
Ctrl+B, then Left Arrow
```

Run:

```txt
1a
1b
1c
q
```

Expected:

```txt
1a: total increments = 10000
1b: all consumers finish normally
1c: staff go home, bottle stats print, bar closes
```

Example `1c` style:

```txt
S 2 going home after mixing 42 drinks
S 1 going home after mixing 28 drinks
S 0 going home after mixing 30 drinks
Bottle 1 used for 100 doses
Bottle 2 used for 0 doses
...
The bar is closed, bye!!!
```

The exact number of drinks mixed by each staff member may vary because thread
scheduling is nondeterministic.

## Part 1: Concurrent Mathematics

File:

```txt
os161-1.99/kern/asst1/math.c
```

### Problem

Ten adder threads increment a shared global counter until it reaches 10000.
Without synchronization, two threads can read the same old value and both write
the same new value, causing a lost update.

### Important Data

```c
volatile unsigned long int counter;
unsigned long int adder_counters[NADDERS];
struct semaphore *finished;
struct lock *math_lock;
```

### Solution

`math_lock` protects the critical section:

```c
lock_acquire(math_lock);
a = counter;
if (a < NADDS) {
        counter = counter + 1;
        b = counter;
        lock_release(math_lock);
        adder_counters[addernumber]++;
} else {
        lock_release(math_lock);
        flag = 0;
}
```

The main thread waits for all adder threads using `finished`.

### Why Correct

Only one thread can read/check/increment `counter` at a time. Therefore no
increments are lost and the final total is exactly 10000.

## Part 2: Producer/Consumer

File:

```txt
os161-1.99/kern/asst1/producerconsumer.c
```

### Problem

Producer threads insert data into a fixed-size buffer. Consumer threads remove
data. Producers must block if the buffer is full. Consumers must block if the
buffer is empty.

### Data

```c
static data_item_t *item_buffer[BUFFER_SIZE];
static int producer_index, consumer_index;
static struct semaphore *mutex, *empty, *full;
```

### Producer Logic

```c
P(empty);
P(mutex);
item_buffer[producer_index] = item;
producer_index = (producer_index + 1) % BUFFER_SIZE;
V(mutex);
V(full);
```

### Consumer Logic

```c
P(full);
P(mutex);
item = item_buffer[consumer_index];
consumer_index = (consumer_index + 1) % BUFFER_SIZE;
V(mutex);
V(empty);
```

### Why Correct

`empty` tracks free slots, `full` tracks occupied slots, and `mutex` protects
the circular-buffer indices. This prevents overwriting, empty reads, and busy
waiting.

## Part 3: Bar Synchronization

Files:

```txt
os161-1.99/kern/asst1/bar.c
os161-1.99/kern/asst1/bar.h
os161-1.99/kern/asst1/bar_driver.c
os161-1.99/kern/asst1/bar_driver.h
```

### Problem

Customers place drink orders. Bartenders consume those orders, mix the drinks,
and return the correct glass to the customer. When all customers are done, all
bartenders must go home. Bottle usage statistics must match the drinks served.

### Shared Resources

The shared resources are:

```txt
order queue
order queue indices
bottles
bottle statistics
number of customers still in the bar
```

### Main Data Structures

In `bar_driver.h`:

```c
struct bar_order {
        int valid;
        int customer_id;
        int requested[DRINK_SIZE];
        int contents[DRINK_SIZE];
        struct semaphore *finished;
};
```

In `bar.c`:

```c
static struct bar_order *order_buffer[ORDER_BUFFER_SIZE];
static int order_in;
static int order_out;
static int customers_left;

static struct semaphore *empty_orders;
static struct semaphore *full_orders;
static struct lock *order_lock;
static struct lock *bottle_locks[NBOTTLES + 1];
```

### Order Queue

The bar uses a bounded circular queue. Customers produce orders. Bartenders
consume orders.

Insert:

```c
P(empty_orders);
lock_acquire(order_lock);
order_buffer[order_in] = order;
order_in = (order_in + 1) % ORDER_BUFFER_SIZE;
lock_release(order_lock);
V(full_orders);
```

Remove:

```c
P(full_orders);
lock_acquire(order_lock);
order = order_buffer[order_out];
order_out = (order_out + 1) % ORDER_BUFFER_SIZE;
lock_release(order_lock);
V(empty_orders);
```

### Customer Flow

A customer prepares an order and calls:

```c
bar_customer_order(&order);
```

That function enqueues the order and waits:

```c
enqueue_order(order);
P(order->finished);
```

The customer blocks until a bartender fills the order and signals the order's
`finished` semaphore.

### Bartender Flow

A bartender repeatedly calls:

```c
order = bar_get_order();
```

If `order->valid == 0`, the bartender goes home. Otherwise the bartender calls:

```c
bar_fill_order(order);
```

and increments its local mixed-drink count.

### Bottle Synchronization

Each bottle has its own lock:

```c
static struct lock *bottle_locks[NBOTTLES + 1];
```

When a drink needs bottles, the code sorts bottle numbers and acquires bottle
locks in increasing order. This prevents deadlock.

Example:

```c
sort_bottles(bottles, nbottles);
lock_acquire(bottle_locks[bottles[i]]);
```

After the drink is mixed, locks are released in reverse order.

### Closing the Bar

`customers_left` counts customers still active. When the last customer leaves,
the solution enqueues one invalid order per bartender:

```c
closing_orders[i].valid = 0;
enqueue_order(&closing_orders[i]);
```

These invalid orders tell bartenders to stop waiting and go home.

### Why Correct

The order queue is protected with semaphores and a lock, so orders are neither
lost nor overwritten. Each customer waits on its own order semaphore, so it
does not leave before receiving the correct drink. Bottle locks protect bottle
usage and allow different bartenders to mix in parallel when they need
different bottles. A fixed bottle-lock order prevents deadlock. Invalid closing
orders ensure every bartender exits after all customers are finished.

## Viva Questions and Answers

### Q1. What is synchronization?

Synchronization controls concurrent access to shared resources so that threads
do not corrupt data or violate ordering rules.

### Q2. What is a race condition?

A race condition occurs when the output depends on unpredictable thread timing.

### Q3. Where was the race in `math.c`?

The global `counter` was shared by all adder threads.

### Q4. How did you fix the math race?

By protecting the counter read/check/increment critical section with
`math_lock`.

### Q5. Why is `adder_counters[]` safe?

Each thread updates only its own slot, so two threads do not write the same
array element.

### Q6. What does `P()` do?

It decrements a semaphore. If the count is zero, the thread sleeps.

### Q7. What does `V()` do?

It increments a semaphore and wakes a waiting thread if one exists.

### Q8. Why does producer/consumer use `empty`, `full`, and `mutex`?

`empty` counts free slots, `full` counts stored items, and `mutex` protects the
buffer indices.

### Q9. Why is the producer/consumer buffer circular?

It reuses a fixed-size array by wrapping indices with modulo arithmetic.

### Q10. What happens if a consumer finds no item?

It blocks on `P(full)` until a producer inserts an item.

### Q11. What happens if a producer finds no free slot?

It blocks on `P(empty)` until a consumer removes an item.

### Q12. What are the shared resources in the bar problem?

The order queue, queue indices, bottles, statistics, and active customer count.

### Q13. Who are producers and consumers in the bar?

Customers produce drink orders. Bartenders consume those orders.

### Q14. How does a customer wait for a drink?

Each order has a `finished` semaphore. The customer sleeps on it until the
bartender signals it.

### Q15. How does a bartender know to go home?

After the last customer leaves, invalid orders are enqueued. When a bartender
receives an invalid order, it exits.

### Q16. Why one invalid order per bartender?

Because each invalid order wakes and stops one bartender. With three
bartenders, three invalid orders are needed.

### Q17. Why does each bottle have a lock?

So bartenders using different bottles can work in parallel, while bartenders
using the same bottle are serialized.

### Q18. Why sort bottle numbers before locking?

To acquire bottle locks in a consistent global order and prevent deadlock.

### Q19. What deadlock could happen without sorted bottle locking?

One bartender could hold bottle 1 and wait for bottle 2 while another holds
bottle 2 and waits for bottle 1.

### Q20. Why are operation times different between runs?

Thread scheduling and timer interrupts are nondeterministic.

### Q21. Why are staff drink counts different between runs?

Bartenders race to take orders from the queue, so the work distribution depends
on scheduling.

### Q22. What output proves `1c` worked?

All staff go home, bottle counts are printed, and the final line says:

```txt
The bar is closed, bye!!!
```

### Q23. What is a lock?

A lock is a mutual exclusion primitive. Only one thread can hold it at a time.

### Q24. What is a wait channel?

A wait channel is where blocked threads sleep until another thread wakes them.

### Q25. Why did you implement locks and CVs?

The local OS/161 tree had incomplete synchronization primitives, and the ASST1
solutions depend on working locks.

### Q26. What does `lock_do_i_hold()` do?

It returns whether the current thread owns the lock.

### Q27. What is a condition variable?

A condition variable lets a thread sleep until a condition over shared state
becomes true. It is used with a lock.

### Q28. What does `cv_wait()` do?

It releases the lock, sleeps, and reacquires the lock after waking.

### Q29. Why did you edit `conf.kern`?

To tell OS/161 to compile the ASST1 source files into the kernel.

### Q30. Why did you edit `menu.c`?

To expose the assignment tests as menu commands `1a`, `1b`, and `1c`.

### Q31. Why did your output differ from a friend's earlier?

Different source trees can wire `1c` to different assignment code. In this
final project, `1c` is wired to Bar Synchronization as required by the
assignment text.

### Q32. What is the main lesson of the assignment?

Find shared state, protect critical sections, use blocking synchronization
instead of busy waiting, and prevent deadlock with careful lock ordering.

## Code Walkthrough Viva Questions

This section is for questions where sir points to a function or variable and
asks, "What is this doing?"

### Q33. In short, how does the whole assignment run?

The OS/161 menu calls one of three driver functions:

```c
1a -> maths()
1b -> run_producerconsumer()
1c -> runbar()
```

Each driver creates kernel threads with `thread_fork()`. Those threads access
shared data, so the solution uses locks and semaphores to make the access safe.

### Q34. What does `maths()` do?

`maths()` initializes the shared counter, creates the `finished` semaphore,
creates `math_lock`, starts 10 adder threads, waits for all of them to finish,
prints statistics, and destroys the synchronization objects.

### Q35. What does `adder()` do?

Each `adder()` thread repeatedly tries to increment the global `counter`.
Before touching `counter`, it acquires `math_lock`. If `counter` is still below
10000, it increments it. If the counter already reached 10000, it releases the
lock and exits.

### Q36. Why is `counter` protected but `adder_counters[addernumber]` is not?

`counter` is written by all adder threads, so it is shared. But each
`adder_counters[addernumber]` slot belongs to only one thread number, so no two
threads update the same slot.

### Q37. What is `finished` in `math.c`?

`finished` is a semaphore used as a join mechanism. Every adder calls:

```c
V(finished);
```

before exiting. The main `maths()` function calls:

```c
P(finished);
```

ten times, so it waits until all ten adders are done.

### Q38. What would happen if `math_lock` was removed?

Two threads could read the same old value of `counter` and both write the same
new value. This would lose increments and the final total might be less than
10000.

### Q39. What does `producerconsumer_startup()` do?

It initializes the circular buffer indices and creates three semaphores:

```c
mutex = sem_create("mutex", 1);
empty = sem_create("empty", BUFFER_SIZE);
full = sem_create("full", 0);
```

Initially the buffer is empty, so all slots are empty and zero slots are full.

### Q40. What does `producer_send()` do?

`producer_send()` inserts one item into the bounded buffer. It first waits for
an empty slot with `P(empty)`, then locks the buffer with `P(mutex)`, stores the
item, advances `producer_index`, unlocks with `V(mutex)`, and signals
availability with `V(full)`.

### Q41. Why is `P(empty)` before `P(mutex)` in `producer_send()`?

A producer should wait for space before entering the critical section. If it
took `mutex` first and then blocked on `empty`, consumers could be prevented
from acquiring `mutex` to remove an item, causing deadlock.

### Q42. What does `consumer_receive()` do?

`consumer_receive()` removes one item from the buffer. It waits for an
available item with `P(full)`, enters the critical section with `P(mutex)`,
reads the item, advances `consumer_index`, leaves the critical section, and
signals one more empty slot with `V(empty)`.

### Q43. Why is modulo used for `producer_index` and `consumer_index`?

Modulo makes the array circular:

```c
producer_index = (producer_index + 1) % BUFFER_SIZE;
consumer_index = (consumer_index + 1) % BUFFER_SIZE;
```

When an index reaches the end of the array, it wraps back to zero.

### Q44. What does `producerconsumer_shutdown()` do?

It destroys the semaphores created during startup:

```c
sem_destroy(mutex);
sem_destroy(empty);
sem_destroy(full);
```

This is cleanup after the simulation ends.

### Q45. What does `run_producerconsumer()` do?

It creates driver semaphores, calls `producerconsumer_startup()`, starts
consumer and producer threads, waits for producers to finish, sends stop
messages to consumers, waits for consumers, then calls
`producerconsumer_shutdown()`.

### Q46. What are stop messages in producer/consumer?

The driver sends special items with:

```c
data1 = 0;
data2 = 0;
```

Consumers interpret this as a signal to exit normally. The buffer code itself
does not depend on the content; it simply transports items.

### Q47. What does `bar_open()` do?

`bar_open()` initializes the bar's shared state:

```c
order_in = 0;
order_out = 0;
customers_left = BAR_CUSTOMERS;
```

It creates the bounded-order-queue semaphores, creates the queue lock, and
creates one lock per bottle.

### Q48. What are `empty_orders` and `full_orders`?

They are counting semaphores for the bar order queue. `empty_orders` counts
free queue slots. `full_orders` counts orders waiting for bartenders.

### Q49. What does `enqueue_order()` do?

It adds an order to the circular order queue:

```c
P(empty_orders);
lock_acquire(order_lock);
order_buffer[order_in] = order;
order_in = (order_in + 1) % ORDER_BUFFER_SIZE;
lock_release(order_lock);
V(full_orders);
```

It blocks if the queue is full.

### Q50. What does `dequeue_order()` do?

It removes the next order from the circular order queue:

```c
P(full_orders);
lock_acquire(order_lock);
order = order_buffer[order_out];
order_out = (order_out + 1) % ORDER_BUFFER_SIZE;
lock_release(order_lock);
V(empty_orders);
```

It blocks if no order is available.

### Q51. What does `bar_customer_order()` do?

It represents a customer placing an order:

```c
enqueue_order(order);
P(order->finished);
```

The customer queues the order and then sleeps until a bartender completes it.

### Q52. Why does every `bar_order` have a `finished` semaphore?

Each customer needs to wait for its own drink, not someone else's drink. A
private semaphore lets the bartender wake exactly the customer whose order was
filled.

### Q53. What does `bar_get_order()` do?

It is the bartender-side function for taking an order. It simply calls
`dequeue_order()` and returns the next order to the bartender.

### Q54. What does `bar_fill_order()` do?

It fills a customer's glass. It collects the requested bottle numbers, sorts
them, locks the required bottles, copies:

```c
order->contents[i] = order->requested[i];
```

then releases the bottle locks and wakes the customer with:

```c
V(order->finished);
```

### Q55. Why does `bar_fill_order()` sort bottle numbers?

Sorting ensures all bartenders acquire bottle locks in the same order. This
prevents circular wait and therefore prevents deadlock.

### Q56. Why does `bar_fill_order()` skip duplicate bottle locks?

If the same bottle appears more than once in a drink, the code should not try
to acquire the same non-recursive lock twice. The duplicate check avoids
self-deadlock.

### Q57. Why release bottle locks after copying the contents?

The bottle locks protect the mixing operation. Once the requested ingredients
have been copied into the glass, the bartender no longer needs the bottles.
Releasing quickly allows other bartenders to use them.

### Q58. What does `bar_customer_done()` do?

It decrements `customers_left`. If this was the last customer, it enqueues one
invalid order for each bartender so all staff threads can wake up and exit.

### Q59. Why not hold `order_lock` while enqueuing closing orders?

`enqueue_order()` itself acquires `order_lock`. Holding `order_lock` and then
calling `enqueue_order()` would try to acquire the same lock again and could
deadlock.

### Q60. What does `bar_close()` do?

It destroys all bottle locks, destroys the order lock, destroys the queue
semaphores, and prints:

```txt
The bar is closed, bye!!!
```

### Q61. What does `runbar()` do?

`runbar()` is the main driver for `1c`. It creates the completion semaphore,
initializes bottle statistics, calls `bar_open()`, starts staff threads, starts
customer threads, waits for all of them to finish, prints bottle usage, and
calls `bar_close()`.

### Q62. What does the `customer()` thread in `bar_driver.c` do?

Each customer creates a private semaphore, places `DRINKS_PER_CUSTOMER` orders,
checks that the returned contents match the requested drink, records bottle
statistics, destroys its semaphore, calls `bar_customer_done()`, and signals
`bar_finished`.

### Q63. What does the `staff()` thread in `bar_driver.c` do?

Each staff thread repeatedly gets an order. If the order is valid, it fills the
order and increments its mixed count. If the order is invalid, it prints how
many drinks it mixed and exits.

### Q64. What does `record_bottle_use()` do?

It updates the bottle usage counters. It uses `stats_lock` because multiple
customer threads can record statistics concurrently.

### Q65. Why are bottle statistics recorded by customers after receiving drinks?

The customer knows the requested drink and verifies the returned contents. Once
the drink is correct, the driver records the ingredients as served doses.

### Q66. Why does Bottle 1 show 100 doses?

In this driver, there are 10 customers and each orders 10 drinks. Each drink
uses `BEER`, which is bottle 1. Therefore:

```txt
10 customers * 10 drinks = 100 doses
```

### Q67. Why can staff mixed counts be different every run?

All staff threads compete to dequeue orders. The scheduler decides which thread
runs first, so one staff member may process more orders than another.

### Q68. What part makes sure all bartenders participate?

Multiple staff threads are created in `runbar()`, and all wait on the same
order queue. Since orders are added throughout the run, different staff threads
can wake and process them depending on scheduling.

### Q69. What part prevents customers from getting empty glasses?

Customers wait on `order->finished`. A bartender calls `V(order->finished)`
only after `bar_fill_order()` copies requested ingredients into `contents`.

### Q70. What part prevents orders from being lost?

The order queue is bounded and synchronized. `empty_orders`, `full_orders`, and
`order_lock` ensure inserts/removes happen safely.

### Q71. What part prevents bartenders from waiting forever?

When the last customer leaves, `bar_customer_done()` enqueues one invalid order
per bartender. These invalid orders wake bartenders and tell them to exit.

### Q72. What would happen if only one invalid order was sent?

Only one bartender would exit. The other bartenders could remain blocked on
`full_orders`, waiting forever.

### Q73. What would happen if bottle locks were not used?

Two bartenders could use the same bottle at the same time, making bottle access
and statistics inconsistent with the model.

### Q74. What would happen if all bottles used one global lock?

It would be correct but less parallel. Bartenders using different bottles would
still block each other unnecessarily.

### Q75. Why is one lock per bottle better?

It allows bartenders to mix in parallel when their drinks need different
bottles, while still protecting each individual bottle.

### Q76. What does `valid` mean in `struct bar_order`?

`valid == 1` means this is a real customer order. `valid == 0` means this is a
closing order telling a bartender to go home.

### Q77. What does `requested[]` mean?

`requested[]` stores the ingredients the customer asked for.

### Q78. What does `contents[]` mean?

`contents[]` stores the ingredients actually placed into the glass by the
bartender.

### Q79. How is correctness checked for drinks?

The customer thread checks:

```c
order.contents[j] == order.requested[j]
```

for every ingredient slot.

### Q80. What is the most important function in the bar solution?

`bar_fill_order()` is central because it handles bottle locking, fills the
drink, and wakes the waiting customer. `enqueue_order()` and `dequeue_order()`
are also central because they synchronize customer-bartender communication.
