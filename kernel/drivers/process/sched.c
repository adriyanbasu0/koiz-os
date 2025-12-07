#include "sched.h"
#include "process.h"

#include "stdint.h"
#include "stddef.h"

#include "../../config/config.h"
#include "../../libc/stdlib.h"

#define MAX_PRIORITY_QUEUES 3
#define STARTING_TICKETS 2000
#define FULL_TIMESLICE_COST 10

struct mlfq_entry {
    struct process* ptr;
    int priority;
    int tickets_remaining;
    struct mlfq_entry* next_process;
};

/* Since the process list is fixed, we can just used a fixed
    section of memory here versus malloc-ing new memory. It's
    small enough it doesn't matter too much */
struct mlfq_entry proc_list[MAX_PROCESSES];
struct mlfq_entry *proc_priority_list[MAX_PRIORITY_QUEUES] = { 0 };
struct mlfq_entry *current_process = 0;

void sched_init(void)
{
    int i = 0;
    for(i = 0; i < MAX_PRIORITY_QUEUES; i++) {
        proc_priority_list[i] = 0;
    }
    printf("Initialized Process Scheduler\n");
}

void sched_addpid(int pid)
{
    struct process* ptr = process_get(pid);
    if(ptr == NULL) {
        printf("Tried to add PID %d to scheduler but is null process!\n");
        panic("Tried to schedule null process!");
    }

    /* You laugh, but it's so i can reference 
        these fields when I need too easier! */
    proc_list[pid].ptr = ptr;
    proc_list[pid].priority = 0;

    proc_list[pid].tickets_remaining = STARTING_TICKETS;
    
    /* Push it in front of the priority list */
    proc_list[pid].next_process = proc_priority_list[0];
    proc_priority_list[0] = &(proc_list[pid]);

#ifdef DEBUG_MSG_SCHEDULER
    printf("Schduler: Added new PID %d to scheduler. Name: %s \n", 
        pid, ptr->name);
#endif
}

void sched_printinfo(void)
{
    printf("Current Process: ");
    if(current_process != NULL) {
        if(current_process->ptr == NULL) {
            panic("There exists a null process in the scheduler!\n");
        }
        printf("%d\n", current_process->ptr->pid);
    } else {
        printf("<empty>\n");
    }

    int i = 0;
    for(i = 0; i < MAX_PRIORITY_QUEUES; i++) {
        printf("P%d:", i);
        if(proc_priority_list[i] == NULL) {
            printf("<empty>\n");
            continue;
        }
        struct mlfq_entry* cptr = proc_priority_list[i];
        while(cptr != NULL) {
            if(cptr->ptr == NULL) {
                panic("There exists a null process in the scheduler!\n");
            }
            printf(" %d (%d) -> ", cptr->ptr->pid, cptr->tickets_remaining);
            cptr = cptr->next_process;
        }
        printf("\n");
    }
}

void sched_update(void)
{
    /* Scenario 1: No current process exists.
     *      In this case, simply find one in the highest priority queue */
    if(current_process == NULL) {
        int i = 0;
        for(i = 0; i < MAX_PRIORITY_QUEUES; i++) {
            if(proc_priority_list[i] != NULL) {
                current_process = proc_priority_list[i];
                break;
            }
        }
        return;
    }

    /* Sanity checks */
    if(current_process == NULL)
        panic("Expected a current_process at this point!\n");
    if(current_process->ptr == NULL)
        panic("There exists a null process in the scheduler!\n");

    /* Decrement the current process by number of tickets (if applicable). */
    current_process->tickets_remaining -= FULL_TIMESLICE_COST;
    if(current_process->tickets_remaining < 0) {
        current_process->tickets_remaining = 0;

        /* Bump process down the queue if we can */
        int cprior = current_process->priority;
        if(cprior < MAX_PRIORITY_QUEUES-1) {

            /* Sanity check */
            if(proc_priority_list[cprior] == NULL)
                panic("Head of current priority list is empty!");

            /* If it exists at the head, simply pop it off the head */
            if(proc_priority_list[cprior] == current_process) {
                proc_priority_list[cprior] = current_process->next_process;
            } else {
                /* We need to search for it */
                struct mlfq_entry* curr = proc_priority_list[cprior];
                struct mlfq_entry* next = current_process->next_process;
                while(curr != NULL) {
                    if(curr->next_process != NULL &&
                        curr->next_process == current_process) {
                        curr->next_process = next;
                    break;
                        }
                        curr = curr->next_process;
                }
                if(curr == NULL) {
                    panic("Cannot find current process in priority!");
                }
            }

            /* Decrement priority */
            int newprior = current_process->priority + 1;
            if(newprior >= MAX_PRIORITY_QUEUES)
                panic("Moved to an out of bounds priority!");

            /* Add to lower priority queue */
            current_process->priority = newprior;
            current_process->tickets_remaining = STARTING_TICKETS;
            current_process->next_process = proc_priority_list[newprior];
            proc_priority_list[newprior] = current_process;
        }
    }

    /* Scenario 2: Current process is NOT highest priority.
     *      Switch to a higher priority process (if possible) */
    if(current_process->priority != 0) {
        struct mlfq_entry* new_process = current_process;
        int i = 0;
        for(i = 0; i < MAX_PRIORITY_QUEUES; i++) {
            if(proc_priority_list[i] != NULL) {
                new_process = proc_priority_list[i];
                break;
            }
        }

        if(new_process->priority < current_process->priority) {
            current_process = new_process;
            return;
        }
    }

    /* Scenario 3: Current process is the highest priority.
     *      Switch to the next process in line (if possible) */
    int cpr = current_process->priority;
    if(current_process->next_process != NULL) {
        current_process = current_process->next_process;
    } else if(proc_priority_list[cpr] != NULL) {
        current_process = proc_priority_list[cpr];
    }

    /* Scenario 4: We're the only highest priority process.
     *      Continue as is! */

    /* --------------------------------------------------------- */
    /* HANDLE KILLED PROCESSES                                   */
    /* --------------------------------------------------------- */

    /* Check if the running process is marked as killed */
    if (current_process->ptr->killed && current_process->ptr->pid != 0) {

        current_process->ptr->state = ZOMBIE;

        /* Force switch to IDLE process (PID 0)
         *          This ensures we don't execute dead code and safe to reap below */
        current_process = &proc_list[0];
    }


    /* --------------------------------------------------------- */
    /* REAP ZOMBIE PROCESSES                                     */
    /* --------------------------------------------------------- */

    /* Iterate through all priority queues to find ZOMBIES */
    int q = 0;
    for(q = 0; q < MAX_PRIORITY_QUEUES; q++) {
        struct mlfq_entry* curr = proc_priority_list[q];
        struct mlfq_entry* prev = NULL;

        while(curr != NULL) {
            /* Check if the entry points to a ZOMBIE process */
            if (curr->ptr->state == ZOMBIE && curr->ptr->pid != 0) {

                /* 1. Unlink from the Linked List */
                if (prev == NULL) {
                    /* Removing the head of the list */
                    proc_priority_list[q] = curr->next_process;
                } else {
                    /* Removing from middle/end */
                    prev->next_process = curr->next_process;
                }

                /* 2. Release Process Resources */
                struct process* zproc = curr->ptr;

                // IMPORTANT: We must clear the memory pointer so process_init_process
                // doesn't panic when reusing this PID.
                // TODO: Call pmem_free(zproc->process_memory_start) here if available.
                zproc->process_memory_start = NULL;
                zproc->process_memory_size = 0;

                /* 3. Reset Process Slot */
                zproc->killed = 0;
                zproc->state = UNUSED;

                /* 4. Reset Scheduler Entry */
                struct mlfq_entry* to_reap = curr;
                to_reap->priority = 0;
                to_reap->tickets_remaining = 0;
                // Note: We leave to_reap->ptr linked, as the slot is now UNUSED

                /* Move curr forward, but keep prev where it is (since curr is gone) */
                curr = curr->next_process;

                /* Break the link on the reaped node for safety */
                to_reap->next_process = NULL;

            } else {
                /* Not a zombie, just move forward */
                prev = curr;
                curr = curr->next_process;
            }
        }
    }
}
