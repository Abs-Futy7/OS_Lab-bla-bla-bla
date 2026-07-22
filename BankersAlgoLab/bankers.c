#include <stdio.h>                              // Needed for printf() and scanf()
#include <stdbool.h>                            // Needed for bool, true, and false

#define MAX_PROCESSES 20                        // Maximum number of processes allowed
#define MAX_RESOURCES 20                        // Maximum number of resource types allowed

int main(void)                                  // Program execution starts from main()
{
    int n, m;                                   // n = process count, m = resource type count
    int existing[MAX_RESOURCES];                // existing[j] stores total instances of resource Rj
    int allocation[MAX_PROCESSES][MAX_RESOURCES]; // allocation[i][j] stores resources of Rj held by Pi
    int max_demand[MAX_PROCESSES][MAX_RESOURCES]; // max_demand[i][j] stores maximum demand of Pi for Rj
    int need[MAX_PROCESSES][MAX_RESOURCES];     // need[i][j] stores remaining demand of Pi for Rj
    int available[MAX_RESOURCES];               // available[j] stores currently free instances of Rj
    int work[MAX_RESOURCES];                    // work[j] simulates available resources during safety checking
    int safe_sequence[MAX_PROCESSES];           // safe_sequence stores the final safe process order
    bool finish[MAX_PROCESSES] = { false };     // finish[i] is true after process Pi is safely completed
    int completed = 0;                          // Counts how many processes are already in safe_sequence
    bool is_safe = true;                        // Becomes false if no safe sequence can be found

    printf("Banker's Algorithm - Safe State Detection\n\n"); // Print program heading

    printf("Enter number of processes and resource types: "); // Ask user for n and m
    if (scanf("%d %d", &n, &m) != 2) {          // Read n and m, and check if both were entered correctly
        printf("Invalid input: expected two integers.\n"); // Error message for wrong input format
        return 1;                               // Stop program with error code
    }

    if (n <= 0 || n > MAX_PROCESSES || m <= 0 || m > MAX_RESOURCES) { // Check array limits
        printf("Invalid size: max %d processes and %d resources allowed.\n",
               MAX_PROCESSES, MAX_RESOURCES);  // Tell user the allowed maximum sizes
        return 1;                               // Stop because input size is invalid
    }

    printf("Enter Existing resources vector E (%d values): ", m); // Ask for total resources
    for (int j = 0; j < m; j++) {                // Loop through each resource type Rj
        scanf("%d", &existing[j]);               // Read total instances of resource Rj
        if (existing[j] < 0) {                   // Resource count cannot be negative
            printf("Invalid input: existing resources cannot be negative.\n"); // Print validation error
            return 1;                            // Stop because input is invalid
        }
    }

    printf("Enter Allocation/Possessed matrix P (%d x %d values):\n", n, m); // Ask for Allocation matrix
    for (int i = 0; i < n; i++) {                // Loop through each process Pi
        for (int j = 0; j < m; j++) {            // Loop through each resource type Rj
            scanf("%d", &allocation[i][j]);      // Read how many Rj resources are allocated to Pi
            if (allocation[i][j] < 0) {          // Allocation cannot be negative
                printf("Invalid input: allocation cannot be negative.\n"); // Print validation error
                return 1;                        // Stop because input is invalid
            }
        }
    }

    printf("Enter Maximum demand matrix Max (%d x %d values):\n", n, m); // Ask for Max matrix
    for (int i = 0; i < n; i++) {                // Loop through each process Pi
        for (int j = 0; j < m; j++) {            // Loop through each resource type Rj
            scanf("%d", &max_demand[i][j]);      // Read maximum demand of Pi for Rj
            if (max_demand[i][j] < allocation[i][j]) { // Max cannot be smaller than current Allocation
                printf("Invalid input: Max[P%d][R%d] is smaller than Allocation.\n", i, j); // Explain bad cell
                return 1;                        // Stop because this matrix is impossible
            }
        }
    }

    /*
     * Step 1: Compute Available vector.
     * Formula: Available[j] = Existing[j] - sum of Allocation[i][j] for all i.
     */
    for (int j = 0; j < m; j++) {                // Calculate available amount for each resource Rj
        int allocated_sum = 0;                   // Start column sum for resource Rj from 0

        for (int i = 0; i < n; i++) {            // Visit every process Pi
            allocated_sum += allocation[i][j];   // Add Pi's allocated amount of resource Rj
        }

        available[j] = existing[j] - allocated_sum; // Free Rj = total Rj - allocated Rj

        if (available[j] < 0) {                  // Allocated resources cannot exceed total existing resources
            printf("Invalid state: allocated R%d exceeds existing resources.\n", j); // Print invalid state
            return 1;                            // Stop because resource state is impossible
        }
    }

    /*
     * Step 2: Compute Need matrix.
     * Formula: Need[i][j] = Max[i][j] - Allocation[i][j].
     */
    for (int i = 0; i < n; i++) {                // Loop through each process Pi
        for (int j = 0; j < m; j++) {            // Loop through each resource type Rj
            need[i][j] = max_demand[i][j] - allocation[i][j]; // Remaining need of Pi for Rj
        }
    }

    /* Step 3: Print Allocation matrix. */
    printf("\nAllocation / Possessed Matrix P:\n"); // Print matrix title
    printf("Process");                             // Print first table column heading
    for (int j = 0; j < m; j++) {                  // Print resource headings R0, R1, ...
        printf("\tR%d", j);                        // Print one resource column heading
    }
    printf("\n");                                  // Move to first process row
    for (int i = 0; i < n; i++) {                  // Print one row for each process
        printf("P%d", i);                          // Print process label Pi
        for (int j = 0; j < m; j++) {              // Print all resource values for this process
            printf("\t%d", allocation[i][j]);      // Print Allocation[i][j]
        }
        printf("\n");                              // End current process row
    }

    /* Step 4: Print Maximum demand matrix. */
    printf("\nMaximum Demand Matrix Max:\n");       // Print matrix title
    printf("Process");                             // Print first table column heading
    for (int j = 0; j < m; j++) {                  // Print resource headings
        printf("\tR%d", j);                        // Print Rj heading
    }
    printf("\n");                                  // Move to first process row
    for (int i = 0; i < n; i++) {                  // Print one row for each process
        printf("P%d", i);                          // Print process label Pi
        for (int j = 0; j < m; j++) {              // Print all maximum demands for Pi
            printf("\t%d", max_demand[i][j]);      // Print Max[i][j]
        }
        printf("\n");                              // End current row
    }

    /* Step 5: Print computed Need matrix. */
    printf("\nComputed Need Matrix:\n");            // Print matrix title
    printf("Process");                             // Print first table column heading
    for (int j = 0; j < m; j++) {                  // Print resource headings
        printf("\tR%d", j);                        // Print Rj heading
    }
    printf("\n");                                  // Move to first process row
    for (int i = 0; i < n; i++) {                  // Print one row for each process
        printf("P%d", i);                          // Print process label Pi
        for (int j = 0; j < m; j++) {              // Print all Need values for Pi
            printf("\t%d", need[i][j]);            // Print Need[i][j]
        }
        printf("\n");                              // End current row
    }

    /* Step 6: Print Existing vector E. */
    printf("\nExisting Resources E: [ ");           // Start vector output
    for (int j = 0; j < m; j++) {                  // Visit every resource type
        printf("%d", existing[j]);                 // Print total existing amount of Rj
        if (j + 1 < m) {                           // If this is not the last resource
            printf(", ");                          // Print separator between values
        }
    }
    printf(" ]\n");                                // Close vector output

    /* Step 7: Print Available vector A and initialize Work. */
    printf("Computed Available Resources A: [ ");   // Start Available vector output
    for (int j = 0; j < m; j++) {                  // Visit every resource type
        printf("%d", available[j]);                // Print currently available amount of Rj
        if (j + 1 < m) {                           // If this is not the last value
            printf(", ");                          // Print separator between values
        }
        work[j] = available[j];                    // Initial Work equals Available
    }
    printf(" ]\n");                                // Close vector output

    /*
     * Step 8: Banker's safety algorithm.
     * Search for an unfinished process whose Need is less than or equal to Work.
     * If found, pretend it finishes and releases its Allocation back into Work.
     */
    while (completed < n) {                        // Continue until all processes finish or unsafe detected
        bool found_process = false;                // No process has been found in this pass yet

        for (int i = 0; i < n; i++) {              // Try every process Pi
            bool can_finish = true;                // Assume Pi can finish until a resource proves otherwise

            if (finish[i]) {                       // Check if Pi is already completed
                continue;                          // Skip completed process
            }

            for (int j = 0; j < m; j++) {          // Check every resource type Rj
                if (need[i][j] > work[j]) {        // If Pi needs more Rj than Work has
                    can_finish = false;            // Pi cannot finish right now
                    break;                         // No need to check remaining resources for Pi
                }
            }

            if (can_finish) {                      // If Pi can get all remaining resources
                for (int j = 0; j < m; j++) {      // Release all resources held by Pi
                    work[j] += allocation[i][j];   // Add Pi's allocation back to Work
                }

                finish[i] = true;                  // Mark Pi as completed in the simulation
                safe_sequence[completed] = i;      // Save Pi as the next process in safe sequence
                completed++;                       // Increase number of completed processes
                found_process = true;              // This pass made progress
            }
        }

        if (!found_process) {                      // If no unfinished process could run in this pass
            is_safe = false;                       // System is unsafe
            break;                                 // Stop safety checking
        }
    }

    /* Step 9: Print final safety result. */
    if (is_safe) {                                 // If every process was completed
        printf("\nResult: The system is SAFE.\n"); // Print safe result
        printf("Safe sequence: < ");              // Start safe sequence output
        for (int i = 0; i < n; i++) {             // Print each process in safe order
            printf("P%d", safe_sequence[i]);      // Print process label
            if (i + 1 < n) {                      // If not the last process
                printf(", ");                     // Print comma separator
            }
        }
        printf(" >\n");                           // Close safe sequence output
    } else {                                      // If at least one process could not be completed
        printf("\nResult: The system is UNSAFE.\n"); // Print unsafe result
        printf("No safe sequence exists.\n");     // Explain that no valid order was found
    }

    return 0;                                     // End program successfully
}
