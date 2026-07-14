# OS Scheduler & Memory Management  

This project consists of two phases: **CPU Scheduling** and **Memory Management**. It is implemented in **C** for **Linux** and explores process scheduling, inter-process communication (IPC), and memory allocation using the **buddy memory allocation system**.  

## Features  

### **Phase 1: OS Scheduler**  
- Implements **three CPU scheduling algorithms**:  
  - **Non-preemptive Highest Priority First (HPF)**  
  - **Shortest Remaining Time Next (SRTN)**  
  - **Round Robin (RR)**  
- Manages process execution using **IPC mechanisms**.  
- Tracks process states using a **Process Control Block (PCB)**.  
- Generates performance logs, including:  
  - **CPU utilization**  
  - **Average turnaround time**  
  - **Average waiting time**  
  - **Standard deviation of turnaround time**  

### **Phase 2: Memory Management**  
- Extends the scheduler to include **buddy memory allocation**.  
- Allocates and deallocates memory dynamically as processes enter and exit.  
- Implements a **1024-byte memory space**, with process sizes up to **256 bytes**.  
- Logs memory allocation and deallocation events.  

## Input & Output  

### **Input File (`processes.txt`)**  
Contains process details in tab-separated format:  
```text
#id arrival runtime priority memsize
1    1       6       5        200
2    3       3       3        170
```  

### **Output Files**  
- **`scheduler.log`** → Logs process scheduling events.  
- **`scheduler.perf`** → Contains CPU performance metrics.  
- **`memory.log`** → Logs memory allocation and deallocation.  

## How to Build and Run

Both phases include a `Makefile` to simplify compilation and execution. Open your terminal and navigate to either the `Phase 1` or `Phase 2` directory.

### 1. Compile the Project
Run the following command to compile all C files into executables:
```bash
make build
```

### 2. Run the Simulation
To automatically generate a random `processes.txt` file and start the scheduler simulation, run:
```bash
make run
```
*Note: The `process_generator` will typically prompt you in the terminal to select the scheduling algorithm and its parameters (like the quantum time for Round Robin).*

### 3. Clean up
To remove all compiled `.out` executables and the generated `processes.txt` file, run:
```bash
make clean
```

## Contributors  
Developed as part of Cairo University’s **Operating Systems** course.  
