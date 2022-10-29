//important parameter is the type of the method we are using, 
//
// neither means IO manager doesnt help with buffer managment
//meaning the driver dealing with the buffer by its own
//
//bufferd - For this transfer type, IRPs supply a pointer to a buffer at
//Irp->AssociatedIrp.SystemBuffer. This buffer represents both the input buffer and 
//the output buffer that are specified in calls to DeviceIoControl 
// and IoBuildDeviceIoControlRequest. The driver transfers data out of, and then into, this buffer.

#pragma once
#define SNOW_PROCESS_DEVICE 0x8000
#define IOCTL_REQUEST_TYPE_ADD CTL_CODE(SNOW_PROCESS_DEVICE, \
							0x800, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_REQUEST_TYPE_REMOVE CTL_CODE(SNOW_PROCESS_DEVICE, \
							0x801, METHOD_NEITHER, FILE_ANY_ACCESS)
#define IOCTL_REQUEST_TYPE_CLEAR_ALL CTL_CODE(SNOW_PROCESS_DEVICE, \
							0x802, METHOD_NEITHER, FILE_ANY_ACCESS)




const int MaxSizeImageName = 300;
struct AllowedProcess
{
	WCHAR ImageFileName[MaxSizeImageName];
	int length_;
};
