#include <FLHook.h>
#include "ActAdjAcct.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActAdjAcct::ActAdjAcct(const ActionParent& parent, const ActAdjAcctArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_AdjAcct),
		archetype(actionArchetype)
	{}

	static void AddCash(const uint clientId, const int cash)
	{
		int currentCash = MAXINT32;
		pub::Player::InspectCash(clientId, currentCash);
		const auto newCash = static_cast<long long int>(currentCash) + cash;
		if (newCash > MAXINT32)
			pub::Player::AdjustCash(clientId, MAXINT32 - currentCash);
		else if (newCash < 0)
			pub::Player::AdjustCash(clientId, -currentCash);
		else
			pub::Player::AdjustCash(clientId, cash);
	}

	void ActAdjAcct::Execute()
	{
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_AdjAcct " + std::to_wstring(archetype->cash) + L" on " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = triggers[parent.triggerId].condition->activator;
			if (activator.type == MissionObjectType::Client)
			{
				AddCash(activator.id, archetype->cash);
				ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
		{
			for (const auto& object : objectsByLabel->second)
			{
				if (object.type == MissionObjectType::Client)
				{
					AddCash(object.id, archetype->cash);
					ConPrint(L" client[" + std::to_wstring(object.id) + L"]");
				}
			}
		}
		ConPrint(L"\n");
	}
}