#include "Mission.h"
#include "Trigger.h"

namespace Missions
{
	std::vector<Mission*> activeMissions;

	Mission::Mission(const MissionArchetype& missionArchetype) :
		archetype(missionArchetype),
		name(missionArchetype.name),
		reward(missionArchetype.reward),
		titleId(missionArchetype.titleId),
		offerId(missionArchetype.offerId),
		ended(false)
	{
		std::vector<Trigger*> initialTriggers;
		for (const auto& triggerArchetype : missionArchetype.triggers)
		{
			Trigger* trigger = new Trigger(this, triggerArchetype);
			triggers.push_back(trigger);
			if (triggerArchetype.active)
				initialTriggers.push_back(trigger);
		}

		for (const auto& trigger : initialTriggers)
			trigger->Activate();
	}

	Mission::~Mission()
	{
		for (Trigger* trigger : triggers)
			delete trigger;
		for (auto it = activeMissions.begin(); it != activeMissions.end(); it++)
		{
			const auto mission = *it;
			if (mission == this)
			{
				activeMissions.erase(it);
				return;
			}
		}
	}

	void Mission::End()
	{
		ended = true;
	}

	void Mission::RemoveTrigger(const Trigger* trigger)
	{
		for (auto it = triggers.begin(); it != triggers.end(); it++)
		{
			if (*it == trigger)
			{
				triggers.erase(it);
				return;
			}
		}
	}
}