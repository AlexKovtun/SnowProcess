#pragma once
#include <ntddk.h>
#include "SnowProcessCommon.h"
#include "FastMutex.h"

#define DRIVER_TAG 'wons'

void SnowProcessUnloadDriver(PDRIVER_OBJECT DriverObject);
void OnProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);
bool IsAllowedProcess(PCUNICODE_STRING ImageName);
NTSTATUS SnowProcessCreateClose(_In_ PDEVICE_OBJECT, _In_ PIRP Irp);
NTSTATUS SnowProcessDeviceControl(_In_ PDEVICE_OBJECT, _In_ PIRP Irp);


//structs

struct SnowProcesses
{
	LIST_ENTRY head_;
	int numOfElements_;
	FastMutex mutex_;
};

template <typename T>
struct Item
{
	LIST_ENTRY Entry;
	T data_;
};


//globals
extern SnowProcesses WhiteProcesses;







