#include "DllInjector.h"
#include "stdafx.h"
#include <iostream>
#include <string>
#include <windows.h>
#include <tlhelp32.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <psapi.h>
#include "windowsutils.h"

DllInjector::DllInjector()
{
	injectionParameters = new InjectionParameters();


}


DllInjector::~DllInjector()
{
}

InjectionParameters * DllInjector::GetInjectionParameters()
{
	return injectionParameters;
}
static HANDLE hMapObject;
static void* vMapData;
void DllInjector::SetInjectionParameters(InjectionParameters ip)
{
	if (!hMapObject) {
		hMapObject = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(InjectionParameters), L"BakkesModInjectionParameters");
		if (hMapObject == NULL) {
			return;
		}

		vMapData = MapViewOfFile(hMapObject, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(InjectionParameters));
		if (vMapData == NULL) {
			CloseHandle(hMapObject);
			return;
		}
		//vMapData = injectionParameters;
		//UnmapViewOfFile(vMapData);
	}
	memcpy(vMapData, &ip, sizeof(InjectionParameters));
}


DWORD DllInjector::InjectDLL(std::wstring processName, std::filesystem::path path)
{
	DWORD processID = GetProcessID64(processName);
	if (processID == 0)
		return NOT_RUNNING;

	HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, false, processID);
	if (h)
	{
		LPVOID LoadLibAddr = (LPVOID)GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW");
		auto ws = path.wstring().c_str();
		auto wslen = (std::wcslen(ws) + 1) * sizeof(WCHAR);
		LPVOID dereercomp = VirtualAllocEx(h, NULL, wslen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
		WriteProcessMemory(h, dereercomp, ws, wslen, NULL);
		HANDLE asdc = CreateRemoteThread(h, NULL, NULL, (LPTHREAD_START_ROUTINE)LoadLibAddr, dereercomp, 0, NULL);
		WaitForSingleObject(asdc, INFINITE);
		DWORD res = 0;
		GetExitCodeThread(asdc, &res);
		LOG_LINE(INFO, "GetExitCodeThread(): " << (int)res);
			LOG_LINE(INFO, "Last error: " << GetLastError());
		VirtualFreeEx(h, dereercomp, wslen, MEM_RELEASE);
		CloseHandle(asdc);
		CloseHandle(h);
		return res != 0 ? OK : NOPE;
	}
	return NOPE;
}

DWORD DllInjector::GetProcessID64(std::wstring processName)
{
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE)
		return 0;

	Process32First(processesSnapshot, &processInfo);
	if (_wcsicmp(processName.c_str(), processInfo.szExeFile) == 0)
	{
		
		BOOL iswow64 = FALSE;
		//https://stackoverflow.com/questions/14184137/how-can-i-determine-whether-a-process-is-32-or-64-bit
		//If IsWow64Process() reports true, the process is 32-bit running on a 64-bit OS
		//So we want it to return false (32 bit on 32 bit os, or 64 bit on 64 bit OS, since we build x64 the first condition will never satisfy since they can't run this exe)
		
		auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processInfo.th32ProcessID);
		if (hProcess == NULL)
		{
			LOG_LINE(INFO, "Error on OpenProcess to check bitness");
		}
		else
		{

			if (IsWow64Process(hProcess, &iswow64))
			{
				//LOG_LINE(INFO, "Rocket league process ID is " << processInfo.th32ProcessID << " | " << " has the WOW factor: " << iswow64);
				if (!iswow64)
				{
					CloseHandle(hProcess);
					CloseHandle(processesSnapshot);
					return processInfo.th32ProcessID;
				}
			}
			else
			{
				LOG_LINE(INFO, "IsWow64Process failed bruv " << GetLastError());
			}
			CloseHandle(hProcess);
		}
	}

	while (Process32Next(processesSnapshot, &processInfo))
	{
		if (_wcsicmp(processName.c_str(), processInfo.szExeFile) == 0)
		{
			BOOL iswow64 = FALSE;
			auto hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processInfo.th32ProcessID);
			if (hProcess == NULL)
			{
				LOG_LINE(INFO, "Error on OpenProcess to check bitness");
			}
			else
			{

				if (IsWow64Process(hProcess, &iswow64))
				{
					//LOG_LINE(INFO, "Rocket league process ID is " << processInfo.th32ProcessID << " | " << " has the WOW factor: " << iswow64);
					if (!iswow64)
					{
						CloseHandle(hProcess);
						CloseHandle(processesSnapshot);
						return processInfo.th32ProcessID;
					}
				}
				else
				{
					LOG_LINE(INFO, "IsWow64Process failed bruv " << GetLastError());
				}
				CloseHandle(hProcess);
			}
		}
		//CloseHandle(processesSnapshot);
	}

	CloseHandle(processesSnapshot);
	return 0;

}

std::vector<DWORD> DllInjector::GetProcessIDS(std::wstring processName)
{
	std::vector<DWORD> ids;
	PROCESSENTRY32 processInfo;
	processInfo.dwSize = sizeof(processInfo);

	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (processesSnapshot == INVALID_HANDLE_VALUE)
		return { };

	Process32First(processesSnapshot, &processInfo);
	if (_wcsicmp(processName.c_str(), processInfo.szExeFile) == 0)
	{

		BOOL iswow64 = FALSE;
		//https://stackoverflow.com/questions/14184137/how-can-i-determine-whether-a-process-is-32-or-64-bit
		//If IsWow64Process() reports true, the process is 32-bit running on a 64-bit OS
		//So we want it to return false (32 bit on 32 bit os, or 64 bit on 64 bit OS, since we build x64 the first condition will never satisfy since they can't run this exe)

		ids.push_back(processInfo.th32ProcessID);
	}

	while (Process32Next(processesSnapshot, &processInfo))
	{
		if (_wcsicmp(processName.c_str(), processInfo.szExeFile) == 0)
		{
			ids.push_back(processInfo.th32ProcessID);
		}
	}

	CloseHandle(processesSnapshot);
	return ids;
}

DWORD DllInjector::IsBakkesModDllInjected(std::wstring processName)
{
	HMODULE hMods[1024];
	HANDLE hProcess;
	DWORD cbNeeded;
	unsigned int i;
	DWORD processID = GetProcessID64(processName);
	if (processID == 0)
		return NOT_RUNNING;

	printf("\nProcess ID: %u\n", processID);

	// Get a handle to the process.

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, processID);
	if (NULL == hProcess)
		return 1;

	// Get a list of all the modules in this process.

	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
	{
		for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			TCHAR szModName[MAX_PATH];

			// Get the full path to the module's file.

			if (GetModuleFileNameEx(hProcess, hMods[i], szModName,
				sizeof(szModName) / sizeof(TCHAR)))
			{
				std::wstring dllName = std::wstring(szModName);
				if (dllName.find(L"bakkesmod.dll") != std::string::npos)
				{
					CloseHandle(hProcess);
					return OK;
				}
			}
		}
	}

	// Release the handle to the process.

	CloseHandle(hProcess);
	return NOPE;
}

GamePlatform DllInjector::GetPlatform(DWORD processID)
{
	HMODULE hMods[1024];
	HANDLE hProcess;
	DWORD cbNeeded;
	unsigned int i;

	// Get a handle to the process.

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, processID);
	if (NULL == hProcess)
		return GamePlatform::UNKNOWN;

	// Get a list of all the modules in this process.

	if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded))
	{
		for (i = 0; i < (cbNeeded / sizeof(HMODULE)); i++)
		{
			TCHAR szModName[MAX_PATH];

			// Get the full path to the module's file.

			if (GetModuleFileNameEx(hProcess, hMods[i], szModName,
				sizeof(szModName) / sizeof(TCHAR)))
			{
				// Print the module name and handle value.
				auto name = std::wstring(szModName);
				if (name.find(L"steam_api64.dll") != std::string::npos || name.find(L"steamapps") != std::string::npos)
				{
					LOG_LINE(INFO, WindowsUtils::WStringToString(std::wstring(szModName)).c_str() << " || " << hMods[i]);
					CloseHandle(hProcess);
					return GamePlatform::STEAM;
				}
				
			}
		}
	}

	// Release the handle to the process.

	CloseHandle(hProcess);

	return GamePlatform::EPIC;
}
