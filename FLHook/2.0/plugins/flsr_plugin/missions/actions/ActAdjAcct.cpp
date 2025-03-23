#include <FLHook.h>
#include "ActAdjAcct.h"
#include "../Mission.h"

namespace Missions
{
	ActAdjAcct::ActAdjAcct(const ActionParent& parent, const ActAdjAcctArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_AdjAcct),
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
		auto& mission = missions.at(parent.missionId);
		auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_AdjAcct " + std::to_wstring(archetype->cash) + L" on " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger.activator;
			if (activator.type == MissionObjectType::Client)
			{
				AddCash(activator.id, archetype->cash);
				ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
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