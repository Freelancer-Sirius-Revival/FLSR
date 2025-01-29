#include "Main.h"

// This solves a bug where nanobots or shield batteries will be transferred to a new ship if that has a capacity of 0 for those.
namespace BatsBotsShipTransferFix
{
	static std::unordered_map<uint, uint> boughtShipArchByClientId;

	void __stdcall GFGoodBuy(const SGFGoodBuyInfo& gbi, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		boughtShipArchByClientId.erase(clientId);
		const GoodInfo* goodInfo = GoodList::find_by_id(gbi.iGoodID);
		if (!goodInfo || goodInfo->iType != 3)
			return;
		const GoodInfo* hullGoodInfo = GoodList::find_by_id(goodInfo->iHullGoodID);
		if (hullGoodInfo && hullGoodInfo->iType == 2)
			boughtShipArchByClientId[clientId] = hullGoodInfo->iShipGoodID;
	}

	struct Result
	{
		bool erase = false;
		float refund = 0.0f;
	};

	static Result ReduceAndRefund(const st6::list<EquipDesc>::iterator& it, const int maxCount, const uint clientId)
	{
		Result result;
		const int countDiff = std::max(0, it->get_count() - maxCount);
		if (countDiff <= 0)
			return result;
		const GoodInfo* goodInfo = GoodList::find_by_id(it->get_arch_id());
		if (goodInfo)
			result.refund = goodInfo->fPrice * countDiff;
		if (maxCount > 0)
			it->iCount -= countDiff;
		else
			result.erase = true;
		return result;
	}

	void __stdcall ReqEquipment(const EquipDescList& equipDescriptorList, unsigned int clientId)
	{
		returncode = DEFAULT_RETURNCODE;

		if (!boughtShipArchByClientId.contains(clientId))
			return;

		Archetype::Ship* ship = Archetype::GetShip(boughtShipArchByClientId[clientId]);
		boughtShipArchByClientId.erase(clientId);
		if (!ship)
			return;

		float refund = 0;
		EquipDescList* list = (EquipDescList*)&equipDescriptorList;
		for (auto it = list->equip.begin(); it != list->equip.end(); )
		{
			const Archetype::Equipment* archetype = Archetype::GetEquipment(it->get_arch_id());
			if (!archetype)
			{
				it++;
				continue;
			}

			const Archetype::AClassType type = archetype->get_class_type();
			Result result;

			if (type == Archetype::AClassType::REPAIR_KIT)
				result = ReduceAndRefund(it, ship->iMaxNanobots, clientId);
			else if (type == Archetype::AClassType::SHIELD_BATTERY)
				result = ReduceAndRefund(it, ship->iMaxShieldBats, clientId);

			refund += result.refund;
			if (result.erase)
				it = list->equip.erase(it);
			else
				it++;
		}
		if (refund > 0.0f)
			pub::Player::AdjustCash(clientId, std::ceil(refund));
	}
}