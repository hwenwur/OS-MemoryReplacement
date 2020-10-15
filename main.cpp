#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>

#define TOTAL_INSTRUCTION 320
#define VM_CAPACITY 32

using namespace std;

/* 内存块 */
class MemoryBlock
{
public:
    MemoryBlock() : free(true) {}

public:
    bool free; // 是否空闲
};

class PageTableEntry
{
public:
    PageTableEntry() : present(false), realPage(-1), accessedSeq(0) {}

public:
    int realPage;    // 实存页索引
    int accessedSeq; // 最近访问序号，用于 LRU 算法标记。
    bool present;    // 是否已加载到实存
};

int instAddr[TOTAL_INSTRUCTION];
int totals = 0;

int createInstructionAddr(const char *fn)
{
    ifstream inFile;
    inFile.open(fn, ios_base::in);
    if (!inFile.is_open())
        return 1;
    inFile >> totals;
    for (int i = 0; i < totals; i++)
    {
        inFile >> instAddr[i];
    }
    inFile.close();
    return 0;
}

int getFreeMemoryCount(vector<MemoryBlock> m)
{
    vector<MemoryBlock>::iterator it = m.begin();
    int count = 0;
    while (it != m.end())
    {
        if (it->free)
            count++;
        it++;
    }
    return count;
}

// 返回缺页中断次数
int FIFO(int memoryPageCount)
{
    vector<MemoryBlock> memory(memoryPageCount);
    vector<PageTableEntry> PMT(VM_CAPACITY);
    int rear = 0;           // 队列尾
    int front = 0;          // 队列头
    int pageFaultCount = 0; // 缺页中断次数
    int page;
    int i;
    for (i = 0; i < totals; i++)
    {
        page = instAddr[i] / 10;
        if (!PMT[page].present)
        {
            // 该页不在主存中
            // 缺页中断处理程序
            pageFaultCount++;
            if (getFreeMemoryCount(memory) == 0)
            {
                // 没有空闲主存
                // 淘汰 memory[rear]
                memory[rear].free = true;
                for (int j = 0; j < PMT.size(); j++)
                {
                    if (PMT[j].realPage == rear)
                        PMT[j].present = false;
                }
                rear = (1 + rear) % memoryPageCount;
            }
            memory[front].free = false;
            PMT[page].present = true;
            PMT[page].realPage = front;
            front = (1 + front) % memoryPageCount;
        }
    }
    return pageFaultCount;
}

int LRU(int memoryPageCount)
{
    vector<MemoryBlock> memory(memoryPageCount);
    vector<PageTableEntry> PMT(VM_CAPACITY);

    int seq = 0;            // 递增序列，用于标记最近一次访问的时间
    int pageFaultCount = 0; // 缺页中断次数
    int page;
    int i;
    for (i = 0; i < totals; i++)
    {
        page = instAddr[i] / 10;
        if (!PMT[page].present)
        {
            pageFaultCount++;
            if (getFreeMemoryCount(memory) == 0)
            {
                // 没有空闲主存
                // 淘汰 memory[] 中 accessedSeq 最小的
                int min = -1, minIndex = 0;
                for (int j = 0; j < PMT.size(); j++)
                {
                    if (PMT[j].present)
                    {
                        if (min == -1 || min > PMT[j].accessedSeq)
                        {
                            min = PMT[j].accessedSeq;
                            minIndex = j;
                        }
                    }
                }
                if (min == -1)
                    throw "Error: can not find the maxval of PMT[].accessedSeq";
                PageTableEntry *m = &PMT[minIndex];
                m->present = false;
                memory[m->realPage].free = true;
            }
            for (int j = 0; j < memory.size(); j++)
            {
                if (memory[j].free)
                {
                    memory[j].free = false;
                    PMT[page].accessedSeq = ++seq;
                    PMT[page].present = true;
                    PMT[page].realPage = j;
                    break;
                }
            }
        }
        else
        {
            // 命中
            PMT[page].accessedSeq = ++seq;
        }
    }
    return pageFaultCount;
}

int main(int argc, char *argv[])
{
    int r = createInstructionAddr("D:\\Download\\test3dat.dat");
    cout << "r = " << r << endl;
    cout << "totals = " << totals << endl;
    cout << setprecision(4) << fixed;
    for (int i = 4; i <= 32; i++)
    {
        int f1 = FIFO(i);
        int f2 = LRU(i);
        cout << i << " page frames:	 "
             << "FIFO:" << (1 - double(f1) / totals)
             << ", LRU:" << (1 - double(f2) / totals) << endl;
    }
    return 0;
}
