#include <stdio.h>   /* Provides printf() and scanf() for input/output. */
#include <stdlib.h>  /* Provides rand() and srand() for lottery scheduling. */
#include <time.h>    /* Provides time(), used to seed the random number generator. */

#define MAX 10       /* Maximum number of processes this program can store. */

/*
 * First-Come, First-Served scheduling.
 * Processes are executed in the same order in which the user enters them.
 * This version assumes every process arrives at time 0.
 */
void fcfs(int n, int burst[]) {
    int wt[MAX], tat[MAX];                 /* wt = waiting time, tat = turnaround time. */
    float avg_wt = 0, avg_tat = 0;         /* These accumulate totals before averaging. */

    wt[0] = 0;                             /* The first process starts immediately, so it waits 0 ms. */
    for (int i = 1; i < n; i++)            /* Start from the second process. */
        wt[i] = wt[i-1] + burst[i-1];      /* Current waiting time = previous waiting time + previous burst. */

    printf("\n===== FCFS Scheduling =====\n");  /* Print section title. */
    printf("%-10s %-15s %-15s %-15s\n", "Process", "Burst(ms)", "Waiting(ms)", "Turnaround(ms)");
                                             /* Print formatted table column names. */
    printf("--------------------------------------------------------------\n"); /* Print table separator. */

    for (int i = 0; i < n; i++) {          /* Process each entered process in original order. */
        tat[i] = wt[i] + burst[i];         /* Turnaround time = waiting time + burst time. */
        avg_wt  += wt[i];                  /* Add this process waiting time to total waiting time. */
        avg_tat += tat[i];                 /* Add this process turnaround time to total turnaround time. */
        printf("P%-9d %-15d %-15d %-15d\n", i+1, burst[i], wt[i], tat[i]);
                                             /* Print process number, burst, waiting, and turnaround time. */
    }

    printf("\nAverage Waiting Time    : %.2f ms\n", avg_wt  / n); /* Print average waiting time. */
    printf("Average Turnaround Time : %.2f ms\n",  avg_tat / n); /* Print average turnaround time. */
}


/*
 * Shortest Job First scheduling.
 * This is non-preemptive: once a process starts, it runs until completion.
 * This version assumes all processes arrive at time 0.
 */
void sjf(int n, int burst[]) {
    int b[MAX], idx[MAX], wt[MAX], tat[MAX]; /* b = sorted burst copy, idx = original process number. */
    float avg_wt = 0, avg_tat = 0;           /* Totals for average waiting and turnaround time. */

    for (int i = 0; i < n; i++) {            /* Copy original data into helper arrays. */
        b[i] = burst[i];                     /* Store burst time in b so original burst[] is unchanged. */
        idx[i] = i+1;                        /* Store original process number: P1, P2, P3, ... */
    }

    for (int i = 1; i < n; i++) {            /* Insertion sort starts from second element. */
        int key_b = b[i];                    /* Burst time currently being inserted into sorted position. */
        int key_idx = idx[i];                /* Original process number of that burst time. */
        int j = i - 1;                       /* Compare with elements before i. */

        while (j >= 0 && b[j] > key_b) {     /* Move larger burst times one position to the right. */
            b[j + 1] = b[j];                 /* Shift burst time right. */
            idx[j + 1] = idx[j];             /* Shift matching process number right too. */
            j--;                             /* Move left to continue checking sorted portion. */
        }

        b[j + 1] = key_b;                    /* Insert burst time into correct sorted position. */
        idx[j + 1] = key_idx;                /* Insert its matching original process number. */
    }

    wt[0] = 0;                               /* First scheduled process waits 0 ms. */
    for (int i = 1; i < n; i++)              /* Calculate waiting time after sorting. */
        wt[i] = wt[i-1] + b[i-1];            /* Current waiting = previous waiting + previous sorted burst. */

    printf("\n===== SJF Scheduling =====\n");   /* Print section title. */
    printf("%-10s %-15s %-15s %-15s\n", "Process", "Burst(ms)", "Waiting(ms)", "Turnaround(ms)");
                                               /* Print formatted table header. */
    printf("--------------------------------------------------------------\n"); /* Print table separator. */

    for (int i = 0; i < n; i++) {            /* Print each process in SJF order. */
        tat[i]   = wt[i] + b[i];             /* Turnaround time = waiting time + burst time. */
        avg_wt  += wt[i];                    /* Add waiting time to total. */
        avg_tat += tat[i];                   /* Add turnaround time to total. */
        printf("P%-9d %-15d %-15d %-15d\n", idx[i], b[i], wt[i], tat[i]);
                                               /* Print original process number with sorted burst result. */
    }

    printf("\nAverage Waiting Time    : %.2f ms\n", avg_wt  / n); /* Print average waiting time. */
    printf("Average Turnaround Time : %.2f ms\n",  avg_tat / n); /* Print average turnaround time. */
}


/*
 * Shortest Remaining Time Next scheduling.
 * This is the preemptive version of SJF.
 * At every time unit, the ready process with the smallest remaining time is selected.
 */
void srtn(int n, int arrival[], int burst[]) {
    int rem[MAX], done[MAX], complete[MAX], wt[MAX], tat[MAX];
                                               /* rem = remaining time, done = finished flag, complete = finish time. */
    float avg_wt = 0, avg_tat = 0;             /* Totals for average waiting and turnaround time. */

    for (int i = 0; i < n; i++) {              /* Initialize every process. */
        rem[i] = burst[i];                     /* At first, remaining time equals full burst time. */
        done[i] = 0;                           /* 0 means this process is not completed yet. */
    }

    int finished = 0;                          /* Number of processes that have completed. */
    int t = 0;                                 /* Current CPU time, starting from 0. */

    while (finished < n) {                     /* Continue until all processes are completed. */
        int sel = -1;                          /* Selected process index; -1 means no process selected yet. */
        int minR = 99999;                      /* Large value used to find the minimum remaining time. */

        for (int i = 0; i < n; i++)            /* Check all processes at current time t. */
            if (!done[i] && arrival[i] <= t && rem[i] < minR) {
                                                   /* Process must be unfinished, already arrived, and shortest so far. */
                minR = rem[i];                 /* Update smallest remaining time found. */
                sel = i;                       /* Select this process to run next. */
            }

        if (sel == -1) {                       /* If no process has arrived yet. */
            t++;                               /* CPU stays idle for 1 ms. */
            continue;                          /* Go back and check again at next time. */
        }

        rem[sel]--;                            /* Run selected process for exactly 1 ms. */
        t++;                                   /* Increase current time by 1 ms. */

        if (rem[sel] == 0) {                   /* If selected process has finished. */
            done[sel] = 1;                     /* Mark process as completed. */
            complete[sel] = t;                 /* Store completion time. */
            finished++;                        /* Increase completed process count. */
        }
    }

    printf("\n===== SRTN Scheduling =====\n");  /* Print section title. */
    printf("%-10s %-12s %-12s %-14s %-12s %-14s\n",
           "Process","Arrival","Burst","Completion","Waiting","Turnaround");
                                               /* Print formatted table header. */
    printf("----------------------------------------------------------------------\n"); /* Print table separator. */

    for (int i = 0; i < n; i++) {              /* Calculate and print result for each process. */
        tat[i]   = complete[i] - arrival[i];   /* Turnaround = completion time - arrival time. */
        wt[i]    = tat[i] - burst[i];          /* Waiting = turnaround time - burst time. */
        avg_wt  += wt[i];                      /* Add waiting time to total. */
        avg_tat += tat[i];                     /* Add turnaround time to total. */
        printf("P%-9d %-12d %-12d %-14d %-12d %-14d\n",
               i+1, arrival[i], burst[i], complete[i], wt[i], tat[i]);
                                               /* Print process information and calculated times. */
    }

    printf("\nAverage Waiting Time    : %.2f ms\n", avg_wt  / n); /* Print average waiting time. */
    printf("Average Turnaround Time : %.2f ms\n",  avg_tat / n); /* Print average turnaround time. */
}


/*
 * Round Robin scheduling.
 * Every process gets CPU time for at most "quantum" ms per turn.
 * This version assumes all processes arrive at time 0.
 */
void round_robin(int n, int burst[], int quantum) {
    int rem[MAX], complete[MAX], wt[MAX], tat[MAX];
                                               /* rem = remaining time, complete = completion time. */
    float avg_wt = 0, avg_tat = 0;             /* Totals for average waiting and turnaround time. */

    for (int i = 0; i < n; i++)                /* Initialize remaining time for each process. */
        rem[i] = burst[i];                     /* At first, each process still needs its full burst time. */

    int t = 0;                                 /* Current CPU time. */
    int finished = 0;                          /* Number of completed processes. */

    for (int i = 0; i < n; i++)                /* Initialize completion times. */
        complete[i] = 0;                       /* 0 means not completed yet. */

    printf("\n===== Round Robin Scheduling (Quantum = %d ms) =====\n", quantum);
                                               /* Print section title with time quantum. */
    printf("Gantt Chart: ");                   /* Start printing the execution timeline. */

    while (finished < n) {                     /* Continue while at least one process is unfinished. */
        int did_work = 0;                      /* Tracks whether any process ran during this pass. */

        for (int i = 0; i < n; i++) {          /* Visit processes in order: P1, P2, P3, ... */
            if (rem[i] > 0) {                  /* Only run a process that still has remaining time. */
                did_work = 1;                  /* Mark that CPU work happened in this loop. */
                int run = (rem[i] < quantum) ? rem[i] : quantum;
                                               /* Run for remaining time if less than quantum, else full quantum. */
                printf("[P%d: %d-%d] ", i+1, t, t+run);
                                               /* Print this process's time slice in the Gantt chart. */
                t += run;                      /* Move current time forward by the amount executed. */
                rem[i] -= run;                 /* Subtract executed time from remaining time. */

                if (rem[i] == 0) {             /* If the process just finished. */
                    complete[i] = t;           /* Store its finishing time. */
                    finished++;                /* Increase completed process count. */
                }
            }
        }

        if (!did_work)                         /* Safety check: if no process ran, stop the loop. */
            break;                             /* Prevents infinite loop in unexpected cases. */
    }

    printf("\n\n%-10s %-15s %-15s %-15s\n", "Process","Burst(ms)","Waiting(ms)","Turnaround(ms)");
                                               /* Print table header. */
    printf("--------------------------------------------------------------\n"); /* Print table separator. */

    for (int i = 0; i < n; i++) {              /* Calculate and print result for each process. */
        tat[i]   = complete[i];                /* Arrival is assumed 0, so turnaround = completion time. */
        wt[i]    = tat[i] - burst[i];          /* Waiting = turnaround time - burst time. */
        avg_wt  += wt[i];                      /* Add waiting time to total. */
        avg_tat += tat[i];                     /* Add turnaround time to total. */
        printf("P%-9d %-15d %-15d %-15d\n", i+1, burst[i], wt[i], tat[i]);
                                               /* Print process result row. */
    }

    printf("\nAverage Waiting Time    : %.2f ms\n", avg_wt  / n); /* Print average waiting time. */
    printf("Average Turnaround Time : %.2f ms\n",  avg_tat / n); /* Print average turnaround time. */
}


/*
 * Lottery scheduling.
 * Each process owns some tickets.
 * A random ticket is drawn, and the process owning that ticket gets CPU time.
 */
void lottery(int n, int burst[], int tickets[], int quantum) {
    int rem[MAX], complete[MAX];               /* rem = remaining time, complete = completion time. */
    float avg_wt = 0, avg_tat = 0;             /* Totals for average waiting and turnaround time. */

    for (int i = 0; i < n; i++) {              /* Initialize all processes. */
        rem[i] = burst[i];                     /* Remaining time initially equals burst time. */
        complete[i] = 0;                       /* Completion time is unknown at the start. */
    }

    int t = 0;                                 /* Current CPU time. */
    int finished = 0;                          /* Number of completed processes. */

    printf("\n===== Lottery Scheduling (Quantum = %d ms) =====\n", quantum);
                                               /* Print section title with time quantum. */
    printf("%-8s %-10s %-10s\n", "Time", "Winner", "Ticket");
                                               /* Print table header for each lottery draw. */
    printf("----------------------------------\n");  /* Print table separator. */

    while (finished < n) {                     /* Continue until all processes complete. */
        int total = 0;                         /* Total tickets of unfinished processes. */

        for (int i = 0; i < n; i++)            /* Count tickets only for unfinished processes. */
            if (rem[i] > 0)                    /* Finished processes cannot win anymore. */
                total += tickets[i];           /* Add this process's tickets to total. */

        if (total == 0)                        /* If no tickets are available. */
            break;                             /* Stop scheduling. */

        int draw = rand() % total;             /* Pick a random ticket number from 0 to total-1. */
        int cum  = 0;                          /* Cumulative ticket count while searching winner. */
        int sel = -1;                          /* Selected winning process index. */

        for (int i = 0; i < n; i++) {          /* Find which process owns the drawn ticket. */
            if (rem[i] <= 0)                   /* Skip completed processes. */
                continue;                      /* Move to next process. */

            cum += tickets[i];                 /* Add this process's tickets to cumulative range. */

            if (draw < cum) {                  /* If drawn ticket falls inside this process's range. */
                sel = i;                       /* This process wins the lottery. */
                break;                         /* Stop searching after finding winner. */
            }
        }

        int run = (rem[sel] < quantum) ? rem[sel] : quantum;
                                               /* Winner runs for remaining time or one quantum. */
        printf("t=%-6d P%-6d ticket #%d\n", t, sel+1, draw);
                                               /* Print current time, winning process, and ticket number. */
        t += run;                              /* Move CPU time forward. */
        rem[sel] -= run;                       /* Reduce winner's remaining burst time. */

        if (rem[sel] == 0) {                   /* If winner finished after this run. */
            complete[sel] = t;                 /* Store completion time. */
            finished++;                        /* Increase completed process count. */
        }
    }

    printf("\n%-10s %-12s %-10s %-14s %-12s %-14s\n",
           "Process","Burst(ms)","Tickets","Completion","Waiting","Turnaround");
                                               /* Print final result table header. */
    printf("--------------------------------------------------------------------------\n");
                                               /* Print table separator. */

    for (int i = 0; i < n; i++) {              /* Print final result for each process. */
        int tat = complete[i];                 /* Arrival is assumed 0, so turnaround = completion time. */
        int wt  = tat - burst[i];              /* Waiting = turnaround time - burst time. */
        avg_wt  += wt;                         /* Add waiting time to total. */
        avg_tat += tat;                        /* Add turnaround time to total. */
        printf("P%-9d %-12d %-10d %-14d %-12d %-14d\n",
               i+1, burst[i], tickets[i], complete[i], wt, tat);
                                               /* Print process number, burst, tickets, completion, waiting, turnaround. */
    }

    printf("\nAverage Waiting Time    : %.2f ms\n", avg_wt  / n); /* Print average waiting time. */
    printf("Average Turnaround Time : %.2f ms\n",  avg_tat / n); /* Print average turnaround time. */
}


/*
 * main() displays a menu, takes user input, and calls the selected scheduler.
 */
int main(void) {
    srand((unsigned)time(NULL));               /* Seed rand() so lottery results change between runs. */

    int choice;                                /* Stores user's menu choice. */

    do {                                       /* Repeat menu until user chooses 0. */
        printf("\n========== CPU Scheduling Menu ==========\n");       /* Print menu title. */
        printf("1. First-Come, First-Served (FCFS)\n");           /* Option 1. */
        printf("2. Shortest Job First (SJF)\n");                  /* Option 2. */
        printf("3. Shortest Remaining Time Next (SRTN)\n");       /* Option 3. */
        printf("4. Round Robin (RR)\n");                          /* Option 4. */
        printf("5. Lottery Scheduling\n");                        /* Option 5. */
        printf("0. Exit\n");                                      /* Option 0 exits program. */
        printf("Enter your choice: ");                            /* Ask user to choose option. */
        scanf("%d", &choice);                                      /* Read menu choice into choice variable. */

        if (choice == 1) {                         /* User selected FCFS. */
            int n, burst[MAX];                     /* n = process count, burst[] = burst times. */

            printf("\nEnter number of processes: "); /* Ask how many processes. */
            scanf("%d", &n);                       /* Read process count. */

            printf("Enter burst times:\n");        /* Ask user to enter burst times. */
            for (int i = 0; i < n; i++) {          /* Loop through each process. */
                printf("P%d: ", i + 1);            /* Prompt for process i+1. */
                scanf("%d", &burst[i]);            /* Store burst time. */
            }

            fcfs(n, burst);                        /* Call FCFS scheduling function. */
        }
        else if (choice == 2) {                    /* User selected SJF. */
            int n, burst[MAX];                     /* n = process count, burst[] = burst times. */

            printf("\nEnter number of processes: "); /* Ask how many processes. */
            scanf("%d", &n);                       /* Read process count. */

            printf("Enter burst times:\n");        /* Ask user to enter burst times. */
            for (int i = 0; i < n; i++) {          /* Loop through each process. */
                printf("P%d: ", i + 1);            /* Prompt for process i+1. */
                scanf("%d", &burst[i]);            /* Store burst time. */
            }

            sjf(n, burst);                         /* Call SJF scheduling function. */
        }
        else if (choice == 3) {                    /* User selected SRTN. */
            int n, arrival[MAX], burst[MAX];       /* Store process count, arrival times, and burst times. */

            printf("\nEnter number of processes: "); /* Ask how many processes. */
            scanf("%d", &n);                       /* Read process count. */

            printf("Enter arrival time and burst time for each process:\n");
                                                    /* SRTN needs arrival and burst time. */
            for (int i = 0; i < n; i++) {          /* Loop through each process. */
                printf("P%d arrival: ", i + 1);    /* Ask arrival time for process i+1. */
                scanf("%d", &arrival[i]);          /* Store arrival time. */
                printf("P%d burst: ", i + 1);      /* Ask burst time for process i+1. */
                scanf("%d", &burst[i]);            /* Store burst time. */
            }

            srtn(n, arrival, burst);               /* Call SRTN scheduling function. */
        }
        else if (choice == 4) {                    /* User selected Round Robin. */
            int n, burst[MAX], quantum;            /* Store process count, burst times, and time quantum. */

            printf("\nEnter number of processes: "); /* Ask how many processes. */
            scanf("%d", &n);                       /* Read process count. */

            printf("Enter burst times:\n");        /* Ask for burst times. */
            for (int i = 0; i < n; i++) {          /* Loop through each process. */
                printf("P%d: ", i + 1);            /* Prompt for process i+1. */
                scanf("%d", &burst[i]);            /* Store burst time. */
            }

            printf("Enter time quantum: ");        /* Ask for Round Robin time quantum. */
            scanf("%d", &quantum);                 /* Read time quantum. */

            round_robin(n, burst, quantum);        /* Call Round Robin scheduling function. */
        }
        else if (choice == 5) {                    /* User selected Lottery Scheduling. */
            int n, burst[MAX], tickets[MAX], quantum;
                                                    /* Store process count, burst times, tickets, and quantum. */

            printf("\nEnter number of processes: "); /* Ask how many processes. */
            scanf("%d", &n);                       /* Read process count. */

            printf("Enter burst time and tickets for each process:\n");
                                                    /* Ask for burst time and ticket count for each process. */
            for (int i = 0; i < n; i++) {          /* Loop through each process. */
                printf("P%d burst: ", i + 1);      /* Ask burst time. */
                scanf("%d", &burst[i]);            /* Store burst time. */
                printf("P%d tickets: ", i + 1);    /* Ask number of tickets. */
                scanf("%d", &tickets[i]);          /* Store ticket count. */
            }

            printf("Enter time quantum: ");        /* Ask for lottery time quantum. */
            scanf("%d", &quantum);                 /* Read time quantum. */

            for (int run = 1; run <= 3; run++) {   /* Run lottery three times to show randomness. */
                printf("\n--- Lottery Run %d ---", run); /* Print run number. */
                lottery(n, burst, tickets, quantum);      /* Call lottery scheduler. */
            }
        }
        else if (choice == 0) {                    /* User selected exit. */
            printf("\nExiting program.\n");        /* Print exit message. */
        }
        else {                                     /* Any other number is invalid. */
            printf("\nInvalid choice. Please try again.\n"); /* Ask user to choose again. */
        }
    } while (choice != 0);                         /* Keep showing menu until choice is 0. */

    return 0;                                      /* Return 0 to indicate successful program termination. */
}
