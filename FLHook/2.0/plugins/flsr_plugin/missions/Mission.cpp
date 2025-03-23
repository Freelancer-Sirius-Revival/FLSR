#include "Mission.h"

namespace Missions
{
	std::vector<MissionArchetypePtr> missionArchetypes;

	std::unordered_map<uint, Mission> missions;
	std::unordered_set<uint> runningMissionIds;

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

	Mission::Mission(const uint id, const MissionArchetypePtr missionArchetype) :
		id(id),
		archetype(missionArchetype)
	{
		uint lastTriggerId = 0;
		for (const auto& triggerArchetype : missionArchetype->triggers)
		{
			triggers.try_emplace(lastTriggerId, lastTriggerId, id, triggerArchetype);
			lastTriggerId++;
		}
	}

	Mission::~Mission()
	{
		runningMissionIds.erase(id);

		// Destroy all triggers first. This is to unregister all conditions before destroying any left-overs of this mission.
		triggers.clear();

		for (const uint clientId : clientIds)
			ClearMusic(clientId);

		// Destroy all spawned objects of this mission.
		for (const uint objectId : objectIds)
		{
			if (pub::SpaceObj::ExistsAndAlive(objectId) == 0)
				pub::SpaceObj::Destroy(objectId, DestroyType::VANISH);
		}
	}

	void Mission::Start()
	{
		runningMissionIds.insert(id);
		std::vector<uint> triggerIds;
		for (const auto& triggerEntry : triggers)
		{
			if (triggerEntry.second.archetype->initiallyActive)
				triggerIds.push_back(triggerEntry.first);
		}

		for (const auto& triggerId : triggerIds)
		{
			const auto& triggerEntry = triggers.find(triggerId);
			if (triggerEntry != triggers.end())
				triggerEntry->second.Activate();
		}
	}

	void Mission::QueueTriggerExecution(const uint triggerId)
	{
		triggerExecutionQueue.push(triggerId);

		// Directly after try to process all queued triggers.
		if (triggerExecutionRunning)
			return;
		triggerExecutionRunning = true;

		while (!triggerExecutionQueue.empty() && !ended)
		{
			Trigger& trigger = triggers.at(triggerExecutionQueue.front());
			triggerExecutionQueue.pop();
			trigger.Execute();
			if (!trigger.archetype->repeatable)
				triggers.erase(trigger.id);
		}
		triggerExecutionRunning = false;

		// Delete the mission if it has ended by one of the triggers.
		if (ended)
			missions.erase(id);
	}

	void Mission::End()
	{
		ended = true;
		runningMissionIds.erase(id);
	}

	void Mission::RemoveObject(const uint objId)
	{
		if (!objectIds.contains(objId))
			return;

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
		objectIds.erase(objId);
		objectivesByObjectId.erase(objId);
	}

	void Mission::RemoveClient(const uint clientId)
	{
		if (!clientIds.contains(clientId))
			return;

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
}