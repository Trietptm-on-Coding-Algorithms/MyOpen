// MyTestAntiDebuger.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <Windows.h>
#include <Shlwapi.h>
#include <TlHelp32.h>
#include "DynamicMapping.h"


bool startInjectionProcess(HANDLE hProcess, BYTE * dllMemory)
{
    LPVOID remoteImageBase = MapModuleToProcess(hProcess, dllMemory, true);
    if (remoteImageBase)
    {
        /*
        FillHookDllData(hProcess, &g_hdd);
        //DWORD initDllFuncAddressRva = GetDllFunctionAddressRVA(dllMemory, "InitDll");
        DWORD hookDllDataAddressRva = GetDllFunctionAddressRVA(dllMemory, "HookDllData");

        if (StartHooking(hProcess, dllMemory, (DWORD_PTR)remoteImageBase))
        {
            if (WriteProcessMemory(hProcess, (LPVOID)((DWORD_PTR)hookDllDataAddressRva + (DWORD_PTR)remoteImageBase), &g_hdd, sizeof(HOOK_DLL_DATA), 0))
            {
                //DWORD exitCode = StartDllInitFunction(hProcess, ((DWORD_PTR)initDllFuncAddressRva + (DWORD_PTR)remoteImageBase), remoteImageBase);


                //if (exitCode == HOOK_ERROR_SUCCESS)

                //{
                wprintf(L"Injection successful, Imagebase %p\n", remoteImageBase);
                //}
                //else
                //{
                //	wprintf(L"Injection failed, exit code %d 0x%X Imagebase %p\n", exitCode, exitCode, remoteImageBase);
                //}

                return true;
            }
            else
            {
                wprintf(L"Failed to write hook dll data\n");
            }
        }*/
    }

    return false;
}

BYTE * ReadFileToMemory(const WCHAR * targetFilePath)
{
    HANDLE hFile;
    DWORD dwBytesRead;
    DWORD FileSize;
    BYTE* FilePtr = 0;

    hFile = CreateFileW(targetFilePath, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        FileSize = GetFileSize(hFile, NULL);
        if (FileSize > 0)
        {
            FilePtr = (BYTE*)calloc(FileSize + 1, 1);
            if (FilePtr)
            {
                if (!ReadFile(hFile, (LPVOID)FilePtr, FileSize, &dwBytesRead, NULL))
                {
                    free(FilePtr);
                    FilePtr = 0;
                }

            }
        }
        CloseHandle(hFile);
    }

    return FilePtr;
}

bool startInjection(DWORD targetPid, const WCHAR * dllPath)
{
    bool result = false;

    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, 0, targetPid);
    if (hProcess)
    {
        BYTE * dllMemory = ReadFileToMemory(dllPath);
        if (dllMemory)
        {
            result = startInjectionProcess(hProcess, dllMemory);
            free(dllMemory);
        }
        else
        {
            wprintf(L"Cannot read file to memory %s\n", dllPath);
        }
        CloseHandle(hProcess);
    }
    else
    {
        wprintf(L"Cannot open process handle %d\n", targetPid);
    }

    return result;
}

DWORD GetProcessIdByName(const WCHAR * processName)
{
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        return 0;
    }

    PROCESSENTRY32 pe32;
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32))
    {
        wprintf(L"Error getting first process\n");
        CloseHandle(hProcessSnap);
        return 0;
    }

    DWORD pid = 0;

    do
    {
        if (!_wcsicmp(pe32.szExeFile, processName))
        {
            pid = pe32.th32ProcessID;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return pid;
}

DWORD_PTR GetAddressOfEntryPoint(BYTE * dllMemory)
{
    PIMAGE_DOS_HEADER pDos = (PIMAGE_DOS_HEADER)dllMemory;
    PIMAGE_NT_HEADERS pNt = (PIMAGE_NT_HEADERS)((DWORD_PTR)pDos + pDos->e_lfanew);
    return pNt->OptionalHeader.AddressOfEntryPoint;
}

// LPVOID StealthDllInjection(HANDLE hProcess, const WCHAR * dllPath, BYTE * dllMemory)
// LPVOID StealthDllInjection(HANDLE hProcess, const WCHAR * dllPath)
LPVOID StealthDllInjection(DWORD targetPid, const WCHAR * dllPath)
{
    LPVOID remoteImageBaseOfInjectedDll = 0;
    BYTE * dllMemory = ReadFileToMemory(dllPath);
    HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, 0, targetPid);
    if (!hProcess)
    {
        return 0;
    }

    if (dllMemory)
    {
        remoteImageBaseOfInjectedDll = MapModuleToProcess(hProcess, dllMemory, false);
        if (remoteImageBaseOfInjectedDll)
        {

            DWORD_PTR entryPoint = GetAddressOfEntryPoint(dllMemory);

            if (entryPoint)
            {
                DWORD_PTR dllMain = entryPoint + (DWORD_PTR)remoteImageBaseOfInjectedDll;

//                 g_log.LogInfo(L"DLL INJECTION: Starting thread at RVA %p VA %p!", entryPoint, dllMain);

                HANDLE hThread = CreateRemoteThread(hProcess, NULL, NULL, (LPTHREAD_START_ROUTINE)dllMain, remoteImageBaseOfInjectedDll, CREATE_SUSPENDED, 0);
                if (hThread)
                {
//                     DoThreadMagic(hThread);
                    ResumeThread(hThread);

                    CloseHandle(hThread);
                }
                else
                {
//                     g_log.LogInfo(L"DLL INJECTION: Failed to start thread %d!", GetLastError());
                }
            }
        }
        else
        {
//             g_log.LogInfo(L"DLL INJECTION: Failed to map image of %s!", dllPath);
        }
    }

    return remoteImageBaseOfInjectedDll;
}

int _tmain(int argc, wchar_t* argv[])
{
    /*
    printf("enter main.\n");
    if (argc <= 2)
    {
        return 0;
    }

    DWORD targetPid = GetProcessIdByName(argv[1]);
    if (targetPid == 0)
    {
        return 0;
    }
//     startInjection(targetPid, argv[2]);
    LPCTSTR dllPath = argv[2];
    StealthDllInjection(targetPid, dllPath);
    printf("leave main.\n");
    return 0;*/

    while (true)
    {
        BOOL result = IsDebuggerPresent();
        printf("result = %d.\n", result);

        OutputDebugString(_T("2019-2-16\n"));

        Sleep(1 * 1000);
    }
    return 0;
}

