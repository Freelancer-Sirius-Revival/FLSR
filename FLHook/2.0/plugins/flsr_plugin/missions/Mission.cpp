#include "Mission.h"
#include "Trigger.h"

namespace Missions
{
	std::vector<MissionArchetypePtr> missionArchetypes;

	std::unordered_map<uint, Mission> missions;

	static void ClearMusic(const uint clientId)
	{
		pub::Audio::Tryptich music;
		music.spaceMusic = 0;
		music.dangerMusic = 0;
		music.battleMusic = 0;
		music.overrideMusic = 0;
		music.playOnce = false;
		music.crossFadeDurationInS = 4.0f;
		pub::Audio::SetMusic(clientId, music);
	}

	uint lastTriggerId = 0;

	Mission::Mission() :
		id(0),
		archetype(nullptr),
		ended(true)
	{}

	Mission::Mission(const uint id, const MissionArchetypePtr missionArchetype) :
		id(id),
		archetype(missionArchetype),
		ended(false)
	{
		for (const auto& triggerArchetype : missionArchetype->triggers)
		{
			triggerIds.insert(++lastTriggerId);
			triggers.try_emplace(lastTriggerId, lastTriggerId, id, triggerArchetype);
		}
	}

	Mission::~Mission()
	{
		for (const auto& id : triggerIds)
			triggers.erase(id);
		
		for (const uint clientId : clientIds)
			ClearMusic(clientId);

		// Copy all ids of non-player-objects. The following process will modify the objects list implicitely.
		const std::unordered_set<uint> objectIdsCopy(objectIds);
		for (const uint objectId : objectIdsCopy)
		{
			if (pub::SpaceObj::ExistsAndAlive(objectId) == 0)
				pub::SpaceObj::Destroy(objectId, DestroyType::VANISH);
		}
	}

	void Mission::End()
	{
		ended = true;
	}

	void Mission::RemoveObject(const uint objId)
	{
		if (!objectIds.contains(objId))
			return;

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
		objectivesByObjectId.erase(objId);
	}

	void Mission::RemoveClient(const uint clientId)
	{
		ClearMusic(clientId);
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
		clientIds.erase(clientId);
	}

	uint lastMissionId = 0;

	bool StartMission(const std::string& missionName)
	{
		MissionArchetypePtr foundMissionArchetype = nullptr;
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

		for (const auto& mission : missions)
		{
			if (mission.second.archetype->name == missionName)
			{
				return false;
			}
		}
		missions.try_emplace(++lastMissionId, lastMissionId, foundMissionArchetype);
		// The following activation of triggers can cause a trigger to be instantly resolved and deleted. Make a copy of the list beforehand.
		const auto triggerIdsCopy = std::unordered_set(missions[lastMissionId].triggerIds);
		for (const auto& triggerId : triggerIdsCopy)
		{
			if (triggers[triggerId].archetype->active)
				triggers[triggerId].Activate();
		}
		return true;
	}

	bool KillMission(const std::string& missionName)
	{
		for (const auto& mission : missions)
		{
			if (mission.second.archetype->name == missionName)
			{
				missions.erase(mission.first);
				return true;
			}
		}
		return false;
	}

	void KillMissions()
	{
		// Do not use clear() function. Destruction of objects at mission deconstruction causes a hook call back on the missions list - which is just being modified.
		for (auto it = missions.begin(); it != missions.end();)
			it = missions.erase(it);
	}

	void RemoveObjectFromMissions(const uint objId)
	{
		for (auto& mission : missions)
			mission.second.RemoveObject(objId);
	}

	void RemoveClientFromMissions(const uint client)
	{
		for (auto& mission : missions)
			mission.second.RemoveClient(client);
	}
}