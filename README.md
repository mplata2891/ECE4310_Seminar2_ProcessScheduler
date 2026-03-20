# ECE4310_Seminar2_ProcessScheduler

**Course:** ECE 4310 — Operating Systems for Embedded Applications  
**Institution:** Cal Poly Pomona  
**Instructor:** Prof. Liviu Oniciuc  
**Assignment:** Seminar Two — Process Scheduler  
**Group:** A6  
**Members:** Mike Plata & Jessica Ramirez  
**Due Date:** March 5, 2024  

---

## Overview

A single-file C simulation of a **Multi-Level Feedback Queue (MLFQ)** scheduler — one of the most widely used CPU scheduling algorithms in real operating systems (Linux's CFS and Windows NT both draw from the same ideas).

The program spins up between 5 and 10 randomly generated processes, sorts them into three priority queues, and then runs the MLFQ scheduler for `2 × (initial process count)` iterations. Each iteration processes the highest-priority non-empty queue, updates wait counters across all queues, promotes any starving processes, and has a random chance of injecting a brand-new process — loosely simulating real workload arrival.

All output is color-coded in the terminal (ANSI escape codes):

| Color | Meaning |
|---|---|
| 🟡 Yellow | Active process info box |
| 🟢 Green | Queue state display |
| 🔴 Red | Initial process count announcement |
| 🔵 Blue | Priority upgrade alert |
| 🟣 Magenta | Priority downgrade alert |

---

## How to Compile & Run

```bash
gcc processScheduler.c -o processScheduler
./processScheduler
```

Requires a C99-or-later compiler. No external dependencies beyond the standard library.

> **Note:** ANSI color output renders correctly in most Unix/Linux terminals and macOS. On Windows, use Windows Terminal or enable virtual terminal processing.

---

## Scheduling Algorithm — Multi-Level Feedback Queue (MLFQ)

The MLFQ scheduler is implemented in `multiLevelFeedbackScheduler()` and operates across three queues on every call:

### The Three Queues

| Queue | Priority Level | Time Quantum | Scheduling Policy |
|---|---|---|---|
| High | 0 | 4 ms | Round-Robin with feedback (demotes after 3 passes) |
| Mid | 1 | 16 ms | Round-Robin with feedback (demotes after 3 passes) |
| Low | 2 | 32 ms | First-Come First-Serve (never demotes) |

### Each Scheduler Tick (one call to `multiLevelFeedbackScheduler`)

1. **Select queue** — The highest-priority non-empty queue is chosen. High beats Mid beats Low.
2. **Run active process** — The head process is dequeued, its state is set to `ACTIVE`, its completion time is decremented by the queue's time quantum, then it is printed before and after.
3. **Feedback decision:**
   - If the process finishes (`completion_time_ms == 0`) → freed from memory (`COMPLETE`).
   - If it has been through the current queue fewer than 3 times → re-enqueued at the back of the same queue.
   - If it has been through the current queue 3 or more times → demoted to the next lower queue (magenta alert).
   - The low-priority queue uses FCFS: an unfinished process goes back to the **front**, not the back.
4. **Update wait counters** — Every process in every non-empty queue that was *not* just run has its `wait_counter` incremented.
5. **Starvation check** — Any process in Mid or Low with `wait_counter == 4` is immediately promoted to the High queue (blue alert). Counter resets on the next run.
6. **New process arrival** — There is a 1-in-4 chance a brand-new process is created and sorted into the appropriate queue.

---

## Data Structures

### `Process` (singly-linked list node)

```c
struct Process {
    Process* next;                        // pointer for linked-list queue
    int      id;                          // auto-incrementing global ID
    char     category[30];               // "System Process" | "Interactive Process" | "Batch Process"
    char     state[10];                  // "READY" | "ACTIVE" | "COMPLETE"
    int      priority_level;             // 0 = high, 1 = mid, 2 = low
    int      completion_time_ms;         // remaining CPU time needed (ms)
    int      processed_through_current_queue; // tracks passes for demotion logic
    int      wait_counter;               // tracks idle time for anti-starvation
};
```

### `Queue` (linked-list wrapper)

```c
typedef struct {
    Process* head;
    Process* tail;
    int priority_level;
    int time_quantum_ms;
    int processes_in_queue;
} Queue;
```

Queues are implemented as **FIFO singly-linked lists** with O(1) enqueue (append to tail) and O(1) dequeue (remove from head). `removeFromQueue()` provides O(n) arbitrary removal used by the starvation upgrade logic.

---

## Function Reference

### Process Helpers

| Function | Description |
|---|---|
| `createNewProcess()` | Allocates and initializes a new `Process` on the heap. Assigns a global ID, sets state to `READY`, and randomizes priority and completion time. |
| `initializeProcessPriority(process)` | Randomly assigns `priority_level` 0–2 and sets the matching `category` string. |
| `initializeProcessCompletionTime(process)` | Randomly assigns `completion_time_ms` to one of: 4, 16, 32, or 64 ms. |
| `sortProcess(hi, mid, low, process)` | Routes a new process into the correct queue based on its `priority_level`. |
| `printProcessInfo(process)` | Prints a formatted yellow box with all process fields to stdout. |

### Queue Helpers

| Function | Description |
|---|---|
| `createNewQueue(priority, quantum)` | Allocates and initializes a new `Queue` on the heap. |
| `enQueue(queue, process)` | Appends a process to the tail. Returns 0 on success, -1 on null queue. |
| `deQueue(queue)` | Removes and returns the head process. Returns NULL if empty. |
| `returnToFrontOfQueue(queue, process)` | Prepends a process to the head — used by the FCFS low queue. |
| `removeFromQueue(queue, process)` | Removes an arbitrary process by cycling the queue. O(n). Used for starvation upgrades. |
| `queueIsNotEmpty(queue)` | Returns true if the queue has at least one process. |
| `printQueueInfo(queue)` | Prints a formatted green box showing queue priority, time quantum, process count, head ID, intermediate IDs, and tail ID. |

### Scheduler Functions

| Function | Description |
|---|---|
| `multiLevelFeedbackScheduler(hi, mid, low)` | Main scheduler tick. Selects the active queue, processes one job, updates counters, checks starvation, and handles new arrivals. |
| `feedbackQueue(active, lower, process)` | Runs a process from a feedback queue (High or Mid). Handles re-queue, demotion, or freeing. |
| `firstComeFirstServeQueue(active, process)` | Runs a process from the Low queue. Unfinished jobs return to the front (FCFS semantics). |
| `runActiveProcess(queue, process)` | Sets state to `ACTIVE`, decrements completion time by the queue's quantum, resets `wait_counter`, prints before/after state. |
| `updateWaitCountersMultipleQueues(q1, q2, q3, last)` | Calls `updateWaitCounters` on every non-empty queue. |
| `updateWaitCounters(queue, last_run)` | Cycles through a queue and increments `wait_counter` on every process that was not just run. |
| `upgradeStarvingProcesses(hi, mid, low)` | Checks Mid and Low queues for starving processes and promotes them. |
| `upgradeProcessPriority(lower, higher)` | Scans a queue and moves any process with `wait_counter == 4` to the higher-priority queue. |

### Misc / Display

| Function | Description |
|---|---|
| `displayQueues(hi, mid, low)` | Calls `printQueueInfo` on all three queues. |
| `newProcessArrival()` | Returns true with 25% probability — simulates unpredictable process arrival. |
| `newProcessAlert(priority)` | Prints a banner when a new process joins a queue. |
| `priorityQueueUpgradeAlert()` | Prints a blue banner on starvation promotion. |
| `priorityQueueDowngradeAlert()` | Prints a magenta banner on feedback demotion. |

---

## Global Variables

| Variable | Purpose |
|---|---|
| `global_nextProcess_id` | Auto-incrementing integer assigned to each new process as its unique ID. |
| `global_time_modifier` | Incremented before every `srand()` call to reduce seed collisions when multiple random values are needed in quick succession. |

---

## Design Notes & Known Limitations

- **`srand()` seeding strategy** — The code re-seeds `rand()` before almost every random call using `time(NULL) + global_time_modifier`. This is a workaround for the low resolution of `time()` (1-second granularity), which would otherwise produce identical seeds across calls made in the same second. A cleaner approach for production code would be to seed once at startup and use a higher-resolution source like `clock_gettime()`.

- **`removeFromQueue()` complexity** — Arbitrary removal is O(n) and rotates the queue to find the target. This is acceptable for a simulation with a small number of processes but would not scale to a production scheduler.

- **Memory management** — Processes that complete (`completion_time_ms == 0`) are `free()`d immediately. Queues themselves are allocated but never freed on program exit — not a concern for a short-lived simulation but worth noting.

- **Infinite loop option** — The `main()` function contains a commented-out `do { ... } while (true)` block that continuously runs the scheduler. The active loop runs for `2 × random_processes` iterations to keep output finite during testing.

- **Starvation threshold** — `LONG_WAIT_TIME` is hardcoded to `4` in `upgradeProcessPriority()`. With a short simulation run, this threshold may trigger frequently; in a longer run it would need tuning relative to actual time quanta.
