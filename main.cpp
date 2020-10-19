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

/* 页表条目 */
class PageTableEntry
{
public:
    PageTableEntry() : present(false), realPage(-1) {}

public:
    int realPage; // 实存页索引
    bool present; // 是否已加载到实存
};

class MemoryManager
{
public:
    MemoryManager(int memoryPageCount) : memory(memoryPageCount) {}
    void free(int index) { memory[index].free = true; }
    void allocate(int index) { memory[index].free = false; }
    bool isFree(int index) { return memory[index].free; }
    std::size_t size()
    {
        return memory.size();
    }
    int getFreeMemoryCount()
    {
        vector<MemoryBlock>::iterator it = memory.begin();
        int count = 0;
        while (it != memory.end())
        {
            if (it->free)
                count++;
            it++;
        }
        return count;
    }

private:
    // 内存块
    vector<MemoryBlock> memory;
};

class BasePageRepl
{
public:
    BasePageRepl(int memoryPageCount, int PMTSize = VM_CAPACITY) : mm(memoryPageCount), PMT(PMTSize) {}
    // @return PageFault count
    int execute(int inst[], int length)
    {
        this->instruction = inst;
        this->instLength = length;
        int pageFaultCount = 0;
        int page;
        for (int i = 0; i < length; i++)
        {
            page = inst[i] / 10;
            this->step = i;
            if (!PMT[page].present)
            {
                pageFaultCount++;
                onPageFault(page);
            }
            else
            {
                onAccess(page);
            }
        }
        return pageFaultCount;
    };
    // 缺页中断处理程序
    virtual void onPageFault(int page)
    {
        if (mm.getFreeMemoryCount() == 0)
        {
            removeMemoryBlock();
        }
        int p = allocateMemory(page);
        if (p == -1)
            std::cerr << "Error: failed to allocate memory." << std::endl;
    }
    /*  
    寻找并分配空闲实存，默认使用第一块空闲实存
    成功返回实存页索引，失败返回 -1 
    */
    virtual int allocateMemory(int page)
    {
        for (int j = 0; j < mm.size(); j++)
        {
            if (mm.isFree(j))
            {
                mm.allocate(j);
                PMT[page].present = true;
                PMT[page].realPage = j;
                return j;
            }
        }
        return -1;
    }
    // 虚存页 命中后 事件
    virtual void onAccess(int page) {}
    // 淘汰一个内存块
    virtual int removeMemoryBlock() = 0;

protected:
    MemoryManager mm;
    vector<PageTableEntry> PMT;
    int step;         // 当前执行指令序号
    int *instruction; // 所有指令
    int instLength;
};

class FIFOPageRepl : public BasePageRepl
{
public:
    FIFOPageRepl(int memoryPageCount, int PMTSize = VM_CAPACITY) : BasePageRepl(memoryPageCount, PMTSize), front(0), rear(0) {}
    int removeMemoryBlock() override
    {
        mm.free(rear);
        for (int j = 0; j < PMT.size(); j++)
        {
            if (PMT[j].realPage == rear)
                PMT[j].present = false;
        }
        int t = rear;
        rear = (1 + rear) % mm.size();
        return t;
    }
    int allocateMemory(int page) override
    {
        mm.allocate(front);
        PMT[page].present = true;
        PMT[page].realPage = front;
        front = (1 + front) % mm.size();
        return front;
    }

private:
    int front, rear; // 循环队列头、尾
};

class LRUPageRepl : public BasePageRepl
{
public:
    LRUPageRepl(int memoryPageCount, int PMTSize = VM_CAPACITY) : BasePageRepl(memoryPageCount, PMTSize) {}
    int removeMemoryBlock() override
    {
        vector<int>::iterator it = lruCache.begin();
        for (int j = 0; j < PMT.size(); j++)
        {
            if (PMT[j].present && PMT[j].realPage == *it)
                PMT[j].present = false;
        }
        int tmp = *it;
        mm.free(tmp);
        lruCache.erase(it);
        return tmp;
    }
    int allocateMemory(int page) override
    {
        int j = BasePageRepl::allocateMemory(page);
        lruCache.push_back(j);
        return j;
    }
    void onAccess(int page) override
    {
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

private:
    vector<int> lruCache;
};

class OPTPageRepl : public BasePageRepl
{
public:
    OPTPageRepl(int memoryPageCount, int PMTSize = VM_CAPACITY) : BasePageRepl(memoryPageCount, PMTSize) {}
    int removeMemoryBlock() override
    {
        vector<int> toRemove;
        for (int j = 0; j < PMT.size(); j++)
        {
            if (PMT[j].present)
                toRemove.push_back(j);
        }

        int nPage;
        for (int j = 1 + step; j < instLength && toRemove.size() > 1; j++)
        {
            nPage = instruction[j] / 10;
            for (int z = 0; z < toRemove.size(); z++)
            {
                if (toRemove[z] == nPage)
                {
                    toRemove.erase(z + toRemove.begin());
                    break;
                }
            }
        }
        int tmp = toRemove[0];
        PMT[tmp].present = false;
        mm.free(PMT[tmp].realPage);
        return 0;
    }
};

enum PageReplType
{
    FIFO,
    LRU,
    OPT
};

float hitRate(PageReplType t, int inst[], int totals, int memoryPageCount)
{
    BasePageRepl *pageRepl;
    switch (t)
    {
    case FIFO:
        pageRepl = new FIFOPageRepl(memoryPageCount);
        break;
    case LRU:
        pageRepl = new LRUPageRepl(memoryPageCount);
        break;
    case OPT:
        pageRepl = new OPTPageRepl(memoryPageCount);
        break;
    default:
        throw "Unknow PageReplType t";
        break;
    }
    int f = pageRepl->execute(inst, totals);
    delete pageRepl;
    return 1 - float(f) / totals;
}

int createInstructionAddr(const char *fn, int inst[], int *totals)
{
    ifstream inFile;
    inFile.open(fn, ios_base::in);
    if (!inFile.is_open())
        return 0;
    int len;
    inFile >> len;
    for (int i = 0; i < len; i++)
    {
        inFile >> inst[i];
    }
    *totals = len;
    inFile.close();
    return 1;
}

int main(int argc, char *argv[])
{
    int inst[TOTAL_INSTRUCTION];
    int totals;

    if (argc >= 2)
    {
        if (createInstructionAddr(argv[1], inst, &totals) == 0)
        {
            cout << "Invalid Data!" << endl;
            return 1;
        }
    }
    else
    {
        cout << "not found data file!" << endl;
        return 1;
    }

    ofstream outFile("pageout.dat");

    outFile << "18122606" << endl;

    outFile << setprecision(4) << fixed;
    int f1, f2, f3;
    for (int i = 4; i <= 32; i++)
    {
        outFile << setw(2) << i << " page frames:	 "
                << "FIFO:" << hitRate(FIFO, inst, totals, i)
                << ", LRU:" << hitRate(LRU, inst, totals, i)
                << ", OPT:" << hitRate(OPT, inst, totals, i) << endl;
    }
    return 0;
}
