#include "process.h"
#include <objbase.h>
#include <strsafe.h>

WCHAR szAgentPath[MAX_PATH];

void WBoxSetAgent(LPWSTR szPath) {
	StringCchCopy(szAgentPath, MAX_PATH, szPath);
}

JobbedProcessManager::JobbedProcessManager() :
		szUsername(nullptr), szPassword(nullptr), szDirectory(nullptr), szCmdLine(nullptr) {
	ZeroMemory(&extLimits, sizeof extLimits);
	extLimits.BasicLimitInformation.ActiveProcessLimit = 1;
	extLimits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
	CoCreateGuid(&guid);
	StringFromGUID2(guid, szGuid, ARRAYSIZE(szGuid));
}

bool JobbedProcessManager::spawn() {
	HANDLE handle;
	WCHAR szName[MAX_PATH];
	StringCchCopy(szName, MAX_PATH, L"wbox_job_");
	StringCchCat(szName, MAX_PATH, szGuid);

	if (!(handle = CreateJobObject(nullptr, szName)))
		throw WindowsException("CreateJobObject");
	hJob = handle;
	wprintf(L"Job: %s\n", szName);

	if (!SetInformationJobObject(hJob, JobObjectExtendedLimitInformation, &extLimits, sizeof extLimits))
		throw WindowsException("SetInformationJobObject");

	STARTUPINFO si = {sizeof(STARTUPINFO), 0};
	PROCESS_INFORMATION pi;

	if (!szUsername)
		return false;

	size_t cchCmdLine = lstrlen(szCmdLine) + lstrlen(szAgentPath) + lstrlen(szGuid) + 5;
	LPWSTR szAgentCmdLine = (LPWSTR) malloc(cchCmdLine * sizeof(WCHAR));
	StringCchCopy(szAgentCmdLine, cchCmdLine, L"\"");
	StringCchCat(szAgentCmdLine, cchCmdLine, szAgentPath);
	StringCchCat(szAgentCmdLine, cchCmdLine, L"\" ");
	StringCchCat(szAgentCmdLine, cchCmdLine, szGuid);
	StringCchCat(szAgentCmdLine, cchCmdLine, L" ");
	StringCchCat(szAgentCmdLine, cchCmdLine, szCmdLine);

	if (!CreateProcessWithLogonW(szUsername, L".", szPassword, 0, szAgentPath, szAgentCmdLine,
								 NORMAL_PRIORITY_CLASS | CREATE_BREAKAWAY_FROM_JOB,
								 nullptr, szDirectory, &si, &pi))
		throw WindowsException("CreateProcessWithLogonW");

	union {
		DWORD dw;
		LONG l;
	} uExitCode;
	WaitForSingleObject(pi.hProcess, INFINITE);
	GetExitCodeProcess(pi.hProcess, &uExitCode.dw);

	if (uExitCode.l < 0) {
		printf("Agent: %d\n", uExitCode.l);
		return false;
	}

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	if (!(handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, uExitCode.dw)))
		throw WindowsException("OpenProcess");

	hProcess = handle;
	AssignProcessToJobObject(hJob, hProcess);

	return true;
}

bool JobbedProcessManager::terminate(unsigned code) {
	if (hProcess)
		if (!TerminateProcess(hProcess, code))
			throw WindowsException("TerminateProcess");
	return false;
}

JobbedProcessManager &JobbedProcessManager::time(double seconds) {
	if (seconds) {
		extLimits.BasicLimitInformation.PerJobUserTimeLimit.QuadPart = uint64_t(seconds * 1000 * 1000 * 10);
		extLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_TIME;
	} else
		extLimits.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_TIME;
	return *this;
}

JobbedProcessManager &JobbedProcessManager::memory(size_t bytes) {
	if (bytes) {
		extLimits.JobMemoryLimit = bytes;
		extLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_JOB_MEMORY;
	} else
		extLimits.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_JOB_MEMORY;
	return *this;
}

JobbedProcessManager &JobbedProcessManager::processes(int count) {
	if (count) {
		extLimits.BasicLimitInformation.ActiveProcessLimit = count;
		extLimits.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
	} else
		extLimits.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_ACTIVE_PROCESS;
	return *this;
}

JobbedProcessManager& JobbedProcessManager::withLogin(LPCWSTR szUsername, LPCWSTR szPassword) {
	this->szUsername = szUsername;
	this->szPassword = szPassword;
	return *this;
}

JobbedProcessManager& JobbedProcessManager::command(LPCWSTR szCmdLine) {
	if (this->szCmdLine)
		free(this->szCmdLine);
	size_t bytes = (lstrlen(szCmdLine) + 1) * sizeof(WCHAR);
	this->szCmdLine = (LPWSTR) malloc(bytes);
	StringCbCopy(this->szCmdLine, bytes, szCmdLine);
	return *this;
}

JobbedProcessManager& JobbedProcessManager::directory(LPCWSTR szDirectory) {
	this->szDirectory = szDirectory;
	return *this;
}

JobbedProcessManager::~JobbedProcessManager() {
}
