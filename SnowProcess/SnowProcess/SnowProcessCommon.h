//important parameter is the type of the method we are using, 
//
// neither means IO manager doesnt help with buffer managment
//meaning the driver dealing with the buffer by its own
//
//bufferd - For this transfer type, IRPs supply a pointer to a buffer at
//Irp->AssociatedIrp.SystemBuffer. This buffer represents both the input buffer and 
//the output buffer that are specified in calls to DeviceIoControl 
// and IoBuildDeviceIoControlRequest. The driver transfers data out of, and then into, this buffer.
#define SNOW_PROCESS_DEVICE 0x8000
#define IOCTL_REQUEST_TYPE_ADD CTL_CODE(SNOW_PROCESS_DEVICE, \
							0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_REQUEST_TYPE_REMOVE CTL_CODE(SNOW_PROCESS_DEVICE, \
							0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_REQUEST_TYPE_CLEAR_ALL CTL_CODE(SNOW_PROCESS_DEVICE, \
							0x802, METHOD_NEITHER, FILE_ANY_ACCESS)

#pragma once
#include "FastMutex.h"


const int MaxSizeImageName = 300 + 1;
struct AllowedProcess
{
	WCHAR ImageFileName[MaxSizeImageName];
	int length_;
};


struct SnowProcesses
{
	LIST_ENTRY head_;
	int numOfElements_;
	FastMutex mutex_;
};

//global 
extern SnowProcesses WhiteProcesses;

template <typename T>
struct Item
{
	LIST_ENTRY Entry;
	T data_;
};

