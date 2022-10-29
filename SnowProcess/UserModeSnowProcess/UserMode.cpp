/*
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>

void DisplayError(LPTSTR lpszFunction);

int __cdecl main(int argc, TCHAR* argv[])
{
    HANDLE hFile;
    char DataBuffer[] = "This is some test data to write to the file.";
    DWORD dwBytesToWrite = (DWORD)strlen(DataBuffer);
    DWORD dwBytesWritten = 0;
    BOOL bErrorFlag = FALSE;

    printf("\n");
    if (argc != 2)
    {
        printf("Usage Error:\tIncorrect number of arguments\n\n");
        _tprintf(TEXT("%s <file_name>\n"), argv[0]);
        return 1;
    }

    hFile = CreateFile(argv[1],                // name of the write
        GENERIC_WRITE,          // open for writing
        0,                      // do not share
        NULL,                   // default security
        CREATE_NEW,             // create new file only
        FILE_ATTRIBUTE_NORMAL,  // normal file
        NULL);                  // no attr. template

    if (hFile == INVALID_HANDLE_VALUE)
    {
        //DisplayError("CreateFile");
        //_tprintf("Terminal failure: Unable to open file \"%s\" for write.\n"), argv[1]);
        return 1;
    }

    _tprintf(TEXT("Writing %d bytes to %s.\n"), dwBytesToWrite, argv[1]);

    bErrorFlag = WriteFile(
        hFile,           // open file handle
        DataBuffer,      // start of data to write
        dwBytesToWrite,  // number of bytes to write
        &dwBytesWritten, // number of bytes that were written
        NULL);            // no overlapped structure

    if (FALSE == bErrorFlag)
    {
        //DisplayError(TEXT("WriteFile"));
        printf("Terminal failure: Unable to write to file.\n");
    }
    else
    {
        if (dwBytesWritten != dwBytesToWrite)
        {
            // This is an error because a synchronous write that results in
            // success (WriteFile returns TRUE) should write all data as
            // requested. This would not necessarily be the case for
            // asynchronous writes.
            printf("Error: dwBytesWritten != dwBytesToWrite\n");
        }
        else
        {
            _tprintf(TEXT("Wrote %d bytes to %s successfully.\n"), dwBytesWritten, argv[1]);
        }
    }

    CloseHandle(hFile);
}

void DisplayError(LPTSTR lpszFunction)
// Routine Description:
// Retrieve and output the system error message for the last-error code
{
    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    DWORD dw = GetLastError();

    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0,
        NULL);

    lpDisplayBuf =
        (LPVOID)LocalAlloc(LMEM_ZEROINIT,
            (lstrlen((LPCTSTR)lpMsgBuf)
                + lstrlen((LPCTSTR)lpszFunction)
                + 40) // account for format string
            * sizeof(TCHAR));

    if (FAILED(StringCchPrintf((LPTSTR)lpDisplayBuf,
        LocalSize(lpDisplayBuf) / sizeof(TCHAR),
        TEXT("%s failed with error code %d as follows:\n%s"),
        lpszFunction,
        dw,
        lpMsgBuf)))
    {
        printf("FATAL ERROR: Unable to output error code.\n");
    }

    _tprintf(TEXT("ERROR: %s\n"), (LPCTSTR)lpDisplayBuf);

    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}
*/


#include <iostream>
#include <Windows.h>
#include "..\SnowProcess\SnowProcessCommon.h"
#include <wchar.h>
#include <string>
#include <fstream>
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
    //opening file and writing to it
    std::ofstream myfile;
    myfile.open("example.txt");
    myfile << "Hi there\n";
    myfile.close();
    return 0;
}