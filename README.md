# Lockdep — Mini-Lockdep Kernel Module

## Done by

| Name | GitHub |
|------|--------|
| Fatema ElZhraa ElFiky | [@fatemaelfiky123](https://github.com/fatema123elfiky) |
| Ansar Osama | [@ansar0yousif](https://github.com/ansar0yousif) |
| Mennatuallah Samir | [@Menna1177 ](https://github.com/Menna1177) |
| Gamal Megahed | [@Gamal-Kobisy](https://github.com/Gamal-Kobisy) |
| Ahmed Ibrahim | [@AhmedIbrahimFCAL](https://github.com/AhmedIbrahimFCAL) |

---

## Overview

This project implements a **Mini-Lockdep** system as a Linux kernel module.  
It simulates the behavior of the Linux kernel's built-in Lockdep tool — which detects deadlocks by tracking lock acquisition order across threads.

---

## What is Lockdep?

**Lockdep** is a runtime locking correctness validator built into the Linux kernel.  
It tracks the order in which locks are acquired and detects potential deadlocks before they happen.

Our **Mini-Lockdep** replicates its core logic:
- Track which locks each thread holds
- Build a dependency graph between locks
- Detect cycles in that graph (cycles = deadlocks)

---

## Project Structure

```
.
├── mini_lockdep.c       # Main kernel module (C)
├── mini_lockdep_test.c  # Plain C version for testing logic
├── Makefile             # Kernel module build file
└── README.md            # This file
```

---

## How It Works

### Step 1 — Tracking
Every time a thread acquires a lock, we record it in a `thread_info` struct:
```c
struct thread_info {
    int pid;
    int held_locks[MAX_LOCKS];
    int lock_count;
};
```

### Step 2 — Dependency Graph
When a thread holds **Lock A** and requests **Lock B**, we add a directed edge:
```
A → B
```
The graph is represented as an adjacency matrix:
```c
int graph[MAX_LOCKS+1][MAX_LOCKS+1];
int graph_degree[MAX_LOCKS+1];
```

### Step 3 — Cycle Detection (DFS)
After every new edge is added, we run a **DFS** using a `Pathstack` (recursion stack) to detect cycles in the directed graph:
```
Cycle found = DEADLOCK detected
```

### Deadlock Example
```
Thread 1: holds Lock 4 → requests Lock 5    (edge: 4→5)
Thread 2: holds Lock 5 → requests Lock 4    (edge: 5→4)

Graph: 4 → 5 → 4   ← CYCLE = DEADLOCK!
```

---

## How to Build & Run

### Plain C (for testing logic)
```bash
gcc -o mini_lockdep mini_lockdep_test.c
./mini_lockdep
```

### Kernel Module
```bash
make                        # compile → produces mini_lockdep.ko
sudo insmod mini_lockdep.ko # load into kernel
dmesg | tail -20            # view output
sudo rmmod mini_lockdep     # unload module
```

---

## Expected Output

```
4-->5-->5-->4-->There is a deadlock here
```

Deadlock is detected after Thread 2 adds edge `5→4`, creating the cycle `4→5→4`.

---

## Design Decisions

- **Simulation only** — no real mutexes are used to avoid kernel hangs
- **Detection after every insert** — cycle check runs after each new dependency edge is added, simulating real-time tracking
- **DFS with Pathstack** — standard directed graph cycle detection using a recursion stack (not just `visited[]`, which would cause false alarms)
- **Fixed-size arrays** — used instead of dynamic structures since the kernel does not support C++ STL

---

## Comparison with Real Lockdep

| Feature | Real Lockdep | Mini-Lockdep |
|---------|-------------|--------------|
| Tracks real kernel locks | ✅ | ❌ (simulation) |
| Dependency graph | ✅ | ✅ |
| Cycle detection | ✅ | ✅ |
| Works as kernel module | ✅ | ✅ |
| Prevents deadlocks | ❌ (detects only) | ❌ (detects only) |
| Overhead | Low (optimized) | Minimal |
