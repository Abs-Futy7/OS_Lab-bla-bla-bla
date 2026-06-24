#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX 10

void fcfs(int n, int burst[]) {
    int wt[MAX], tat[MAX];
    float avg_wt = 0, avg_tat = 0;

    wt[0] = 0;
    for (int i = 1; i < n; i++)
        wt[i] = wt[i-1] + burst[i-1];

    printf("\n===== FCFS Scheduling =====\n");
    printf("%-10s %-15s %-15s %-15s\n", "Process", "Burst(ms)", "Waiting(ms)", "Turnaround(ms)");
    printf("--------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        tat[i] = wt[i] + burst[i];
        avg_wt  += wt[i];
        avg_tat += tat[i];
        printf("P%-9d %-15d %-15d %-15d\n", i+1, burst[i], wt[i], tat[i]);
    }

    printf("\nAverage Waiting Time    : %.2f ms\n", avg_wt  / n);
    printf("Average Turnaround Time : %.2f ms\n",  avg_tat / n);
}


void sjf(int n, int burst[]) {
    int b[MAX], idx[MAX], wt[MAX], tat[MAX];
    float avg_wt = 0, avg_tat = 0;

    for (int i = 0; i < n; i++) { b[i] = burst[i]; idx[i] = i+1; }

    for (int i = 1; i < n; i++) {
        int key_b = b[i], key_idx = idx[i], j = i - 1;
        while (j >= 0 && b[j] > key_b) {
            b[j + 1] = b[j];
            idx[j + 1] = idx[j];
            j--;
        }
        b[j + 1] = key_b;
        idx[j + 1] = key_idx;
    }

    wt[0] = 0;
    for (int i = 1; i < n; i++) wt[i] = wt[i-1] + b[i-1];

    printf("\n===== SJF Scheduling =====\n");
    printf("%-10s %-15s %-15s %-15s\n", "Process", "Burst(ms)", "Waiting(ms)", "Turnaround(ms)");
    printf("--------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        tat[i]   = wt[i] + b[i];
        avg_wt  += wt[i];
        avg_tat += tat[i];
        printf("P%-9d %-15d %-15d %-15d\n", idx[i], b[i], wt[i], tat[i]);
    }

    printf("\nAverage Waiting Time    : %.2f ms\n", avg_wt  / n);
    printf("Average Turnaround Time : %.2f ms\n",  avg_tat / n);
}


void srtn(int n, int arrival[], int burst[]) {
    int rem[MAX], done[MAX], complete[MAX], wt[MAX], tat[MAX];
    float avg_wt = 0, avg_tat = 0;

    for (int i = 0; i < n; i++) { rem[i] = burst[i]; done[i] = 0; }

    int finished = 0, t = 0;
    while (finished < n) {
        int sel = -1, minR = 99999;
        for (int i = 0; i < n; i++)
            if (!done[i] && arrival[i] <= t && rem[i] < minR) {
                minR = rem[i]; sel = i;
            }
        if (sel == -1) { t++; continue; }
        rem[sel]--;
        t++;
        if (rem[sel] == 0) {
            done[sel]    = 1;
            complete[sel] = t;
            finished++;
        }
    }

    printf("\n===== SRTN Scheduling =====\n");
    printf("%-10s %-12s %-12s %-14s %-12s %-14s\n",
           "Process","Arrival","Burst","Completion","Waiting","Turnaround");
    printf("----------------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        tat[i]   = complete[i] - arrival[i];
        wt[i]    = tat[i] - burst[i];
        avg_wt  += wt[i];
        avg_tat += tat[i];
        printf("P%-9d %-12d %-12d %-14d %-12d %-14d\n",
               i+1, arrival[i], burst[i], complete[i], wt[i], tat[i]);
    }

    printf("\nAverage Waiting Time    : %.2f ms\n", avg_wt  / n);
    printf("Average Turnaround Time : %.2f ms\n",  avg_tat / n);
}


void round_robin(int n, int burst[], int quantum) {
    int rem[MAX], complete[MAX], wt[MAX], tat[MAX];
    float avg_wt = 0, avg_tat = 0;

    for (int i = 0; i < n; i++) rem[i] = burst[i];

    int t = 0, finished = 0;
    /* track finish time per process */
    for (int i = 0; i < n; i++) complete[i] = 0;

    printf("\n===== Round Robin Scheduling (Quantum = %d ms) =====\n", quantum);
    printf("Gantt Chart: ");

    while (finished < n) {
        int did_work = 0;
        for (int i = 0; i < n; i++) {
            if (rem[i] > 0) {
                did_work = 1;
                int run = (rem[i] < quantum) ? rem[i] : quantum;
                printf("[P%d: %d-%d] ", i+1, t, t+run);
                t += run;
                rem[i] -= run;
                if (rem[i] == 0) { complete[i] = t; finished++; }
            }
        }
        if (!did_work) break;
    }

    printf("\n\n%-10s %-15s %-15s %-15s\n", "Process","Burst(ms)","Waiting(ms)","Turnaround(ms)");
    printf("--------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        tat[i]   = complete[i];         
        wt[i]    = tat[i] - burst[i];
        avg_wt  += wt[i];
        avg_tat += tat[i];
        printf("P%-9d %-15d %-15d %-15d\n", i+1, burst[i], wt[i], tat[i]);
    }

    printf("\nAverage Waiting Time    : %.2f ms\n", avg_wt  / n);
    printf("Average Turnaround Time : %.2f ms\n",  avg_tat / n);
}


void lottery(int n, int burst[], int tickets[], int quantum) {
    int rem[MAX], complete[MAX];
    float avg_wt = 0, avg_tat = 0;

    for (int i = 0; i < n; i++) { rem[i] = burst[i]; complete[i] = 0; }

    int t = 0, finished = 0;

    printf("\n===== Lottery Scheduling (Quantum = %d ms) =====\n", quantum);
    printf("%-8s %-10s %-10s\n", "Time", "Winner", "Ticket");
    printf("----------------------------------\n");

    while (finished < n) {
        int total = 0;
        for (int i = 0; i < n; i++) if (rem[i] > 0) total += tickets[i];
        if (total == 0) break;

        int draw = rand() % total;
        int cum  = 0, sel = -1;
        for (int i = 0; i < n; i++) {
            if (rem[i] <= 0) continue;
            cum += tickets[i];
            if (draw < cum) { sel = i; break; }
        }

        int run = (rem[sel] < quantum) ? rem[sel] : quantum;
        printf("t=%-6d P%-6d ticket #%d\n", t, sel+1, draw);
        t      += run;
        rem[sel] -= run;
        if (rem[sel] == 0) { complete[sel] = t; finished++; }
    }

    printf("\n%-10s %-12s %-10s %-14s %-12s %-14s\n",
           "Process","Burst(ms)","Tickets","Completion","Waiting","Turnaround");
    printf("--------------------------------------------------------------------------\n");

    for (int i = 0; i < n; i++) {
        int tat = complete[i];           /* arrival = 0 */
        int wt  = tat - burst[i];
        avg_wt  += wt;
        avg_tat += tat;
        printf("P%-9d %-12d %-10d %-14d %-12d %-14d\n",
               i+1, burst[i], tickets[i], complete[i], wt, tat);
    }

    printf("\nAverage Waiting Time    : %.2f ms\n", avg_wt  / n);
    printf("Average Turnaround Time : %.2f ms\n",  avg_tat / n);
}


int main(void) {
    srand((unsigned)time(NULL));

    int choice;

    do {
        printf("\n========== CPU Scheduling Menu ==========\n");
        printf("1. First-Come, First-Served (FCFS)\n");
        printf("2. Shortest Job First (SJF)\n");
        printf("3. Shortest Remaining Time Next (SRTN)\n");
        printf("4. Round Robin (RR)\n");
        printf("5. Lottery Scheduling\n");
        printf("0. Exit\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);

        if (choice == 1) {
            int n, burst[MAX];

            printf("\nEnter number of processes: ");
            scanf("%d", &n);

            printf("Enter burst times:\n");
            for (int i = 0; i < n; i++) {
                printf("P%d: ", i + 1);
                scanf("%d", &burst[i]);
            }

            fcfs(n, burst);
        }
        else if (choice == 2) {
            int n, burst[MAX];

            printf("\nEnter number of processes: ");
            scanf("%d", &n);

            printf("Enter burst times:\n");
            for (int i = 0; i < n; i++) {
                printf("P%d: ", i + 1);
                scanf("%d", &burst[i]);
            }

            sjf(n, burst);
        }
        else if (choice == 3) {
            int n, arrival[MAX], burst[MAX];

            printf("\nEnter number of processes: ");
            scanf("%d", &n);

            printf("Enter arrival time and burst time for each process:\n");
            for (int i = 0; i < n; i++) {
                printf("P%d arrival: ", i + 1);
                scanf("%d", &arrival[i]);
                printf("P%d burst: ", i + 1);
                scanf("%d", &burst[i]);
            }

            srtn(n, arrival, burst);
        }
        else if (choice == 4) {
            int n, burst[MAX], quantum;

            printf("\nEnter number of processes: ");
            scanf("%d", &n);

            printf("Enter burst times:\n");
            for (int i = 0; i < n; i++) {
                printf("P%d: ", i + 1);
                scanf("%d", &burst[i]);
            }

            printf("Enter time quantum: ");
            scanf("%d", &quantum);

            round_robin(n, burst, quantum);
        }
        else if (choice == 5) {
            int n, burst[MAX], tickets[MAX], quantum;

            printf("\nEnter number of processes: ");
            scanf("%d", &n);

            printf("Enter burst time and tickets for each process:\n");
            for (int i = 0; i < n; i++) {
                printf("P%d burst: ", i + 1);
                scanf("%d", &burst[i]);
                printf("P%d tickets: ", i + 1);
                scanf("%d", &tickets[i]);
            }

            printf("Enter time quantum: ");
            scanf("%d", &quantum);

            for (int run = 1; run <= 3; run++) {
                printf("\n--- Lottery Run %d ---", run);
                lottery(n, burst, tickets, quantum);
            }
        }
        else if (choice == 0) {
            printf("\nExiting program.\n");
        }
        else {
            printf("\nInvalid choice. Please try again.\n");
        }
    } while (choice != 0);

    return 0;
}

