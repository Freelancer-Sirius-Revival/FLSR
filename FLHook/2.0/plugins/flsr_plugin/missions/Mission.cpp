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
		const std::unordered_set<uint> objectIdsCopy(objectIds);
		for (const uint objectId : objectIdsCopy)
		{
			if (pub::SpaceObj::ExistsAndAlive(objectId) == 0)
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

	void Mission::RemoveObject(const uint objId)
	{
		objectIds.erase(objId);
		for (auto it = objectIdsByName.begin(); it != objectIdsByName.end();)
		{
			if (it->second == objId)
				it = objectIdsByName.erase(it);
			else
				it++;
		}
		for (auto labelsIt = objectsByLabel.begin(); labelsIt != objectsByLabel.end();)
		{
			for (auto objsIt = labelsIt->second.begin(); objsIt != labelsIt->second.end();)
			{
				if (objsIt->type == MissionObjectType::Object && objsIt->id == objId)
					objsIt = labelsIt->second.erase(objsIt);
				else
					objsIt++;
			}
			if (labelsIt->second.empty())
				labelsIt = objectsByLabel.erase(labelsIt);
			else
				labelsIt++;
		}
	}

	void Mission::RemoveClient(const uint clientId)
	{
		clientIds.erase(clientId);
		for (auto labelsIt = objectsByLabel.begin(); labelsIt != objectsByLabel.end();)
		{
			for (auto objsIt = labelsIt->second.begin(); objsIt != labelsIt->second.end();)
			{
				if (objsIt->type == MissionObjectType::Client && objsIt->id == clientId)
					objsIt = labelsIt->second.erase(objsIt);
				else
					objsIt++;
			}
			if (labelsIt->second.empty())
				labelsIt = objectsByLabel.erase(labelsIt);
			else
				labelsIt++;
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
		for (const auto& mission : activeMissions)
			mission->RemoveObject(objId);
	}
}