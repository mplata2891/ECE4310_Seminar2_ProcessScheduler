/*
    Class:      ECE 4310 - Op Sys for Embedded Apps
    Professor:  Liviu Oniciuc
    Assignment: Seminar Two - Process Scheduler
    Due Date:   3/5/2024
    Group:      A6
    Members:    Mike Plata & Jessica Ramirez
*/

///////////////////////////////////////////////////////////////////////////////
//LIBRARY INCLUSIONS
///////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

///////////////////////////////////////////////////////////////////////////////
//TYPE-DEF
///////////////////////////////////////////////////////////////////////////////
typedef struct Process Process;
struct Process
{
    Process* next;

    int id;
    char category[30];
    char state[10];
    int priority_level;
    int completion_time_ms;
    int processed_through_current_queue;
    int wait_counter;    
    
};

typedef struct Queue
{
    Process* head;
    Process* tail;

    int priority_level;
    int time_quantum_ms;
    int processes_in_queue;
}Queue;

///////////////////////////////////////////////////////////////////////////////
//PRE-PROCESSOR DIRECTIVES
///////////////////////////////////////////////////////////////////////////////
Process* createNewProcess();
void initializeProcessPriority(Process* process);
void initializeProcessCompletionTime(Process* process);
void sortProcess(Queue* hi, Queue* mid, Queue* low, Process* process);
void printProcessInfo(Process* process);
Queue* createNewQueue();
int enQueue(Queue* queue, Process* process);
Process* deQueue(Queue* queue);
void returnToFrontOfQueue(Queue* queue, Process* process);
Process* removeFromQueue(Queue* queue, Process* process_to_remove);
bool queueIsNotEmpty(Queue* queue);
void printQueueInfo(Queue* queue);
void multiLevelFeedbackScheduler(Queue* hi, Queue* mid, Queue* low);
void feedbackQueue(Queue* active_queue, Queue* lower_queue, Process* active_process);
void firstComeFirstServeQueue(Queue* active_queue, Process* active_process);
void runActiveProcess(Queue* queue, Process* process);
void updateWaitCountersMultipleQueues(Queue* q1, Queue* q2, Queue* q3, Process* last_process);
void updateWaitCounters(Queue* queue, Process* last_run_process);
void upgradeStarvingProcesses(Queue* hi, Queue* mid, Queue* low);
void upgradeProcessPriority(Queue* lower_priority_queue, Queue* higher_priority_queue);
void displayQueues(Queue* hi, Queue* mid, Queue* low);
bool newProcessArrival();
void newProcessAlert(char priority[]);
void priorityQueueUpgradeAlert();
void priorityQueueDowngradeAlert();

///////////////////////////////////////////////////////////////////////////////
//GLOBAL VARIABLES
///////////////////////////////////////////////////////////////////////////////
int global_nextProcess_id = 0;
int global_time_modifier = 0;

///////////////////////////////////////////////////////////////////////////////
//MAIN FUNCTION
///////////////////////////////////////////////////////////////////////////////
int main()
{   
    srand((unsigned int)(time(NULL) + global_time_modifier)); //seed rand function
    global_time_modifier++;
    int random_processes = rand() % 6 + 5; //random number from 5-10
    
    //define 3 queues of differing priority and unique time quantums
    Queue* high_priority_queue = createNewQueue(0,4);
    Queue* mid_priority_queue = createNewQueue(1,16);
    Queue* low_priority_queue = createNewQueue(2,32);

    //define 5 to 10 random process, sorted into differing priority queues 
    for(int make_new_process = 0; make_new_process < random_processes; make_new_process++)
        sortProcess(high_priority_queue, mid_priority_queue, low_priority_queue, createNewProcess());

    printf("\033[31m%d Processes Initially Made\n\n\033[0m", random_processes); 

    //continually run the Multi-Level Feedback Scheduler for testing
    /* do
    {
        displayQueues(high_priority_queue, mid_priority_queue, low_priority_queue);
        multiLevelFeedbackScheduler(high_priority_queue, mid_priority_queue, low_priority_queue);
        
    } while (true); */

    for(int run_scheduler = 0; run_scheduler < (random_processes * 2); run_scheduler++)
    {
        displayQueues(high_priority_queue, mid_priority_queue, low_priority_queue);
        multiLevelFeedbackScheduler(high_priority_queue, mid_priority_queue, low_priority_queue);
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//HELPER FUNCTIONS - PROCESSES
///////////////////////////////////////////////////////////////////////////////
Process* createNewProcess()
{
    Process* new_process = (Process*) malloc(sizeof(Process));

    if(new_process == NULL) 
        return NULL;

    new_process->next = NULL;
    new_process->id = global_nextProcess_id;
    strcpy(new_process->state,"READY");
    new_process->processed_through_current_queue = 0;
    new_process->wait_counter = 0; 
    initializeProcessPriority(new_process);
    initializeProcessCompletionTime(new_process);
      
    global_nextProcess_id++;
    return new_process;
}

void initializeProcessPriority(Process* process)
{
    srand((unsigned int)(time(NULL) + global_time_modifier)); //seed rand function
    global_time_modifier++;

    switch(process->priority_level = rand() % 3)
    {
        case 0:
            strcpy(process->category,"System Process");
            break;

        case 1:
            strcpy(process->category,"Interactive Process");
            break;

        case 2:
            strcpy(process->category,"Batch Process");
            break;

        default:
            strcpy(process->category,"Categorization ERROR");
            break;
    }
}

void initializeProcessCompletionTime(Process* process)
{
    srand((unsigned int)(time(NULL) + global_time_modifier)); //seed rand function
    global_time_modifier++;
    
    switch(rand() % 4) //random number from 0-3
    {
        case 0:
            process->completion_time_ms = 4;
            break;

        case 1:
            process->completion_time_ms = 16;
            break;

        case 2:
            process->completion_time_ms = 32;
            break;

        case 3:
            process->completion_time_ms = 64;
            break;
            
        default:
            process->completion_time_ms = 0;
            break;
    }
}

void sortProcess(Queue* hi, Queue* mid, Queue* low, Process* process)
{
    switch(process->priority_level)
    {
        case 0:
            enQueue(hi, process);
            newProcessAlert("HIGH");
            break;

        case 1:
            enQueue(mid, process);
            newProcessAlert("MID");
            break;

        case 2:
            enQueue(low, process);
            newProcessAlert("LOW");
            break;

        default:
            printf("\n:::: ERROR SORTING PROCESS ::::\n");
            break;
    }
}

void printProcessInfo(Process* process)
{
    printf("\033[33m+--------------------------------------+\n");
    printf("| Process ID:           %8d       |\n", process->id);
    printf("| Category:             %8s |\n", process->category);
    printf("| State:                %8s       |\n", process->state);
    printf("| Priority Level:       %8d       |\n", process->priority_level);
    printf("| Completion Time (ms): %8d       |\n", process->completion_time_ms);
    printf("| Wait Counter:         %8d       |\n", process->wait_counter);
    printf("+--------------------------------------+\033[0m\n");
}

///////////////////////////////////////////////////////////////////////////////
//HELPER FUNCTIONS - QUEUES
///////////////////////////////////////////////////////////////////////////////
Queue* createNewQueue(int priority_level, int time_quantum)
{
    Queue* new_queue = (Queue*) malloc(sizeof(Queue));

    if(new_queue == NULL)
        return NULL;

    new_queue->head = NULL;
    new_queue->tail = NULL;
    new_queue->priority_level = priority_level;
    new_queue->time_quantum_ms = time_quantum;
    new_queue->processes_in_queue = 0;

    return new_queue;
}

int enQueue(Queue* queue, Process* process)
{
    if(queue == NULL)
        return -1;

    if(queue->head == NULL)
    {
        queue->head = process;
        queue->tail = process;
        queue->processes_in_queue++;
        return 0;
    }

    queue->tail->next = process;
    queue->tail = process;
    queue->processes_in_queue++;
    return 0;
}

Process* deQueue(Queue* queue)
{
    if(queue == NULL || queue->head == NULL)
        return NULL;

    Process* dequeued_process = queue->head;
    
    if(queue->head == queue->tail)
    {
        queue->head = NULL;
        queue->tail = NULL;
        dequeued_process->next = NULL;
        queue->processes_in_queue--;
        return dequeued_process;
    }

    queue->head = queue->head->next;
    dequeued_process->next = NULL;
    queue->processes_in_queue--;
    return dequeued_process;
}

void returnToFrontOfQueue(Queue* queue, Process* process)
{
    if(!queueIsNotEmpty(queue))
        enQueue(queue, process);
    else
    {
        process->next = queue->head;
        queue->head = process;
    }

    queue->processes_in_queue++;
}

Process* removeFromQueue(Queue* queue, Process* process_to_remove)
{
    Process* original_head = queue->head;
    Process* process_to_check;
    Process* removed_process;

    //checks if the queue's head is the process to be removed
    //if yes, removes and returns it, if not, puts it in the back of the queue
    if(process_to_remove == queue->head)
    {
        removed_process = deQueue(queue);
        removed_process->next = NULL;
        return removed_process;
    }
    else
        enQueue(queue,deQueue(queue));
    
    //if original head was not process to be removed, cycles through queue to find
    //the process to be removed, and puts queue back in original order
    do
    {
        process_to_check = deQueue(queue);
        
        if(process_to_check == process_to_remove)
            removed_process = process_to_check;
        else
            enQueue(queue, process_to_check);

    } while (queue->head != original_head);
    
    removed_process->next = NULL;
    return removed_process;
}

bool queueIsNotEmpty(Queue* queue)
{
    if(queue->head != NULL) 
        return true;
    else 
        return false;
}

void printQueueInfo(Queue* queue)
{   
    printf("\033[32m╔════════════════════════════════════╗\n");
    printf("║  Queue Priority Level:  %8d   ║\n", queue->priority_level);
    printf("║  Time Quantum (ms):     %8d   ║\n", queue->time_quantum_ms);
    printf("║  Processes in Queue:    %8d   ║\n", queue->processes_in_queue);
    printf("║  Head Process ID:       %8d   ║\n", (queue->head != NULL) ? queue->head->id : -1);

    if(queue->processes_in_queue > 2)
    {
        Process* current_process = queue->head->next;
        
        do
        {
            printf("║  Next Process ID:       %8d   ║\n", current_process->id);
            current_process = current_process->next;

        } while (current_process != queue->tail);
        
    }

    printf("║  Tail Process ID:       %8d   ║\n", (queue->tail != NULL) ? queue->tail->id : -1);
    printf("╚════════════════════════════════════╝\033[0m\n");
}

///////////////////////////////////////////////////////////////////////////////
//HELPER FUNCTIONS - SCHEDULER
///////////////////////////////////////////////////////////////////////////////
void multiLevelFeedbackScheduler(Queue* hi, Queue* mid, Queue* low)
{
    /* srand((unsigned int)(time(NULL) + global_time_modifier)); //seed rand function
    global_time_modifier++; */

    Process* active_process = NULL;
    
    //check queues for processes and process queues in order from hi to low priority
    if(queueIsNotEmpty(hi))
        feedbackQueue(hi, mid, active_process);
    else if(queueIsNotEmpty(mid))
        feedbackQueue(mid, low, active_process);
    else if(queueIsNotEmpty(low))
        firstComeFirstServeQueue(low, active_process);
    
    updateWaitCountersMultipleQueues(hi, mid, low, active_process);

    upgradeStarvingProcesses(hi, mid, low);

    //if random number from 0-3 is a 1, create new process and place in queue
    if(newProcessArrival())
    {
        sortProcess(hi, mid, low, createNewProcess());
        //printf("\n\nNEW PROCESS ADDED TO A QUEUE\n\n");
    }
}

void feedbackQueue(Queue* active_queue, Queue* lower_queue, Process* active_process)
{
    active_process = deQueue(active_queue);
    runActiveProcess(active_queue, active_process);
    active_process->processed_through_current_queue++;

    if(active_process->completion_time_ms > 0 && active_process->processed_through_current_queue < 3)
        enQueue(active_queue, active_process);
    else if(active_process->completion_time_ms > 0)
    {
        active_process->processed_through_current_queue = 0;
        enQueue(lower_queue, active_process);
        priorityQueueDowngradeAlert();
    }
       
    else
        free(active_process);
}

void firstComeFirstServeQueue(Queue* active_queue, Process* active_process)
{
    //take the process at the front of the queue and run it 
    active_process = deQueue(active_queue);
    runActiveProcess(active_queue, active_process);

    if(active_process->completion_time_ms > 0)
        returnToFrontOfQueue(active_queue, active_process);
    else
        free(active_process);
}

void runActiveProcess(Queue* queue, Process* process)
{
    //update process' state to active
    strcpy(process->state,"ACTIVE");
    printProcessInfo(process);
    
    //update process' time to completion
    process->completion_time_ms -= queue->time_quantum_ms;
    if(process->completion_time_ms < 0)
        process->completion_time_ms = 0;

    //reset process' time waiting to be the active process
    process->wait_counter = 0;
    
    //update process' state to either ready or complete
    if(process->completion_time_ms == 0)
        strcpy(process->state,"COMPLETE");
    else
        strcpy(process->state,"READY");
    
    printProcessInfo(process);
}

void updateWaitCountersMultipleQueues(Queue* q1, Queue* q2, Queue* q3, Process* last_process)
{
    if(queueIsNotEmpty(q1))
        updateWaitCounters(q1, last_process);

    if(queueIsNotEmpty(q2))
        updateWaitCounters(q2, last_process);

    if(queueIsNotEmpty(q3))
        updateWaitCounters(q3, last_process);
}

void updateWaitCounters(Queue* queue, Process* last_run_process)
{
    Process* original_head = queue->head;
    Process* process_to_update;

    do
    {
        process_to_update = deQueue(queue);

        if(process_to_update != last_run_process)
            process_to_update->wait_counter++;

        enQueue(queue, process_to_update);

    } while (queue->head != original_head);
    
}

void upgradeStarvingProcesses(Queue* hi, Queue* mid, Queue* low)
{
    //if any starving proccess found, upgrade them to the highest priority queue.
    if(queueIsNotEmpty(mid))
        upgradeProcessPriority(mid, hi);

    //if any starving proccess found, upgrade them to the highest priority queue.
    if(queueIsNotEmpty(low))
        upgradeProcessPriority(low, hi);
}

void upgradeProcessPriority(Queue* lower_priority_queue, Queue* higher_priority_queue)
{
    Process* current_process = lower_priority_queue->head;
    Process* removed_process;

    const int LONG_WAIT_TIME = 4;

    do
    {
        if(current_process->wait_counter == LONG_WAIT_TIME)
        {
            removed_process = current_process;
            current_process = current_process->next;

            removed_process = removeFromQueue(lower_priority_queue, removed_process);
            enQueue(higher_priority_queue, removed_process);

            priorityQueueUpgradeAlert();
        }
        else
            current_process = current_process->next;
            
    } while (current_process != NULL);
}

///////////////////////////////////////////////////////////////////////////////
//HELPER FUNCTIONS - MISC
///////////////////////////////////////////////////////////////////////////////
void displayQueues(Queue* hi, Queue* mid, Queue* low)
{
    printQueueInfo(hi);
    printQueueInfo(mid);
    printQueueInfo(low);
}

bool newProcessArrival()
{
    srand((unsigned int)(time(NULL) + global_time_modifier)); //seed rand function
    global_time_modifier++;

    if(rand() % 4 == 1)
        return true;
    else 
        return false;
}

void newProcessAlert(char priority[])
{
    printf("+*****************************************+\n");
    printf("| NEW PROCESS ADDED TO %s PRIORITY QUEUE |\n", priority);
    printf("+*****************************************+\n");
}

void priorityQueueUpgradeAlert()
{
    printf("\033[34m+<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>+\n");
    printf("| PROCESS UPGRADED TO HIGH PRIORITY QUEUE  |\n");
    printf("+<<<<<<<<<<<<<<<<<<<<<>>>>>>>>>>>>>>>>>>>>>+\033[0m\n");
}

void priorityQueueDowngradeAlert()
{
    printf("\033[35m+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+\n");
    printf("|  PROCESS DOWNGRADED TO LOWER PRIORITY QUEUE |\n");
    printf("+~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~+\033[0m\n");
}