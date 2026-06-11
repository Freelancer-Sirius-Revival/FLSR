#pragma once
#include <FLHook.h>

namespace EquipWhiteList
{
	void ReadFiles();
	void __stdcall BaseEnter_AFTER(unsigned int baseId, unsigned int clientId);
	void __stdcall BaseExit(unsigned int baseId, unsigned int clientId);
	void __stdcall GFGoodBuy(const SGFGoodBuyInfo& gbi, unsigned int clientId);
	void __stdcall ReqEquipment(const EquipDescList& equipDescriptorList, unsigned int clientId);
	void __stdcall ReqAddItem(unsigned int& goodArchetypeId, char* hardpoint, int& count, float& status, bool& mounted, uint clientId);
}