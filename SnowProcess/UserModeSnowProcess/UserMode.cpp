#include <iostream>
#include <Windows.h>
#include "..\SnowProcess\SnowProcessCommon.h"
#include <wchar.h>
#include <string>

void AddToList(std::wstring name)
{
	HANDLE hDevice = CreateFile(L"\\\\.\\SnowProcess", GENERIC_WRITE,
		FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		std::cout << GetLastError() << std::endl;
		//return 1;
	}

	std::cout << IOCTL_REQUEST_TYPE_ADD << std::endl;
	AllowedProcess sampleProcess;
	sampleProcess.length_ = 11;
	wcscpy_s(sampleProcess.ImageFileName, name.c_str());

	DWORD returned;
	BOOL success = DeviceIoControl(hDevice, IOCTL_REQUEST_TYPE_ADD, &sampleProcess,
		sizeof(AllowedProcess), nullptr, 0, &returned, nullptr);
	if (success)
		std::cout << "IO worked well!" << std::endl;
	else
	{
		std::cout << "Priority change failed!" << std::endl;
		std::cout << GetLastError() << std::endl;
	}

	CloseHandle(hDevice);
}

int main()
{
	AddToList(L"alex.exe");
	AddToList(L"bob.exe");
	AddToList(L"alon.exe");
	return 0;
}