#include "SnowProcess.h"
#include "SnowProcessCommon.h"
#include "ListOperations.h"

NTSTATUS CompleteIrp(PIRP Irp, NTSTATUS status = STATUS_SUCCESS, ULONG_PTR info = 0) {
	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = info;
	IoCompleteRequest(Irp, 0);
	return status;
}

NTSTATUS SnowProcessCreateClose(PDEVICE_OBJECT, PIRP Irp)
{
	return CompleteIrp(Irp);
}


NTSTATUS SnowProcessDeviceControl(PDEVICE_OBJECT, PIRP Irp)
{
	auto stack = IoGetCurrentIrpStackLocation(Irp);

	auto len = stack->Parameters.DeviceIoControl.InputBufferLength;
	if (len == 0)
		return CompleteIrp(Irp, STATUS_INVALID_BUFFER_SIZE);

	auto code = stack->Parameters.DeviceIoControl.IoControlCode;
	DbgPrint("code: %lu\n", code);

	//TODO:split to functions
	switch(code)
	{ 
		case IOCTL_REQUEST_TYPE_ADD:
		{
			auto data =  (AllowedProcess *)stack->Parameters.DeviceIoControl.Type3InputBuffer;
			if (!data)
				return CompleteIrp(Irp, STATUS_BUFFER_ALL_ZEROS);

			
			//TODO:check existing process in the white list create for this a function
			auto newAllowedProcess = (Item<AllowedProcess> *) ExAllocatePool2(POOL_FLAG_PAGED, \
			sizeof(Item<AllowedProcess>), DRIVER_TAG);
			if (!newAllowedProcess)
				return CompleteIrp(Irp, STATUS_FAILED_DRIVER_ENTRY); //TODO: find normal return value

			auto& item = newAllowedProcess->data_;
			item.length_= data->length_;
			::memcpy(item.ImageFileName, data->ImageFileName, MaxSizeImageName);
			DbgPrint("%ws", item.ImageFileName);
			AddAllowedProcess(&newAllowedProcess->Entry);
		}break;

		case IOCTL_REQUEST_TYPE_REMOVE:
		{

		}break;

		case IOCTL_REQUEST_TYPE_CLEAR_ALL:
		{
			ClearAllProcesses();
		}break;
	}

	return CompleteIrp(Irp, STATUS_SUCCESS);
}
	