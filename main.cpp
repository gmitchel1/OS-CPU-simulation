#include <iostream>
#include <fstream>
#include <cstring>
#include <string>

using namespace std;

enum State {
    NEW_STATE,
    READY_STATE,
    RUNNING_STATE,
    TERMINATED_STATE
};

struct PCB {
    char pid[20];
    int arrival;
    int burst;
    int remaining;
    int priority;
    State state;
    int startTime;
    int completionTime;
    bool started;
};

string stateToString(State s) {
    switch (s) {
        case NEW_STATE: return "NEW";
        case READY_STATE: return "READY";
        case RUNNING_STATE: return "RUNNING";
        case TERMINATED_STATE: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

struct ReadyQueue {
    PCB** arr;
    int size;
    int capacity;

    ReadyQueue(int cap = 100) {
        capacity = cap;
        size = 0;
        arr = new PCB*[capacity];
    }

    ~ReadyQueue() {
        delete[] arr;
    }

    bool isEmpty() {
        return size == 0;
    }

    void enqueue(PCB* p) {
        if (size < capacity) {
            arr[size++] = p;
        }
    }

    PCB* dequeueFront() {
        if (size == 0) return nullptr;
        PCB* front = arr[0];
        for (int i = 1; i < size; i++) {
            arr[i - 1] = arr[i];
        }
        size--;
        return front;
    }

    void removeAt(int index) {
        if (index < 0 || index >= size) return;
        for (int i = index + 1; i < size; i++) {
            arr[i - 1] = arr[i];
        }
        size--;
    }

    PCB* getAt(int index) {
        if (index < 0 || index >= size) return nullptr;
        return arr[index];
    }
};

void printPCB(ofstream& out, PCB* p) {
    out << "PID = " << p->pid
        << " Arr =" << p->arrival
        << " Burst =" << p->burst
        << " Rem =" << p->remaining
        << " Prio =" << p->priority
        << " State = " << stateToString(p->state)
        << "\n";
}

void printTrace(ofstream& out, int time, PCB* running, ReadyQueue& ready) {
    out << "Time " << time << ":\n";
    out << "RUNNING :\n";
    if (running == nullptr) {
        out << "IDLE\n";
    } else {
        printPCB(out, running);
    }

    out << "READY :\n";
    for (int i = 0; i < ready.size; i++) {
        printPCB(out, ready.getAt(i));
    }
    out << "\n";
}

int loadProcesses(const char* filename, PCB*& processes) {
    ifstream in(filename);
    if (!in) {
        cout << "Error opening input file.\n";
        return 0;
    }

    string header1, header2, header3, header4;
    in >> header1 >> header2 >> header3 >> header4;

    int capacity = 100;
    int count = 0;
    processes = new PCB[capacity];

    string pid;
    int arrival, burst, priority;

    while (in >> pid >> arrival >> burst >> priority) {
        strcpy(processes[count].pid, pid.c_str());
        processes[count].arrival = arrival;
        processes[count].burst = burst;
        processes[count].remaining = burst;
        processes[count].priority = priority;
        processes[count].state = NEW_STATE;
        processes[count].startTime = -1;
        processes[count].completionTime = -1;
        processes[count].started = false;
        count++;
    }

    in.close();
    return count;
}

bool allTerminated(PCB* processes, int n) {
    for (int i = 0; i < n; i++) {
        if (processes[i].state != TERMINATED_STATE) {
            return false;
        }
    }
    return true;
}

void admitNewArrivals(PCB* processes, int n, int time, ReadyQueue& ready) {
    for (int i = 0; i < n; i++) {
        if (processes[i].state == NEW_STATE && processes[i].arrival == time) {
            processes[i].state = READY_STATE;
            ready.enqueue(&processes[i]);
        }
    }
}

int pickShortestRemaining(ReadyQueue& ready) {
    if (ready.isEmpty()) return -1;
    int best = 0;
    for (int i = 1; i < ready.size; i++) {
        if (ready.getAt(i)->remaining < ready.getAt(best)->remaining) {
            best = i;
        }
    }
    return best;
}

int pickHighestPriority(ReadyQueue& ready) {
    if (ready.isEmpty()) return -1;
    int best = 0;
    for (int i = 1; i < ready.size; i++) {
        if (ready.getAt(i)->priority < ready.getAt(best)->priority) {
            best = i;
        }
    }
    return best;
}

void printGantt(ofstream& out, string* gantt, int len) {
    out << "GANTT Chart:\n";
    for (int i = 0; i < len; i++) {
        out << "[" << i << "-" << i + 1 << ":" << gantt[i] << "] ";
    }
    out << "\n\n";
}

void printMetrics(ofstream& out, PCB* processes, int n) {
    out << "Metrics:\n";
    int totalWait = 0;
    int totalTurn = 0;
    int totalResp = 0;

    for (int i = 0; i < n; i++) {
        int turnaround = processes[i].completionTime - processes[i].arrival;
        int waiting = turnaround - processes[i].burst;
        int response = processes[i].startTime - processes[i].arrival;

        totalWait += waiting;
        totalTurn += turnaround;
        totalResp += response;

        out << processes[i].pid
            << " WT=" << waiting
            << " TAT=" << turnaround
            << " RT=" << response << "\n";
    }

    out << "\nAverage WT = " << (double)totalWait / n << "\n";
    out << "Average TAT = " << (double)totalTurn / n << "\n";
    out << "Average RT = " << (double)totalResp / n << "\n";
}

void resetProcesses(PCB* processes, int n) {
    for (int i = 0; i < n; i++) {
        processes[i].remaining = processes[i].burst;
        processes[i].state = NEW_STATE;
        processes[i].startTime = -1;
        processes[i].completionTime = -1;
        processes[i].started = false;
    }
}

void simulateFCFS(PCB* processes, int n, const char* outputFile) {
    resetProcesses(processes, n);

    ofstream out(outputFile);
    ReadyQueue ready(100);
    PCB* running = nullptr;
    string gantt[1000];
    int ganttLen = 0;
    int time = 0;

    while (!allTerminated(processes, n)) {
        admitNewArrivals(processes, n, time, ready);

        if (running == nullptr && !ready.isEmpty()) {
            running = ready.dequeueFront();
            running->state = RUNNING_STATE;
            if (!running->started) {
                running->started = true;
                running->startTime = time;
            }
        }

        if (running != nullptr) {
            running->remaining--;
        }

        printTrace(out, time, running, ready);

        if (running != nullptr) {
            gantt[ganttLen++] = running->pid;
        } else {
            gantt[ganttLen++] = "IDLE";
        }

        if (running != nullptr && running->remaining == 0) {
            running->completionTime = time + 1;
            running->state = TERMINATED_STATE;
            running = nullptr;
        }

        time++;
    }

    printGantt(out, gantt, ganttLen);
    printMetrics(out, processes, n);
    out.close();
}

void simulatePriority(PCB* processes, int n, const char* outputFile) {
    resetProcesses(processes, n);

    ofstream out(outputFile);
    ReadyQueue ready(100);
    PCB* running = nullptr;
    string gantt[1000];
    int ganttLen = 0;
    int time = 0;

    while (!allTerminated(processes, n)) {
        admitNewArrivals(processes, n, time, ready);

        if (running == nullptr && !ready.isEmpty()) {
            int idx = pickHighestPriority(ready);
            running = ready.getAt(idx);
            ready.removeAt(idx);
            running->state = RUNNING_STATE;

            if (!running->started) {
                running->started = true;
                running->startTime = time;
            }
        }

        if (running != nullptr) {
            running->remaining--;
        }

        printTrace(out, time, running, ready);

        if (running != nullptr) {
            gantt[ganttLen++] = running->pid;
        } else {
            gantt[ganttLen++] = "IDLE";
        }

        if (running != nullptr && running->remaining == 0) {
            running->completionTime = time + 1;
            running->state = TERMINATED_STATE;
            running = nullptr;
        }

        time++;
    }

    printGantt(out, gantt, ganttLen);
    printMetrics(out, processes, n);
    out.close();
}

void simulateSRTF(PCB* processes, int n, const char* outputFile) {
    resetProcesses(processes, n);

    ofstream out(outputFile);
    ReadyQueue ready(100);
    PCB* running = nullptr;
    string gantt[1000];
    int ganttLen = 0;
    int time = 0;

    while (!allTerminated(processes, n)) {
        admitNewArrivals(processes, n, time, ready);

        if (running != nullptr) {
            int bestIdx = pickShortestRemaining(ready);
            if (bestIdx != -1 && ready.getAt(bestIdx)->remaining < running->remaining) {
                running->state = READY_STATE;
                ready.enqueue(running);

                running = ready.getAt(bestIdx);
                ready.removeAt(bestIdx);
                running->state = RUNNING_STATE;

                if (!running->started) {
                    running->started = true;
                    running->startTime = time;
                }
            }
        }

        if (running == nullptr && !ready.isEmpty()) {
            int idx = pickShortestRemaining(ready);
            running = ready.getAt(idx);
            ready.removeAt(idx);
            running->state = RUNNING_STATE;

            if (!running->started) {
                running->started = true;
                running->startTime = time;
            }
        }

        if (running != nullptr) {
            running->remaining--;
        }

        printTrace(out, time, running, ready);

        if (running != nullptr) {
            gantt[ganttLen++] = running->pid;
        } else {
            gantt[ganttLen++] = "IDLE";
        }

        if (running != nullptr && running->remaining == 0) {
            running->completionTime = time + 1;
            running->state = TERMINATED_STATE;
            running = nullptr;
        }

        time++;
    }

    printGantt(out, gantt, ganttLen);
    printMetrics(out, processes, n);
    out.close();
}

void simulateRR(PCB* processes, int n, const char* outputFile, int quantum) {
    resetProcesses(processes, n);

    ofstream out(outputFile);
    ReadyQueue ready(100);
    PCB* running = nullptr;
    string gantt[1000];
    int ganttLen = 0;
    int time = 0;
    int sliceUsed = 0;

    while (!allTerminated(processes, n)) {
        admitNewArrivals(processes, n, time, ready);

        if (running == nullptr && !ready.isEmpty()) {
            running = ready.dequeueFront();
            running->state = RUNNING_STATE;
            sliceUsed = 0;

            if (!running->started) {
                running->started = true;
                running->startTime = time;
            }
        }

        if (running != nullptr) {
            running->remaining--;
            sliceUsed++;
        }

        printTrace(out, time, running, ready);

        if (running != nullptr) {
            gantt[ganttLen++] = running->pid;
        } else {
            gantt[ganttLen++] = "IDLE";
        }

        if (running != nullptr && running->remaining == 0) {
            running->completionTime = time + 1;
            running->state = TERMINATED_STATE;
            running = nullptr;
            sliceUsed = 0;
        } else if (running != nullptr && sliceUsed == quantum) {
            running->state = READY_STATE;
            ready.enqueue(running);
            running = nullptr;
            sliceUsed = 0;
        }

        time++;
    }

    printGantt(out, gantt, ganttLen);
    printMetrics(out, processes, n);
    out.close();
}

int main() {
    PCB* processes = nullptr;
    int n = loadProcesses("input.txt", processes);

    if (n == 0) {
        return 1;
    }

    simulateFCFS(processes, n, "output_fcfs.txt");
    simulateSRTF(processes, n, "output_srtf.txt");
    simulatePriority(processes, n, "output_priority.txt");
    simulateRR(processes, n, "output_rr.txt", 2);

    delete[] processes;

    cout << "Simulation complete.\n";
    cout << "Generated:\n";
    cout << "- output_fcfs.txt\n";
    cout << "- output_srtf.txt\n";
    cout << "- output_priority.txt\n";
    cout << "- output_rr.txt\n";

    return 0;
}
