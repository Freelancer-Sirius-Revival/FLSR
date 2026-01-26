#pragma once
#include <FLHook.h>

namespace BatsBotsShipTransferFix
{
    void __stdcall GFGoodBuy(const SGFGoodBuyInfo& gbi, unsigned int clientId);
    void __stdcall ReqEquipment(const EquipDescList& equipDescriptorList, unsigned int clientId);
}