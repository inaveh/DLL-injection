#include <Windows.h>
#include <stdio.h>
#include <iostream>
#include <Tlhelp32.h>
#include <comdef.h>

#define LIBRARY "D:\\cyber\\bootcamp\\os\\DLL\\mydll\\mydll\\Debug\\mydll.dll"

using namespace std;

int main()
{
	DWORD err;
	WCHAR* fileExt;
	WCHAR szDir[256];

	// Get full path of DLL to inject
	DWORD pathLen = GetFullPathNameA(
		(LPSTR)LIBRARY,		// file name
		256, 				// size of path buffer
		(LPSTR)szDir,		// path buffer
		(LPSTR*)&fileExt	// address of file name in path
	);

	// Get LoadLibrary function address –
	// the address doesn't change at remote process
	PVOID addrLoadLibrary =
		(PVOID)GetProcAddress(GetModuleHandle(L"kernel32.dll"),
			"LoadLibraryA");

	// Get id of the notepad.exe
	int procID;
	HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, NULL);
	PROCESSENTRY32 pEntry;
	PROCESSENTRY32 pEntry1;
	pEntry.dwSize = sizeof(pEntry);
	BOOL hRes = Process32First(hSnapShot, &pEntry);
	while (hRes)
	{
		_bstr_t b(pEntry.szExeFile);
		if (strcmp(b, "notepad.exe") == 0)
		{
			pEntry1 = pEntry;
			procID = (DWORD)pEntry.th32ProcessID;
			cout <<procID;
			break;
		}
		hRes = Process32Next(hSnapShot, &pEntry);
	}
	//int procID = (DWORD)pEntry1.th32ProcessID;
	// Open remote process
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID);

	// Get a pointer to memory location in remote process, 
	// big enough to store DLL path
	PVOID memAddr = (PVOID)VirtualAllocEx(hProc, NULL, pathLen, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (NULL == memAddr) {
		err = GetLastError();
		return 0;
	}

	// Write DLL name to remote process memory
	BOOL check = WriteProcessMemory(hProc, memAddr, (void*)LIBRARY, pathLen, NULL);
	if (0 == check) {
		err = GetLastError();
		return 0;
	}

	// Open remote thread, while executing LoadLibrary 
	// with parameter DLL name, will trigger DLLMain
	HANDLE hRemote = CreateRemoteThread(hProc, NULL, 0,
		(LPTHREAD_START_ROUTINE)addrLoadLibrary, memAddr, NULL, NULL);
	if (NULL == hRemote) {
		err = GetLastError();
		return 0;
	}

	WaitForSingleObject(hRemote, INFINITE);
	check = CloseHandle(hRemote);
	Sleep(1000);
	return 0;
}
