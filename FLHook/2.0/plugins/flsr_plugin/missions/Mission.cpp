#include "Mission.h"
#include "Trigger.h"

namespace Missions
{
	std::vector<MissionArchetype> missionArchetypes;

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

		for (const auto& object : objects)
		{
			if (pub::SpaceObj::ExistsAndAlive(object.id) == 0) //0 means alive, -2 dead
			{
				pub::SpaceObj::Destroy(object.id, DestroyType::VANISH);
			}
		}

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

	bool StartMission(const std::string& missionName)
	{
		MissionArchetype* foundMissionArchetype = NULL;
		for (auto& mission : missionArchetypes)
		{
			if (mission.name == missionName)
			{
				foundMissionArchetype = &mission;
				break;
			}
		}
		if (!foundMissionArchetype)
			return false;

		for (const Mission* mission : activeMissions)
		{
			if (mission->name == missionName)
			{
				return false;
			}
		}
		activeMissions.push_back(new Mission(*foundMissionArchetype));
		return true;
	}

	bool KillMission(const std::string& missionName)
	{
		for (auto it = activeMissions.begin(); it != activeMissions.end(); it++)
		{
			const auto mission = *it;
			if (mission->name == missionName)
			{
				TriggerArchetype triggerArch;
				triggerArch.name = "Admin forced End";
				triggerArch.actions.push_back({ TriggerAction::Act_EndMission, NULL });
				Trigger* abortionTrigger = new Trigger(mission, triggerArch);
				abortionTrigger->QueueExecution();
				return true;
			}
		}
		return false;
	}

	void RemoveObjectFromMissions(const uint objId)
	{
		for (const auto& labelAndMission : activeMissions)
		{
			for (auto it = labelAndMission->objects.begin(); it != labelAndMission->objects.end();)
			{
				if (it->id == objId)
					it = labelAndMission->objects.erase(it);
				else
					it++;
			}
		}
	}
}