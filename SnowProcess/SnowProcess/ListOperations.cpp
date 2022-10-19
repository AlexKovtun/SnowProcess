#include "ListOperations.h"
#include "SnowProcess.h"
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
    AutoLock<FastMutex> lock(WhiteProcesses.mutex_); // is it needed?
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

LIST_ENTRY *FindObject()
{
    auto *iter = &WhiteProcesses.head_;
    auto data = CONTAINING_RECORD(iter, Item<AllowedProcess>, data_);
    DbgPrint("%ws\n", data->data_.ImageFileName);
    iter = iter->Blink;
    data = CONTAINING_RECORD(iter, Item<AllowedProcess>, data_);
    DbgPrint("%ws\n", data->data_.ImageFileName);
    while (&WhiteProcesses.head_ != &(*iter))
    {
        data = CONTAINING_RECORD(iter, Item<AllowedProcess>, data_);
        DbgPrint("%ws\n", data->data_.ImageFileName);
        iter = iter->Blink;
    }
    return nullptr;
}
