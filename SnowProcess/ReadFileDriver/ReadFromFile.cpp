#include <ntddk.h>

#include "../SnowProcess/AutoLock.h"
#include "../SnowProcess/FastMutex.h"
#include "../SnowProcess/SnowProcess.h"


NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0) {
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = info;
    IoCompleteRequest(Irp, 0);
    return status;
}

void ReadFileUnloadDriver(PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING symbolicLink = RTL_CONSTANT_STRING(L"\\??\\SnowProcess");
    IoDeleteSymbolicLink(&symbolicLink);
    IoDeleteDevice(DriverObject->DeviceObject);   
}

NTSTATUS ReadFileCreateClose(PDEVICE_OBJECT, PIRP Irp)
{
    return CompleteIrp(Irp);
}

NTSTATUS ReadFileDeviceControl(PDEVICE_OBJECT, PIRP Irp)
{
    auto stack = IoGetCurrentIrpStackLocation(Irp);
    auto len = stack->Parameters.DeviceIoControl.InputBufferLength;
    if (len == 0)
        return CompleteIrp(Irp, STATUS_INVALID_BUFFER_SIZE);

    auto code = stack->Parameters.DeviceIoControl.IoControlCode;

    //TODO:split to functions
    switch (code)
    {
    case IOCTL_REQUEST_TYPE_ADD:
    {
        DbgPrint("WeMadeCommuniction!\n");
        auto data = (AllowedProcess*)stack->Parameters.DeviceIoControl.Type3InputBuffer;
        if (!data)
            return CompleteIrp(Irp, STATUS_BUFFER_ALL_ZEROS);


        //TODO:check existing process in the white list create for this a function
        auto newAllowedProcess = (Item<AllowedProcess> *) ExAllocatePool2(POOL_FLAG_PAGED, \
            sizeof(Item<AllowedProcess>), DRIVER_TAG);
        if (!newAllowedProcess)
            return CompleteIrp(Irp, STATUS_FAILED_DRIVER_ENTRY); //TODO: find normal return value

        auto& item = newAllowedProcess->data_;
        item.length_ = data->length_;
        ::memcpy(item.ImageFileName, data->ImageFileName, MaxSizeImageName);
        //AddAllowedProcess(&newAllowedProcess->Entry);
    }break;

    case IOCTL_REQUEST_TYPE_REMOVE:
    {

    }break;

    case IOCTL_REQUEST_TYPE_CLEAR_ALL:
    {
        //ClearAllProcesses();
    }break;
    }

    return CompleteIrp(Irp, STATUS_SUCCESS);
}





extern "C"
NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status = true;
    PDEVICE_OBJECT DeviceObject = NULL;
    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\device\\SnowProcess");
    UNICODE_STRING symbolName = RTL_CONSTANT_STRING(L"\\??\\SnowProcess");

    do
    {
        //init device object
        status = IoCreateDevice(DriverObject, 0, &devName,
            FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
        if (!NT_SUCCESS(status))
        {
            DbgPrint("Failed to create Device Object\n");
            break;
        }

        //TODO:figure out if need direct io for this project
        //DeviceObject->Flags |= DO_DIRECT_IO;

        //init symbolic link
        status = IoCreateSymbolicLink(&symbolName, &devName);
        if (!NT_SUCCESS(status))
        {
            DbgPrint("Failed to create Symbolic Link\n");
            break;
        }
        //symbolicLinkCreated = true;
    
        /*
        DriverObject->DriverUnload = ReadFromFileUnloadDriver;
        DriverObject->MajorFunction[IRP_MJ_CREATE] = ReadFromFileCreateClose;
        DriverObject->MajorFunction[IRP_MJ_CLOSE] = ReadFromFileCreateClose;
        DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ReadFromFileDeviceControl;
        */

    } while (false);

    return status;

}