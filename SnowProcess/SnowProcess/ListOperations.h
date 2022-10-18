#pragma once
#include <ntddk.h>
#include "SnowProcessCommon.h"

//TODO: //consider moving it to another cpp file
void InitWhiteListProcesses();
void AddAllowedProcess(LIST_ENTRY* entry);
void ClearAllProcesses();
void DeleteProcess();