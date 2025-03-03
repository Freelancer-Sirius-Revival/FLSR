#include "Mission.h"
#include "Trigger.h"

namespace Missions
{
	std::vector<MissionArchetypePtr> missionArchetypes;

	std::vector<Mission*> activeMissions;

	Mission::Mission(const MissionArchetypePtr missionArchetype) :
		archetype(missionArchetype),
		ended(false)
	{
		std::vector<Trigger*> initialTriggers;
		for (const auto& triggerArchetype : missionArchetype->triggers)
		{
			Trigger* trigger = new Trigger(this, triggerArchetype);
			triggers.push_back(trigger);
			if (triggerArchetype->active)
				initialTriggers.push_back(trigger);
		}

		for (const auto& trigger : initialTriggers)
			trigger->Activate();
	}

	Mission::~Mission()
	{
		for (Trigger* trigger : triggers)
			delete trigger;
		
		// Copy all ids of non-player-objects. The following process will modify the objects list implicitely.
		std::vector<uint> objectIds;
		for (const auto& object : objects)
		{
			if (!object.clientId)
				objectIds.push_back(object.objId);
		}
		for (const uint objectId : objectIds)
		{
			if (pub::SpaceObj::ExistsAndAlive(objectId) == 0) //0 means alive, -2 dead
				pub::SpaceObj::Destroy(objectId, DestroyType::VANISH);
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
		std::shared_ptr<MissionArchetype> foundMissionArchetype = nullptr;
		for (const auto& mission : missionArchetypes)
		{
			if (mission->name == missionName)
			{
				foundMissionArchetype = mission;
				break;
			}
		}
		if (!foundMissionArchetype)
			return false;

		for (const Mission* mission : activeMissions)
		{
			if (mission->archetype->name == missionName)
			{
				return false;
			}
		}
		activeMissions.push_back(new Mission(foundMissionArchetype));
		return true;
	}

	bool KillMission(const std::string& missionName)
	{
		for (auto it = activeMissions.begin(); it != activeMissions.end(); it++)
		{
			const auto mission = *it;
			if (mission->archetype->name == missionName)
			{
				TriggerArchetypePtr triggerArch(new TriggerArchetype());
				triggerArch->name = "Admin forced End";
				triggerArch->actions.push_back({ TriggerAction::Act_EndMission, NULL });
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
				if (it->objId == objId)
					it = labelAndMission->objects.erase(it);
				else
					it++;
			}
		}
	}
}