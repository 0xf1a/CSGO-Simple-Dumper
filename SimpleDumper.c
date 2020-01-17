#include <stdio.h>
#include <time.h>
#include "xLiteMem.h"
#include "SimpleDumper.h"

VOID DumpTime(PSTR pszOut)
{
	CHAR buffer[128] = { 0 };

	struct tm t1;
	time_t t2 = time(NULL);
	localtime_s(&t1, &t2);

	strftime(EXP(buffer), "// CSGO offsets and netvars\n// %d.%m.%Y. %H:%M:%S\n", &t1);

	snprintf(pszOut, SD_OUTPUT_FILESIZE, "%s\n%sdwEpochTime%s0x%llX%s\n\n", buffer, SD_PREFIX, SD_SEPARATOR, t2, SD_SUFFIX);
}
VOID DumpSpace(PSTR pszOut)
{
	snprintf(pszOut, SD_OUTPUT_FILESIZE, "%s\n", pszOut);
}

DWORD GetValueFromBuffer(PSTR pszIn, PCSTR pszName)
{
	if (pszIn != NULL)
	{
		PSTR p = strstr(pszIn, pszName);
		if (p != NULL)
		{
			return strtoul(p + strlen(pszName) + strlen(SD_SEPARATOR), NULL, 0);
		}
	}

	return 0;
}
DWORD DumpToStreams(PSTR pszIn, PSTR pszOut, PCSTR pszName, DWORD dwValue)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwPrevValue = GetValueFromBuffer(pszIn, pszName);

	if (dwPrevValue != dwValue && dwPrevValue)
	{
		SetConsoleTextAttribute(hConsole, YellowFore);
		printf("[!] %s: ", pszName);
		SetConsoleTextAttribute(hConsole, RedFore);
		printf("0x%X", dwPrevValue);
		SetConsoleTextAttribute(hConsole, YellowFore);
		printf(" -> ");
		SetConsoleTextAttribute(hConsole, LimeFore);
		printf("0x%X\n", dwValue);
	}
	else
	{
		SetConsoleTextAttribute(hConsole, YellowFore);
		printf("[+] %s: ", pszName);
		SetConsoleTextAttribute(hConsole, LimeFore);
		printf("0x%X\n", dwValue);
	}

	snprintf(pszOut, SD_OUTPUT_FILESIZE, "%s%s%s%s0x%X%s\n", pszOut, SD_PREFIX, pszName, SD_SEPARATOR, dwValue, SD_SUFFIX);
	SetConsoleTextAttribute(hConsole, GrayFore);

	return dwValue;
}
DWORD DumpOffset(PSTR pszIn, PSTR pszOut, MODULE32 Module, PCSTR pszName, PBYTE pbPattern, UINT uLength, UCHAR bWildcard, UINT uOffset, UINT uExtra, BOOLEAN bRelative, BOOLEAN bSubtract)
{
	DWORD dwOffset = FindPattern(Module.pbBuffer, Module.dwBase, Module.dwSize, pbPattern, uLength, bWildcard, uOffset, uExtra, bRelative, bSubtract);
	return DumpToStreams(pszIn, pszOut, pszName, dwOffset);
}
DWORD DumpNetvar(PSTR pszIn, PSTR pszOut, HANDLE hProcess, DWORD dwStart, PCSTR pszClassName, PCSTR pszVarName, PCSTR pszOutName, UINT uExtra)
{
	DWORD dwNetvar = FindNetvar(hProcess, dwStart, pszClassName, pszVarName) + uExtra;
	return DumpToStreams(pszIn, pszOut, pszOutName ? pszOutName : pszVarName, dwNetvar);
}

BOOL WriteToDumpFile(HANDLE hFile, PSTR pszOut)
{
	DWORD dwWritten = 0;

	if (pszOut && SetFilePointer(hFile, 0, 0, FILE_BEGIN) != INVALID_SET_FILE_POINTER && SetEndOfFile(hFile))
	{
		return WriteFile(hFile, pszOut, strlen(pszOut), &dwWritten, NULL);
	}

	return FALSE;
}
BOOL ReadDumpFile(HANDLE* hFile, PSTR* pszBuffer)
{
	DWORD dwRead = 0;

	*hFile = CreateFile(_T(SD_OUTPUT_FILENAME), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
	if (*hFile != INVALID_HANDLE_VALUE)
	{
		DWORD dwSize = GetFileSize(*hFile, NULL);
		if (dwSize && dwSize != INVALID_FILE_SIZE)
		{
			*pszBuffer = (PSTR)malloc(dwSize);
			if (*pszBuffer)
			{
				return ReadFile(*hFile, *pszBuffer, dwSize, &dwRead, NULL);
			}
		}
	}

	return FALSE;
}

VOID MainDumper(HANDLE hProcess, MODULE32 Client, MODULE32 Engine)
{
	HANDLE hFile = NULL;
	PSTR pszIn = NULL;
	ReadDumpFile(&hFile, &pszIn);

	CHAR szOut[SD_OUTPUT_FILESIZE] = { 0 };
	DumpTime(szOut);

	DWORD dwLocalPayer = DumpOffset(pszIn, szOut, Client, "dwLocalPayer", EXP("\x8D\x34\x85\xAA\xAA\xAA\xAA\x89\x15\xAA\xAA\xAA\xAA\x8B\x41\x08\x8B\x48\x04\x83\xF9\xFF"), 0xAA, 0x3, 0x4, TRUE, TRUE);
	DWORD dwEntityList = DumpOffset(pszIn, szOut, Client, "dwEntityList", EXP("\xBB\xAA\xAA\xAA\xAA\x83\xFF\x01\x0F\x8C\xAA\xAA\xAA\xAA\x3B\xF8"), 0xAA, 0x1, 0x0, TRUE, TRUE);
	DWORD dwClientState = DumpOffset(pszIn, szOut, Engine, "dwClientState", EXP("\xA1\xAA\xAA\xAA\xAA\x33\xD2\x6A\x00\x6A\x00\x33\xC9\x89\xB0"), 0xAA, 0x1, 0x0, TRUE, TRUE);
	DWORD dwPlayerResource = DumpOffset(pszIn, szOut, Client, "dwPlayerResource", EXP("\x8B\x3D\xAA\xAA\xAA\xAA\x85\xFF\x0F\x84\xAA\xAA\xAA\xAA\x81\xC7"), 0xAA, 0x2, 0x0, TRUE, TRUE);
	DWORD dwGlowObject = DumpOffset(pszIn, szOut, Client, "dwGlowObject", EXP("\x0F\x11\xAA\xAA\xAA\xAA\xAA\x83\xC8\x01\xC7\x05\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\x0F\x28\xAA\xAA\xAA\xAA\xAA\x68"), 0xAA, 0x3, 0x0, TRUE, TRUE);
	DWORD dwForceAttack = DumpOffset(pszIn, szOut, Client, "dwForceAttack", EXP("\x89\x0D\xAA\xAA\xAA\xAA\x8B\x0D\xAA\xAA\xAA\xAA\x8B\xF2\x8B\xC1\x83\xCE\x04"), 0xAA, 0x2, 0x0, TRUE, TRUE);
	DWORD dwForceJump = DumpOffset(pszIn, szOut, Client, "dwForceJump", EXP("\x8B\x0D\xAA\xAA\xAA\xAA\x8B\xD6\x8B\xC1\x83\xCA\x02"), 0xAA, 0x2, 0x0, TRUE, TRUE);
	DWORD dwForceAlt1 = DumpToStreams(pszIn, szOut, "dwForceAlt1", dwForceJump + 0x3C);
	DWORD dwSensitivity = DumpOffset(pszIn, szOut, Client, "dwSensitivity", EXP("\x6A\x01\x51\xC7\x04\x24\x17\xB7\xD1\x38\xB9"), 0xAA, 0xB, 0x2C, TRUE, TRUE);

	DumpSpace(szOut);

	DWORD m_dwPlayerInfo = DumpOffset(pszIn, szOut, Engine, "m_dwPlayerInfo", EXP("\x8B\x89\xAA\xAA\xAA\xAA\x85\xC9\x0F\x84\xAA\xAA\xAA\xAA\x8B\x01"), 0xAA, 0x2, 0x0, TRUE, FALSE);
	DWORD m_dwViewAngles = DumpOffset(pszIn, szOut, Engine, "m_dwViewAngles", EXP("\xF3\x0F\x11\x80\xAA\xAA\xAA\xAA\xD9\x46\x04\xD9\x05\xAA\xAA\xAA\xAA"), 0xAA, 0x4, 0x0, TRUE, FALSE);
	DWORD m_szMapPath = DumpOffset(pszIn, szOut, Engine, "m_szMapPath", EXP("\x05\xAA\xAA\xAA\xAA\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x80\x3D"), 0xAA, 0x1, 0x0, TRUE, FALSE);
	DWORD m_iLocalPayer = DumpOffset(pszIn, szOut, Engine, "m_iLocalPayer", EXP("\x8B\x80\xAA\xAA\xAA\xAA\x40\xC3"), 0xAA, 0x2, 0x0, TRUE, FALSE);
	DWORD m_dwInGame = DumpOffset(pszIn, szOut, Engine, "m_dwInGame", EXP("\x83\xB8\xAA\xAA\xAA\xAA\x06\x0F\x94\xC0\xC3"), 0xAA, 0x2, 0x0, TRUE, FALSE);

	DumpSpace(szOut);

	DWORD dwGetAllClasses = FindPattern(Client.pbBuffer, Client.dwBase, Client.dwSize, EXP("\x44\x54\x5F\x54\x45\x57\x6F\x72\x6C\x64\x44\x65\x63\x61\x6C"), 0xAA, 0x0, 0x0, FALSE, FALSE);
	dwGetAllClasses = FindPattern(Client.pbBuffer, Client.dwBase, Client.dwSize, (PBYTE)&dwGetAllClasses, sizeof(PBYTE), 0x0, 0x2B, 0x0, TRUE, FALSE);

	DWORD m_hViewModel = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BasePlayer", "m_hViewModel[0]", "m_hViewModel", 0);
	DWORD m_iViewModelIndex = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseCombatWeapon", "m_iViewModelIndex", NULL, 0);
	DWORD m_flFallbackWear = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAttributableItem", "m_flFallbackWear", NULL, 0);
	DWORD m_nFallbackPaintKit = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAttributableItem", "m_nFallbackPaintKit", NULL, 0);
	DWORD m_iItemIDHigh = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAttributableItem", "m_iItemIDHigh", NULL, 0);
	DWORD m_iEntityQuality = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAttributableItem", "m_iEntityQuality", NULL, 0);
	DWORD m_iItemDefinitionIndex = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAttributableItem", "m_iItemDefinitionIndex", NULL, 0);
	DWORD m_hActiveWeapon = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseCombatCharacter", "m_hActiveWeapon", NULL, 0);
	DWORD m_hMyWeapons = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseCombatCharacter", "m_hMyWeapons", NULL, 0);
	DWORD m_nModelIndex = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseViewModel", "m_nModelIndex", NULL, 0);

	DumpSpace(szOut);

	DWORD m_bHasDefuser = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayer", "m_bHasDefuser", NULL, 0);
	DWORD m_iCrossHairID = DumpToStreams(pszIn, szOut, "m_iCrossHairID", m_bHasDefuser + 0x5C);
	DWORD m_flFlashDuration = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayer", "m_flFlashDuration", NULL, 0);
	DWORD m_iGlowIndex = DumpToStreams(pszIn, szOut, "m_iGlowIndex", m_flFlashDuration + 0x18);
	DWORD m_iShotsFired = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayer", "m_iShotsFired", NULL, 0);
	DWORD m_bIsScoped = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayer", "m_bIsScoped", NULL, 0);
	DWORD m_vecPunch = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BasePlayer", "m_Local", "m_vecPunch", 0x70);
	DWORD m_dwBoneMatrix = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAnimating", "m_nForceBone", "m_dwBoneMatrix", 0x1C);
	DWORD m_iPlayerC4 = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayerResource", "m_iPlayerC4", NULL, 0);
	DWORD m_bSpotted = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseEntity", "m_bSpotted", NULL, 0);

	DumpSpace(szOut);

	DWORD m_lifeState = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayer", "m_lifeState", NULL, 0);
	DWORD m_vecOrigin = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseEntity", "m_vecOrigin", NULL, 0);
	DWORD m_angRotation = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseEntity", "m_angRotation", NULL, 0);
	DWORD m_vecVelocity = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BasePlayer", "m_vecVelocity[0]", "m_vecVelocity", 0);
	DWORD m_vecViewOffset = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BasePlayer", "m_vecViewOffset[0]", "m_vecViewOffset", 0);
	DWORD m_fFlags = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayer", "m_fFlags", NULL, 0);
	DWORD m_iHealth = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BasePlayer", "m_iHealth", NULL, 0);
	DWORD m_iTeamNum = DumpNetvar(pszIn, szOut, hProcess, dwGetAllClasses, "DT_BaseEntity", "m_iTeamNum", NULL, 0);

	DWORD m_bDormant = DumpOffset(pszIn, szOut, Client, "m_bDormant", EXP("\x55\x8B\xEC\x53\x8B\x5D\x08\x56\x8B\xF1\x88\x9E\xAA\xAA\xAA\xAA\xE8"), 0xAA, 0xC, 0x0, TRUE, FALSE);

	if (pszIn) { free(pszIn); }

	WriteToDumpFile(hFile, szOut);
	CloseHandle(hFile);
}

int main()
{
	MODULE32 Client = { 0 };
	MODULE32 Engine = { 0 };

	DWORD dwProcessId = 0;
	HANDLE hProcess = NULL;

	dwProcessId = GetProcessIdByProcessName(_T("csgo.exe"));
	printf("[+] csgo.exe process id: %u\n", dwProcessId);

	hProcess = OpenProcess(PROCESS_VM_READ, FALSE, dwProcessId);
	if (hProcess == INVALID_HANDLE_VALUE)
	{
		printf("[!] Error opening handle to CSGO!\n");
		return 1;
	}

	Client.dwBase = GetModuleBaseAddress(dwProcessId, _T("client_panorama.dll"));
	printf("[+] client_panorama.dll base: 0x%x\n", Client.dwBase);

	Client.dwSize = GetModuleSize(dwProcessId, _T("client_panorama.dll"));
	printf("[+] client_panorama.dll size: 0x%x\n", Client.dwSize);

	Client.pbBuffer = (PBYTE)malloc(Client.dwSize);
	if (!Client.pbBuffer || !ReadMemory(hProcess, Client.dwBase, Client.pbBuffer, Client.dwSize))
	{
		printf("[!] Error dumping client_panorama.dll!\n");
		goto cleanup;
	}

	Engine.dwBase = GetModuleBaseAddress(dwProcessId, _T("engine.dll"));
	printf("[+] engine.dll base: 0x%x\n", Engine.dwBase);

	Engine.dwSize = GetModuleSize(dwProcessId, _T("engine.dll"));
	printf("[+] engine.dll size: 0x%x\n", Engine.dwSize);

	Engine.pbBuffer = (PBYTE)malloc(Engine.dwSize);
	if (!Engine.pbBuffer || !ReadMemory(hProcess, Engine.dwBase, Engine.pbBuffer, Engine.dwSize))
	{
		printf("[!] Error dumping engine.dll!\n");
		goto cleanup;
	}

	printf("\n");

	MainDumper(hProcess, Client, Engine);

cleanup:
	if (Client.pbBuffer) { free(Client.pbBuffer); }
	if (Engine.pbBuffer) { free(Engine.pbBuffer); }
	CloseHandle(hProcess);

	getchar();
	return 0;
}