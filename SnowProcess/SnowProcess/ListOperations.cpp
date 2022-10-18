#include "ListOperations.h"
#include "SnowProcessCommon.h"
#include "AutoLock.h"
#include "FastMutex.h"


void InitWhiteListProcesses()
{
    InitializeListHead(&WhiteProcesses.head_);
    WhiteProcesses.mutex_.Init();
}

void AddAllowedProcess(LIST_ENTRY* entry)
{
    AutoLock<FastMutex> lock(WhiteProcesses.mutex_);
    ++WhiteProcesses.numOfElements_;
    InsertTailList(&WhiteProcesses.head_, entry);
}

void ClearAllProcesses()
{
    while (!IsListEmpty(&WhiteProcesses.head_))
    {
        auto prevHead = RemoveHeadList(&WhiteProcesses.head_);
        --WhiteProcesses.numOfElements_;
        if(prevHead)
        {
            //getting to Entry address than calculating the need memory to free, all done thorugh Entry
            auto item = CONTAINING_RECORD(prevHead, Item<AllowedProcess>, Entry);
            ExFreePool(item);
        }
    }
}

void DeleteProcess()
{
    UNREFERENCED_PARAMETER(WhiteProcesses);
}
