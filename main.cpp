#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <deque>

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
    PageTableEntry() : present(false), realPage(-1) {}

public:
    int realPage; // 实存页索引
    bool present; // 是否已加载到实存
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
    vector<int> lruCache;

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
                // 淘汰 lruCache.begin()
                vector<int>::iterator it = lruCache.begin();
                for (int j = 0; j < PMT.size(); j++)
                {
                    if (PMT[j].present && PMT[j].realPage == *it)
                        PMT[j].present = false;
                }
                memory[*it].free = true;
                lruCache.erase(it);
            }

            for (int j = 0; j < memory.size(); j++)
            {
                // 任意找一块空闲实存
                if (memory[j].free)
                {
                    memory[j].free = false;
                    lruCache.push_back(j);
                    PMT[page].present = true;
                    PMT[page].realPage = j;
                    break;
                }
            }
        }
        else
        {
            // 命中
            vector<int>::iterator it = lruCache.begin();
            while (it != lruCache.end())
            {
                if (*it == PMT[page].realPage)
                {
                    // 把命中的页移到最右边
                    int tmp = *it;
                    lruCache.erase(it);
                    lruCache.push_back(tmp);
                    break;
                }
                it++;
            }
        }
    }
    return pageFaultCount;
}

int OPT(int memoryPageCount)
{
    vector<MemoryBlock> memory(memoryPageCount);
    vector<PageTableEntry> PMT(VM_CAPACITY);
    int pageFaultCount = 0;
    int page;
    for (int i = 0; i < totals; i++)
    {
        page = instAddr[i] / 10;
        if (!PMT[page].present)
        {
            // page fault exception
            pageFaultCount++;
            if(getFreeMemoryCount(memory) == 0)
            {
                vector<int> toRemove;
                for(int j = 0; j < PMT.size(); j++)
                {
                    if(PMT[j].present)
                        toRemove.push_back(j);
                }

                int nPage;
                for(int j = 1 + i; j < totals && toRemove.size() > 1; j++)
                {
                    nPage = instAddr[j] / 10;
                    for(int z = 0; z < toRemove.size(); z++)
                    {
                        if(toRemove[z] == nPage)
                        {
                            toRemove.erase(z + toRemove.begin());
                            break;
                        }
                    }
                }
                // if toRemove.size() still more than 1, delete random one.
                int tmp = toRemove[0];
                PMT[tmp].present = false;
                memory[ PMT[tmp].realPage ].free = true;

            }
            for (int j = 0; j < memory.size(); j++)
            {
                // 任意找一块空闲实存
                if (memory[j].free)
                {
                    memory[j].free = false;
                    PMT[page].present = true;
                    PMT[page].realPage = j;
                    break;
                }
            }
        
        }
    }
    return pageFaultCount;
}

int main(int argc, char *argv[])
{
    int r = createInstructionAddr("./test3dat.dat");
    cout << setprecision(4) << fixed;
    for (int i = 4; i <= 32; i++)
    {
        int f1 = FIFO(i);
        int f2 = LRU(i);
        int f3 = OPT(i);
        cout << setw(2) << i << " page frames:	 "
             << "FIFO:" << (1 - double(f1) / totals)
             << ", LRU:" << (1 - double(f2) / totals)
             << ", OPT:" << (1 - double(f3) / totals) << endl;
    }
    return 0;
}
