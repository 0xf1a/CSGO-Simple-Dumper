#include <stdio.h>
#include <time.h>
#include "xLiteMem.h"
#include "SimpleDumper.h"

VOID DumpTime(PSTR lpOut)
{
	char buffer[128];
	struct tm t1;
	time_t t2 = time(NULL);
	localtime_s(&t1, &t2);
	strftime(buffer, sizeof(buffer) - 1, "// CSGO offsets and netvars\n// %d.%m.%Y. %H:%M:%S\n", &t1);
	snprintf(lpOut, SD_OUTPUT_FILESIZE, "%s\n%sdwEpochTime%s0x%llX%s\n\n", buffer, SD_PREFIX, SD_SEPARATOR, t2, SD_SUFFIX);
}
VOID DumpSpace(PSTR lpOut)
{
	snprintf(lpOut, SD_OUTPUT_FILESIZE, "%s\n", lpOut);
}

DWORD GetValueFromBuffer(PSTR lpIn, LPCSTR lpName)
{
	if (lpIn != NULL)
	{
		PSTR p = strstr(lpIn, lpName);
		if (p != NULL)
		{
			return strtoul(p + strlen(lpName) + strlen(SD_SEPARATOR), NULL, 0);
		}
	}

	return 0;
}
DWORD DumpToStreams(PSTR lpIn, PSTR lpOut, LPCSTR lpName, DWORD dwValue)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwPrevValue = GetValueFromBuffer(lpIn, lpName);

	if (dwPrevValue != dwValue && dwPrevValue)
	{
		SetConsoleTextAttribute(hConsole, YellowFore);
		printf("[!] %s: ", lpName);
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
		printf("[+] %s: ", lpName);
		SetConsoleTextAttribute(hConsole, LimeFore);
		printf("0x%X\n", dwValue);
	}

	snprintf(lpOut, SD_OUTPUT_FILESIZE, "%s%s%s%s0x%X%s\n", lpOut, SD_PREFIX, lpName, SD_SEPARATOR, dwValue, SD_SUFFIX);
	SetConsoleTextAttribute(hConsole, GrayFore);

	return dwValue;
}
DWORD DumpOffset(PSTR lpIn, PSTR lpOut, MODULE32 Module, LPCSTR lpName, PBYTE pbPattern, UINT uLength, UCHAR bWildcard, UINT uOffset, UINT uExtra, BOOLEAN bRelative, BOOLEAN bSubtract)
{
	DWORD dwOffset = FindPattern(Module.pbBuffer, Module.dwBase, Module.dwSize, pbPattern, uLength, bWildcard, uOffset, uExtra, bRelative, bSubtract);
	return DumpToStreams(lpIn, lpOut, lpName, dwOffset);
}
DWORD DumpNetvar(PSTR lpIn, PSTR lpOut, HANDLE hProcess, DWORD dwStart, LPCSTR lpClassName, LPCSTR lpVarName, LPCSTR lpOutName, UINT uExtra)
{
	DWORD dwNetvar = FindNetvar(hProcess, dwStart, lpClassName, lpVarName) + uExtra;
	return DumpToStreams(lpIn, lpOut, lpOutName ? lpOutName : lpVarName, dwNetvar);
}

BOOL WriteToDumpFile(HANDLE hFile, PSTR lpOut)
{
	if (!lpOut) { return FALSE; }

	DWORD dwWritten = 0;

	SetFilePointer(hFile, 0, 0, FILE_BEGIN);

	return WriteFile(hFile, lpOut, strlen(lpOut), &dwWritten, NULL);
}
BOOL ReadDumpFile(HANDLE* hFile, PSTR* lpBuffer)
{
	DWORD dwRead = 0;

	*hFile = CreateFile(_T(SD_OUTPUT_FILENAME), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
	if (*hFile == INVALID_HANDLE_VALUE) { return FALSE; }

	DWORD dwSize = GetFileSize(*hFile, NULL);
	if (dwSize == INVALID_FILE_SIZE) { return FALSE; }

	*lpBuffer = (PCHAR)malloc(dwSize);
	if (!*lpBuffer) { return FALSE; }

	return ReadFile(*hFile, *lpBuffer, dwSize, &dwRead, NULL);
}

VOID MainDumper(HANDLE hProcess, MODULE32 Client, MODULE32 Engine)
{
	HANDLE hFile = NULL;
	PSTR lpIn = NULL;
	ReadDumpFile(&hFile, &lpIn);

	char szOut[SD_OUTPUT_FILESIZE] = { 0 };
	DumpTime(szOut);

	BYTE bLocalPlayer[23] = "\x8D\x34\x85\xAA\xAA\xAA\xAA\x89\x15\xAA\xAA\xAA\xAA\x8B\x41\x08\x8B\x48\x04\x83\xF9\xFF";
	DWORD dwLocalPlayer = DumpOffset(lpIn, szOut, Client, "dwLocalPlayer", bLocalPlayer, sizeof(bLocalPlayer) - 1, 0xAA, 0x3, 0x4, TRUE, TRUE);

	BYTE bEntityList[17] = "\xBB\xAA\xAA\xAA\xAA\x83\xFF\x01\x0F\x8C\xAA\xAA\xAA\xAA\x3B\xF8";
	DWORD dwEntityList = DumpOffset(lpIn, szOut, Client, "dwEntityList", bEntityList, sizeof(bEntityList) - 1, 0xAA, 0x1, 0x0, TRUE, TRUE);

	BYTE bClientState[16] = "\xA1\xAA\xAA\xAA\xAA\x33\xD2\x6A\x00\x6A\x00\x33\xC9\x89\xB0";
	DWORD dwClientState = DumpOffset(lpIn, szOut, Engine, "dwClientState", bClientState, sizeof(bClientState) - 1, 0xAA, 0x1, 0x0, TRUE, TRUE);

	BYTE bPlayerResource[17] = "\x8B\x3D\xAA\xAA\xAA\xAA\x85\xFF\x0F\x84\xAA\xAA\xAA\xAA\x81\xC7";
	DWORD dwPlayerResource = DumpOffset(lpIn, szOut, Client, "dwPlayerResource", bPlayerResource, sizeof(bPlayerResource) - 1, 0xAA, 0x2, 0x0, TRUE, TRUE);

	BYTE bGlowObject[29] = "\x0F\x11\xAA\xAA\xAA\xAA\xAA\x83\xC8\x01\xC7\x05\xAA\xAA\xAA\xAA\xAA\xAA\xAA\xAA\x0F\x28\xAA\xAA\xAA\xAA\xAA\x68";
	DWORD dwGlowObject = DumpOffset(lpIn, szOut, Client, "dwGlowObject", bGlowObject, sizeof(bGlowObject) - 1, 0xAA, 0x3, 0x0, TRUE, TRUE);

	BYTE bForceAttack[20] = "\x89\x0D\xAA\xAA\xAA\xAA\x8B\x0D\xAA\xAA\xAA\xAA\x8B\xF2\x8B\xC1\x83\xCE\x04";
	DWORD dwForceAttack = DumpOffset(lpIn, szOut, Client, "dwForceAttack", bForceAttack, sizeof(bForceAttack) - 1, 0xAA, 0x2, 0x0, TRUE, TRUE);

	BYTE bForceJump[14] = "\x8B\x0D\xAA\xAA\xAA\xAA\x8B\xD6\x8B\xC1\x83\xCA\x02";
	DWORD dwForceJump = DumpOffset(lpIn, szOut, Client, "dwForceJump", bForceJump, sizeof(bForceJump) - 1, 0xAA, 0x2, 0x0, TRUE, TRUE);

	DWORD dwForceAlt1 = DumpToStreams(lpIn, szOut, "dwForceAlt1", dwForceJump + 0x3C);

	BYTE bSensitivity[12] = "\x6A\x01\x51\xC7\x04\x24\x17\xB7\xD1\x38\xB9";
	DWORD dwSensitivity = DumpOffset(lpIn, szOut, Client, "dwSensitivity", bSensitivity, sizeof(bSensitivity) - 1, 0xAA, 0xB, 0x2C, TRUE, TRUE);

	DumpSpace(szOut);

	BYTE bPlayerInfo[17] = "\x8B\x89\xAA\xAA\xAA\xAA\x85\xC9\x0F\x84\xAA\xAA\xAA\xAA\x8B\x01";
	DWORD m_dwPlayerInfo = DumpOffset(lpIn, szOut, Engine, "m_dwPlayerInfo", bPlayerInfo, sizeof(bPlayerInfo) - 1, 0xAA, 0x2, 0x0, TRUE, FALSE);

	BYTE bViewAngles[18] = "\xF3\x0F\x11\x80\xAA\xAA\xAA\xAA\xD9\x46\x04\xD9\x05\xAA\xAA\xAA\xAA";
	DWORD m_dwViewAngles = DumpOffset(lpIn, szOut, Engine, "m_dwViewAngles", bViewAngles, sizeof(bViewAngles) - 1, 0xAA, 0x4, 0x0, TRUE, FALSE);

	BYTE bMapPath[16] = "\x05\xAA\xAA\xAA\xAA\xC3\xCC\xCC\xCC\xCC\xCC\xCC\xCC\x80\x3D";
	DWORD m_szMapPath = DumpOffset(lpIn, szOut, Engine, "m_szMapPath", bMapPath, sizeof(bMapPath) - 1, 0xAA, 0x1, 0x0, TRUE, FALSE);

	BYTE bGetLocalPlayer[9] = "\x8B\x80\xAA\xAA\xAA\xAA\x40\xC3";
	DWORD m_iLocalPlayer = DumpOffset(lpIn, szOut, Engine, "m_iLocalPlayer", bGetLocalPlayer, sizeof(bGetLocalPlayer) - 1, 0xAA, 0x2, 0x0, TRUE, FALSE);

	BYTE bInGame[12] = "\x83\xB8\xAA\xAA\xAA\xAA\x06\x0F\x94\xC0\xC3";
	DWORD m_dwInGame = DumpOffset(lpIn, szOut, Engine, "m_dwInGame", bInGame, sizeof(bInGame) - 1, 0xAA, 0x2, 0x0, TRUE, FALSE);

	DumpSpace(szOut);

	BYTE bGetAllClasses[16] = "\x44\x54\x5F\x54\x45\x57\x6F\x72\x6C\x64\x44\x65\x63\x61\x6C";
	DWORD dwGetAllClasses = FindPattern(Client.pbBuffer, Client.dwBase, Client.dwSize, bGetAllClasses, sizeof(bGetAllClasses) - 1, 0xAA, 0x0, 0x0, FALSE, FALSE);
	dwGetAllClasses = FindPattern(Client.pbBuffer, Client.dwBase, Client.dwSize, (PBYTE)&dwGetAllClasses, sizeof(PBYTE), 0x0, 0x2B, 0x0, TRUE, FALSE);

	DWORD m_hViewModel = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BasePlayer", "m_hViewModel[0]", "m_hViewModel", 0);
	DWORD m_iViewModelIndex = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseCombatWeapon", "m_iViewModelIndex", NULL, 0);
	DWORD m_flFallbackWear = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAttributableItem", "m_flFallbackWear", NULL, 0);
	DWORD m_nFallbackPaintKit = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAttributableItem", "m_nFallbackPaintKit", NULL, 0);
	DWORD m_iItemIDHigh = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAttributableItem", "m_iItemIDHigh", NULL, 0);
	DWORD m_iEntityQuality = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAttributableItem", "m_iEntityQuality", NULL, 0);
	DWORD m_iItemDefinitionIndex = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAttributableItem", "m_iItemDefinitionIndex", NULL, 0);
	DWORD m_hActiveWeapon = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseCombatCharacter", "m_hActiveWeapon", NULL, 0);
	DWORD m_hMyWeapons = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseCombatCharacter", "m_hMyWeapons", NULL, 0);
	DWORD m_nModelIndex = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseViewModel", "m_nModelIndex", NULL, 0);

	DumpSpace(szOut);

	DWORD m_bHasDefuser = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayer", "m_bHasDefuser", NULL, 0);
	DWORD m_iCrossHairID = DumpToStreams(lpIn, szOut, "m_iCrossHairID", m_bHasDefuser + 0x5C);
	DWORD m_flFlashDuration = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayer", "m_flFlashDuration", NULL, 0);
	DWORD m_iGlowIndex = DumpToStreams(lpIn, szOut, "m_iGlowIndex", m_flFlashDuration + 0x18);
	DWORD m_iShotsFired = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayer", "m_iShotsFired", NULL, 0);
	DWORD m_bIsScoped = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayer", "m_bIsScoped", NULL, 0);
	DWORD m_vecPunch = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BasePlayer", "m_Local", "m_vecPunch", 0x70);
	DWORD m_dwBoneMatrix = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseAnimating", "m_nForceBone", "m_dwBoneMatrix", 0x1C);
	DWORD m_iPlayerC4 = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayerResource", "m_iPlayerC4", NULL, 0);
	DWORD m_bSpotted = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseEntity", "m_bSpotted", NULL, 0);

	DumpSpace(szOut);

	DWORD m_vecOrigin = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseEntity", "m_vecOrigin", NULL, 0);
	DWORD m_angRotation = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseEntity", "m_angRotation", NULL, 0);
	DWORD m_vecViewOffset = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BasePlayer", "m_vecViewOffset[0]", "m_vecViewOffset", 0);
	DWORD m_fFlags = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_CSPlayer", "m_fFlags", NULL, 0);
	DWORD m_iHealth = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BasePlayer", "m_iHealth", NULL, 0);
	DWORD m_iTeamNum = DumpNetvar(lpIn, szOut, hProcess, dwGetAllClasses, "DT_BaseEntity", "m_iTeamNum", NULL, 0);

	BYTE bDormant[18] = "\x55\x8B\xEC\x53\x8B\x5D\x08\x56\x8B\xF1\x88\x9E\xAA\xAA\xAA\xAA\xE8";
	DWORD m_bDormant = DumpOffset(lpIn, szOut, Client, "m_bDormant", bDormant, sizeof(bDormant) - 1, 0xAA, 0xC, 0x0, TRUE, FALSE);

	if (lpIn) { free(lpIn); }

	WriteToDumpFile(hFile, szOut);
}

int main()
{
	MODULE32 Client = { 0 };
	MODULE32 Engine = { 0 };

	DWORD dwProcessId = 0;
	HANDLE hProcess = NULL;

	dwProcessId = GetProcessIdByProcessName(_T("csgo.exe"));
	printf("[+] csgo.exe process id: %d\n", dwProcessId);

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
	if (!Client.pbBuffer || !ReadMem(hProcess, Client.dwBase, Client.pbBuffer, Client.dwSize))
	{
		printf("[!] Error dumping client_panorama.dll!\n");
		goto cleanup;
	}

	Engine.dwBase = GetModuleBaseAddress(dwProcessId, _T("engine.dll"));
	printf("[+] engine.dll base: 0x%x\n", Engine.dwBase);
	
	Engine.dwSize = GetModuleSize(dwProcessId, _T("engine.dll"));
	printf("[+] engine.dll size: 0x%x\n", Engine.dwSize);
	
	Engine.pbBuffer = (PBYTE)malloc(Engine.dwSize);
	if (!Engine.pbBuffer || !ReadMem(hProcess, Engine.dwBase, Engine.pbBuffer, Engine.dwSize))
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