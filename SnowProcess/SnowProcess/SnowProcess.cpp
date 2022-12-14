/*++

Module Name:

    SnowProcess.c

Abstract:

    This is the main module of the SnowProcess miniFilter driver.

Environment:

    Kernel mode

--*/

#include <fltKernel.h>
#include <dontuse.h>
#include <ntddk.h>

#include "AutoLock.h"
#include "SnowProcess.h"
#include "ListOperations.h"

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


PFLT_FILTER gFilterHandle;
ULONG_PTR OperationStatusCtx = 1;

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG gTraceFlags = 0;
UNICODE_STRING currentNameExe = RTL_CONSTANT_STRING(L"\\Device\\HarddiskVolume2\\x64\\Release\\UserModeSnowProcess.exe");

#define PT_DBG_PRINT( _dbgLevel, _string )          \
    (FlagOn(gTraceFlags,(_dbgLevel)) ?              \
        DbgPrint _string :                          \
        ((int)0))




SnowProcesses WhiteProcesses;

/*************************************************************************
    Prototypes
*************************************************************************/

EXTERN_C_START

NTSTATUS ZwQueryInformationProcess(
    _In_      HANDLE           ProcessHandle,
    _In_      PROCESSINFOCLASS ProcessInformationClass,
    _Out_     PVOID            ProcessInformation,
    _In_      ULONG            ProcessInformationLength,
    _Out_opt_ PULONG           ReturnLength
);


DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    );

NTSTATUS
SnowProcessInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    );

VOID
SnowProcessInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

VOID
SnowProcessInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    );

NTSTATUS
SnowProcessUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    );

NTSTATUS
SnowProcessInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SnowProcessPreOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

FLT_PREOP_CALLBACK_STATUS
SnowProcessPreSetInformation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

FLT_PREOP_CALLBACK_STATUS SnowProcessPreCreate
(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

VOID
SnowProcessOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    );

FLT_POSTOP_CALLBACK_STATUS
SnowProcessPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    );

FLT_PREOP_CALLBACK_STATUS
SnowProcessPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    );

BOOLEAN
SnowProcessDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    );


EXTERN_C_END

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, SnowProcessUnload)
#pragma alloc_text(PAGE, SnowProcessInstanceQueryTeardown)
#pragma alloc_text(PAGE, SnowProcessInstanceSetup)
#pragma alloc_text(PAGE, SnowProcessInstanceTeardownStart)
#pragma alloc_text(PAGE, SnowProcessInstanceTeardownComplete)
#endif

//
//  operation registration
//

CONST FLT_OPERATION_REGISTRATION Callbacks[] = {
    { IRP_MJ_CREATE, 0, SnowProcessPreOperation, nullptr},    
    { IRP_MJ_SET_INFORMATION, 0, SnowProcessPreSetInformation, nullptr },
    { IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//

CONST FLT_REGISTRATION FilterRegistration = {

    sizeof( FLT_REGISTRATION ),         //  Size
    FLT_REGISTRATION_VERSION,           //  Version
    0,                                  //  Flags

    NULL,                               //  Context
    Callbacks,                          //  Operation callbacks

    SnowProcessUnload,                           //  MiniFilterUnload

    SnowProcessInstanceSetup,                    //  InstanceSetup
    SnowProcessInstanceQueryTeardown,            //  InstanceQueryTeardown
    SnowProcessInstanceTeardownStart,            //  InstanceTeardownStart
    SnowProcessInstanceTeardownComplete,         //  InstanceTeardownComplete

    NULL,                               //  GenerateFileName
    NULL,                               //  GenerateDestinationFileName
    NULL                                //  NormalizeNameComponent

};



NTSTATUS
SnowProcessInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
/*++

Routine Description:

    This routine is called whenever a new instance is created on a volume. This
    gives us a chance to decide if we need to attach to this volume or not.

    If this routine is not defined in the registration structure, automatic
    instances are always created.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Flags describing the reason for this attach request.

Return Value:

    STATUS_SUCCESS - attach
    STATUS_FLT_DO_NOT_ATTACH - do not attach

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("SnowProcess!SnowProcessInstanceSetup: Entered\n") );

    return STATUS_SUCCESS;
}


NTSTATUS
SnowProcessInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This is called when an instance is being manually deleted by a
    call to FltDetachVolume or FilterDetach thereby giving us a
    chance to fail that detach request.

    If this routine is not defined in the registration structure, explicit
    detach requests via FltDetachVolume or FilterDetach will always be
    failed.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Indicating where this detach request came from.

Return Value:

    Returns the status of this operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("SnowProcess!SnowProcessInstanceQueryTeardown: Entered\n") );

    return STATUS_SUCCESS;
}


VOID
SnowProcessInstanceTeardownStart (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the start of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("SnowProcess!SnowProcessInstanceTeardownStart: Entered\n") );
}


VOID
SnowProcessInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
/*++

Routine Description:

    This routine is called at the end of instance teardown.

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance and its associated volume.

    Flags - Reason why this instance is being deleted.

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("SnowProcess!SnowProcessInstanceTeardownComplete: Entered\n") );
}



/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/
bool isDeleteAllowed(_In_ PFLT_CALLBACK_DATA data)
{
    PFLT_FILE_NAME_INFORMATION nameInfo = nullptr;
    bool allow = true;
    NTSTATUS status;
    do
    {
        status = FltGetFileNameInformation(data,
            FLT_FILE_NAME_QUERY_DEFAULT | FLT_FILE_NAME_NORMALIZED, &nameInfo);
        if (!NT_SUCCESS(status))
            break;

        status = FltParseFileNameInformation(nameInfo);
        if (!NT_SUCCESS(status))
            break;

        UNICODE_STRING fullPath;
        fullPath.Length = nameInfo->Volume.Length + nameInfo->ParentDir.Length;
        fullPath.Buffer = nameInfo->ParentDir.Buffer;
        if(fullPath.Buffer != NULL)
        {
            UNICODE_STRING path = RTL_CONSTANT_STRING(L"\\x64");

            if (RtlPrefixUnicodeString(&path, &fullPath, false))
                allow = false;
        }
    } while (false);
    if (nameInfo)
        FltReleaseFileNameInformation(nameInfo);
    return allow;
}



void InitDispathRoutines(PDRIVER_OBJECT DriverObject)
{
    DriverObject->DriverUnload = SnowProcessUnloadDriver;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = SnowProcessCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = SnowProcessCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = SnowProcessDeviceControl;
}


void SnowProcessUnloadDriver(PDRIVER_OBJECT DriverObject)
{
    PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, true);
    UNICODE_STRING symbolicLink = RTL_CONSTANT_STRING(L"\\??\\SnowProcess");
    IoDeleteSymbolicLink(&symbolicLink);
    IoDeleteDevice(DriverObject->DeviceObject);
    ClearAllProcesses();
}

void OnProcessNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
    UNREFERENCED_PARAMETER(Process);
    UNREFERENCED_PARAMETER(ProcessId);

    if (CreateInfo)
    {
        DbgPrint("Create Process\n");
        
        if (!IsAllowedProcess(CreateInfo->ImageFileName))
            CreateInfo->CreationStatus = STATUS_ACCESS_DENIED; //fail the process 
    }
}

bool IsAllowedProcess(PCUNICODE_STRING ImageName)
{
    UNREFERENCED_PARAMETER(ImageName);
    //TODO: if inside the linked list allowed,otherwise return false
    UNICODE_STRING str = RTL_CONSTANT_STRING(L"bob");
    auto isFound = FindObject();
    if(!isFound)
    {
        DbgPrint("something working\n");
        return true;
    }
    //if (RtlEqualUnicodeString(ImageName, str)
      //  return true;
    
    return true;
}



NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:

    This is the initialization routine for this miniFilter driver.  This
    registers with FltMgr and initializes all global data structures.

Arguments:

    DriverObject - Pointer to driver object created by the system to
        represent this driver.

    RegistryPath - Unicode string identifying where the parameters for this
        driver are located in the registry.

Return Value:

    Routine can return non success error codes.

--*/
{
    PT_DBG_PRINT(PTDBG_TRACE_ROUTINES,
        ("SnowProcess!DriverEntry: Entered\n"));

    if(RegistryPath->Buffer)
        DbgPrint("%ws\n", RegistryPath->Buffer);

    InitWhiteListProcesses();

    NTSTATUS status = true;
    PDEVICE_OBJECT DeviceObject = NULL;
    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\device\\SnowProcess");
    UNICODE_STRING symbolName = RTL_CONSTANT_STRING(L"\\??\\SnowProcess");

    auto symbolicLinkCreated = false;
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
        symbolicLinkCreated = true;

        status = PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, false);
        if (!NT_SUCCESS(status))
        {
            DbgPrint("Failed to register the Driver to callback routine of process Craetion\n");
            break;
        }

        //  Register with FltMgr to tell it our callback routines
        status = FltRegisterFilter(DriverObject, &FilterRegistration,
                                                        &gFilterHandle);
        FLT_ASSERT(NT_SUCCESS(status));
        if (!NT_SUCCESS(status)) 
            break;

        InitDispathRoutines(DriverObject); //init dispath rotuines for the driver create/close/iodevice

        status = FltStartFiltering(gFilterHandle); //  Start filtering i/o

    } while (false);

    //if something got wrong free the resources
    if (!NT_SUCCESS(status))
    {
        if(gFilterHandle)
            FltUnregisterFilter(gFilterHandle);
        if (symbolicLinkCreated)
            IoDeleteSymbolicLink(&symbolName);
        if (DeviceObject)
            IoDeleteDevice(DeviceObject);
    }

    return status;
}

NTSTATUS
SnowProcessUnload (
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
    )
/*++

Routine Description:

    This is the unload routine for this miniFilter driver. This is called
    when the minifilter is about to be unloaded. We can fail this unload
    request if this is not a mandatory unload indicated by the Flags
    parameter.

Arguments:

    Flags - Indicating if this is a mandatory unload.

Return Value:

    Returns STATUS_SUCCESS.

--*/
{
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("SnowProcess!SnowProcessUnload: Entered\n") );

    FltUnregisterFilter( gFilterHandle );

    return STATUS_SUCCESS;
}


/*************************************************************************
    MiniFilter callback routines.
    MiniFilter callback routines.
*************************************************************************/
NTSTATUS IsUserProcess(PFLT_CALLBACK_DATA Data)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG retLength = 0;
    ULONG pniSize = 512;
    PUNICODE_STRING processName = nullptr;

    auto process = PsGetThreadProcess(Data->Thread);

    //get handle to the process
    HANDLE hProcess;
    status = ObOpenObjectByPointer(process, OBJ_KERNEL_HANDLE,
        nullptr, 0, nullptr, KernelMode, &hProcess);
    if (!NT_SUCCESS(status))
        return STATUS_UNSUCCESSFUL;

    //allocate memory to process name
    processName = (PUNICODE_STRING)ExAllocatePool2(POOL_FLAG_PAGED, pniSize, DRIVER_TAG);
    if (processName == NULL)
        return STATUS_INSUFFICIENT_RESOURCES; // check what needed to return

    status = ZwQueryInformationProcess(hProcess, ProcessImageFileName, processName, pniSize, &retLength);

    if (!NT_SUCCESS(status))
    {
        DbgPrint("**0x%08X**\n", status);
        ExFreePoolWithTag(processName, DRIVER_TAG);
        if (hProcess)
            ZwClose(hProcess);
        return STATUS_UNSUCCESSFUL;
    }

    
    if (processName)
    {
        DbgPrint("%ws\n%ws\n", processName->Buffer, currentNameExe.Buffer);
        if (!RtlEqualUnicodeString(processName, &currentNameExe, FALSE)) //currentName exe stands for 
        {
            status = STATUS_UNSUCCESSFUL;
            DbgPrint("blocked\n");
        }
        else
        {
            status = STATUS_SUCCESS;
            DbgPrint("Sucess\n");
        }
            
    }

    if (hProcess)
        ZwClose(hProcess);
    
    ExFreePoolWithTag(processName, DRIVER_TAG);

    return status;
}


FLT_PREOP_CALLBACK_STATUS SnowProcessPreOperation
     (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER(CompletionContext);
    NTSTATUS status;
    if (Data->RequestorMode == KernelMode)
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    
    // checks if UserMode(Our Program) trying to modify the process
   
    auto& params = Data->Iopb->Parameters.Create;
    if (params.Options & FILE_DELETE_ON_CLOSE)
    {
        DbgPrint("Delete on close: %wZ\n", FltObjects->FileObject->FileName);
        
        if (!isDeleteAllowed(Data))
        {
            Data->IoStatus.Status = STATUS_ACCESS_DENIED;
            return FLT_PREOP_COMPLETE;
        }
    }
    if (FlagOn(Data->Iopb->Parameters.Create.SecurityContext->DesiredAccess,
        FILE_WRITE_DATA | FILE_APPEND_DATA |
        DELETE | FILE_WRITE_ATTRIBUTES | FILE_WRITE_EA |
        WRITE_DAC | WRITE_OWNER | ACCESS_SYSTEM_SECURITY)) 
    {
        status = IsUserProcess(Data);
        //TODO: handle status if needed
        if (NT_SUCCESS(status))
            return FLT_PREOP_SUCCESS_NO_CALLBACK;

        if (!isDeleteAllowed(Data))
        {
            Data->IoStatus.Status = STATUS_ACCESS_DENIED;
            return FLT_PREOP_COMPLETE;
        }
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}



FLT_PREOP_CALLBACK_STATUS
SnowProcessPreSetInformation(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{    
    UNREFERENCED_PARAMETER(CompletionContext);
    UNREFERENCED_PARAMETER(FltObjects);
    NTSTATUS status = STATUS_SUCCESS;

    if (Data->RequestorMode == KernelMode)
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    auto& params = Data->Iopb->Parameters.SetFileInformation;
    if (params.FileInformationClass != FileDispositionInformation &&
        params.FileInformationClass != FileDispositionInformationEx)
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    auto info = (FILE_DISPOSITION_INFORMATION* )params.InfoBuffer;

    if (!info->DeleteFile)
        return FLT_PREOP_SUCCESS_NO_CALLBACK;
    if(isDeleteAllowed(Data))
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    status = IsUserProcess(Data);
    if(NT_SUCCESS(status))
        return FLT_PREOP_SUCCESS_NO_CALLBACK;

    Data->IoStatus.Status = STATUS_ACCESS_DENIED;
    return FLT_PREOP_COMPLETE;
}


VOID
SnowProcessOperationStatusCallback (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
    _In_ NTSTATUS OperationStatus,
    _In_ PVOID RequesterContext
    )
/*++

Routine Description:

    This routine is called when the given operation returns from the call
    to IoCallDriver.  This is useful for operations where STATUS_PENDING
    means the operation was successfully queued.  This is useful for OpLocks
    and directory change notification operations.

    This callback is called in the context of the originating thread and will
    never be called at DPC level.  The file object has been correctly
    referenced so that you can access it.  It will be automatically
    dereferenced upon return.

    This is non-pageable because it could be called on the paging path

Arguments:

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    RequesterContext - The context for the completion routine for this
        operation.

    OperationStatus -

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( FltObjects );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("SnowProcess!SnowProcessOperationStatusCallback: Entered\n") );

    PT_DBG_PRINT( PTDBG_TRACE_OPERATION_STATUS,
                  ("SnowProcess!SnowProcessOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
                   OperationStatus,
                   RequesterContext,
                   ParameterSnapshot->MajorFunction,
                   ParameterSnapshot->MinorFunction,
                   FltGetIrpName(ParameterSnapshot->MajorFunction)) );
}


FLT_POSTOP_CALLBACK_STATUS
SnowProcessPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_opt_ PVOID CompletionContext,
    _In_ FLT_POST_OPERATION_FLAGS Flags
    )
/*++

Routine Description:

    This routine is the post-operation completion routine for this
    miniFilter.

    This is non-pageable because it may be called at DPC level.

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The completion context set in the pre-operation routine.

    Flags - Denotes whether the completion is successful or is being drained.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );
    UNREFERENCED_PARAMETER( Flags );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("SnowProcess!SnowProcessPostOperation: Entered\n") );

    return FLT_POSTOP_FINISHED_PROCESSING;
}


FLT_PREOP_CALLBACK_STATUS
SnowProcessPreOperationNoPostOperation (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _Flt_CompletionContext_Outptr_ PVOID *CompletionContext
    )
/*++

Routine Description:

    This routine is a pre-operation dispatch routine for this miniFilter.

    This is non-pageable because it could be called on the paging path

Arguments:

    Data - Pointer to the filter callbackData that is passed to us.

    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    CompletionContext - The context for the completion routine for this
        operation.

Return Value:

    The return value is the status of the operation.

--*/
{
    UNREFERENCED_PARAMETER( Data );
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( CompletionContext );

    PT_DBG_PRINT( PTDBG_TRACE_ROUTINES,
                  ("SnowProcess!SnowProcessPreOperationNoPostOperation: Entered\n") );

    // This template code does not do anything with the callbackData, but
    // rather returns FLT_PREOP_SUCCESS_NO_CALLBACK.
    // This passes the request down to the next miniFilter in the chain.

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}


BOOLEAN
SnowProcessDoRequestOperationStatus(
    _In_ PFLT_CALLBACK_DATA Data
    )
/*++

Routine Description:

    This identifies those operations we want the operation status for.  These
    are typically operations that return STATUS_PENDING as a normal completion
    status.

Arguments:

Return Value:

    TRUE - If we want the operation status
    FALSE - If we don't

--*/
{
    PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

    //
    //  return boolean state based on which operations we are interested in
    //

    return (BOOLEAN)

            //
            //  Check for oplock operations
            //

             (((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
               ((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK)  ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK)   ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
                (iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

              ||

              //
              //    Check for directy change notification
              //

              ((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
               (iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
             );
}
