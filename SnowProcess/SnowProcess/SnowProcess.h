#pragma once
#include <ntddk.h>

void SnowProcessUnloadDriver(PDRIVER_OBJECT DriverObject);
void OnProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);
NTSTATUS SnowProcessCreateClose(_In_ PDEVICE_OBJECT, _In_ PIRP Irp);
NTSTATUS SnowProcessDeviceControl(_In_ PDEVICE_OBJECT, _In_ PIRP Irp);
