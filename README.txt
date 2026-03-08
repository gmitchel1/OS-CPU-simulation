CPU Scheduling Simulator Project

How to compile:
g++ src/main.cpp -o scheduler

How to run:
./scheduler

Files generated:
output_fcfs.txt
output_srtf.txt
output_priority.txt
output_rr.txt

Assumptions:
1. Lower priority number means higher priority.
2. Round Robin time quantum is 2.
3. SRTF only preempts when a READY process has strictly smaller remaining time.
4. FCFS and Priority are non-preemptive.
5. READY processes are printed in queue order.
6. Input format must match:
   PID Arrival Burst Priority
