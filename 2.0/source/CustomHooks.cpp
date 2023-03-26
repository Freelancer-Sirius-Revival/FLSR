#include <Windows.h>
#include "hook.h"


DWORD jmbk;




bool HookMEM(void* hookAddress, void* ourFunc, int len)
{

	if (len >= 5)
	{
		DWORD protection;
		VirtualProtect(hookAddress, len, PAGE_EXECUTE_READWRITE, &protection);
		memset(hookAddress, 0x90, len);
		DWORD relativeAddress = ((DWORD)ourFunc - (DWORD)hookAddress) - 5;
		*(BYTE*)hookAddress = 0xE9;
		*(DWORD*)((DWORD)hookAddress + 1) = relativeAddress;
		DWORD temp;
		VirtualProtect(hookAddress, len, protection, &temp);
		return true;

	}

	else

	{
		return false;
	}

}



BOOL IsBadMemPtr(BOOL write, void* ptr, size_t size)
{
	MEMORY_BASIC_INFORMATION mbi;
	BOOL ok;
	DWORD mask;
	BYTE* p = (BYTE*)ptr;
	BYTE* maxp = p + size;
	BYTE* regend;

	if (size == 0)
	{
		return FALSE;
	}

	if (p == NULL)
	{
		return TRUE;
	}

	if (write == FALSE)
	{
		mask = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
	}
	else
	{
		mask = PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
	}

	do
	{
		if (p == ptr || p == regend)
		{
			if (VirtualQuery((LPCVOID)p, &mbi, sizeof(mbi)) == 0)
			{
				return TRUE;
			}
			else
			{
				regend = ((BYTE*)mbi.BaseAddress + mbi.RegionSize);
			}
		}

		ok = (mbi.Protect & mask) != 0;

		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
		{
			ok = FALSE;
		}

		if (!ok)
		{
			return TRUE;
		}

		if (maxp <= regend) /* the whole address range is inside the current memory region */
		{
			return FALSE;
		}
		else if (maxp > regend) /* this region is a part of (or overlaps with) the address range we are checking */
		{
			p = regend; /* lets move to the next memory region */
		}
	} while (p < maxp);

	return FALSE;
}









int flag_z = 0;


void isValidPtr(DWORD DestroyMission)
{
	if (IsBadMemPtr(0, (void*)DestroyMission, 4))
	{
		flag_z = 1;
		AddLog("MISSIBUGFIX Triggered");
	}
}

_declspec(naked) void MissionBug_HotFix()
{
	_asm {

		mov eax, [esp + 0x14]
		mov eax, [eax + 0x24]
		pushad
		mov [flag_z],0
		push eax
		call isValidPtr
		add esp, 0x4
		popad
		cmp [flag_z],1
		jz block_mission

		mov esi, [eax]
		cmp esi, eax
		jmp[jmbk]

		block_mission:
		xor eax,eax
		jmp[jmbk]


	}

}




void HookFunc()
{
	

	DWORD missionbug_address = (DWORD)GetModuleHandleA("Content.dll") + 0x150C7;
	int  missionbug_address_Length = 11;
	jmbk = missionbug_address + missionbug_address_Length;
	HookMEM((void*)missionbug_address, MissionBug_HotFix, missionbug_address_Length);

}