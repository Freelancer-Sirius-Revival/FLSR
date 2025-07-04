#include "Mission.h"
#include "conditions/CndCount.h"

namespace Missions
{
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

	Mission::Mission(const std::string name, const uint id, const bool initiallyActive) :
		name(name),
		id(id),
		initiallyActive(initiallyActive),
		state(initiallyActive ? MissionState::AwaitingInitialActivation : MissionState::Inactive),
		offerId(0),
		triggerExecutionRunning(false)
	{}

	Mission::~Mission()
	{
		// Destroy all triggers first. This is to unregister all conditions before destroying any left-overs of this mission.
		triggers.clear();
		End();
	}

	void Mission::Reset()
	{
		End();
		state = initiallyActive ? MissionState::AwaitingInitialActivation : MissionState::Inactive;
	}

	bool Mission::Start()
	{
		if (state != MissionState::Inactive && state != MissionState::AwaitingInitialActivation)
			return false;

		state = MissionState::Active;

		for (auto& trigger : triggers)
		{
			trigger.Reset();
			if (trigger.IsAwaitingInitialActivation())
				trigger.Activate();
		}
		return true;
	}

	void Mission::QueueTriggerExecution(const uint triggerId, const MissionObject& activator)
	{
		triggerExecutionQueue.push({ triggerId, activator });

		// Directly after try to process all queued triggers.
		if (triggerExecutionRunning)
			return;
		triggerExecutionRunning = true;

		while (!triggerExecutionQueue.empty() && state != MissionState::Finished)
		{
			const auto& entry = triggerExecutionQueue.front();
			Trigger& trigger = triggers.at(entry.first);
			triggerExecutionQueue.pop();
			trigger.Execute(entry.second);
		}
		triggerExecutionRunning = false;
	}

	void Mission::End()
	{
		state = MissionState::Finished;

		for (const uint clientId : clientIds)
			ClearMusic(clientId);

		// Destroy all spawned objects of this mission.
		for (const uint objectId : objectIds)
		{
			if (pub::SpaceObj::ExistsAndAlive(objectId) == 0)
				pub::SpaceObj::Destroy(objectId, DestroyType::VANISH);
		}

		countConditionsByMission.erase(id);
	}

	void Mission::EvaluateCountConditions(const uint label)
	{
		const auto& countConditions = countConditionsByMission.find(id);
		if (countConditions == countConditionsByMission.end() || countConditions->second.size() == 0)
			return;

		const auto& labelEntries = objectsByLabel.find(label);
		const uint count = labelEntries == objectsByLabel.end() ? 0 : labelEntries->second.size();
		const auto& countConditionsCopy = std::unordered_set(countConditions->second);
		for (const auto& cnd : countConditionsCopy)
		{
			if (countConditions->second.contains(cnd) && cnd->Matches(label, count))
				cnd->ExecuteTrigger();
		}
	}

	void Mission::AddObject(const uint objId, const uint name, const std::unordered_set<uint> labels)
	{
		objectIds.insert(objId);
		objectIdsByName[name] = objId;
		for (const auto& label : labels)
		{
			objectsByLabel[label].emplace_back(MissionObjectType::Object, objId);
			EvaluateCountConditions(label);
		}
	}

	void Mission::AddLabelToObject(const MissionObject& object, const uint label)
	{
		if (object.id == 0)
			return;

		if (object.type == MissionObjectType::Client)
		{
			if (!HkIsValidClientID(object.id) || HkIsInCharSelectMenu(object.id))
				return;
			// Clients are made known to the mission by giving them a label.
			clientIds.insert(object.id);
		}

		for (const auto& objectByLabel : objectsByLabel[label])
		{
			if (objectByLabel == object)
				return;
		}

		objectsByLabel[label].push_back(object);
		EvaluateCountConditions(label);
	}

	void Mission::RemoveLabelFromObject(const MissionObject& object, const uint label)
	{
		if (object.id == 0)
			return;

		if (const auto& objectByLabel = objectsByLabel.find(label); objectByLabel != objectsByLabel.end())
		{
			for (auto it = objectByLabel->second.begin(); it != objectByLabel->second.end();)
			{
				if (*it == object)
					it = objectByLabel->second.erase(it);
				else
					it++;
			}
			if (objectByLabel->second.empty())
				objectsByLabel.erase(label);
		}

		// Once all labels were removed from a client, delete it from the mission.
		if (object.type == MissionObjectType::Client)
		{
			for (const auto& objectByLabel : objectsByLabel)
			{
				for (const auto& foundObject : objectByLabel.second)
				{
					if (foundObject == object)
						return;
				}
			}
			clientIds.erase(object.id);
		}

		EvaluateCountConditions(label);
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
		std::vector<uint> labels;
		for (auto labelsIt = objectsByLabel.begin(); labelsIt != objectsByLabel.end();)
		{
			for (auto objsIt = labelsIt->second.begin(); objsIt != labelsIt->second.end();)
			{
				if (objsIt->type == MissionObjectType::Object && objsIt->id == objId)
				{
					labels.push_back(labelsIt->first);
					objsIt = labelsIt->second.erase(objsIt);
				}
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
		for (const auto& label : labels)
			EvaluateCountConditions(label);
	}

	void Mission::RemoveClient(const uint clientId)
	{
		if (!clientIds.contains(clientId))
			return;

		ClearMusic(clientId);
		std::vector<uint> labels;
		for (auto labelsIt = objectsByLabel.begin(); labelsIt != objectsByLabel.end();)
		{
			for (auto objsIt = labelsIt->second.begin(); objsIt != labelsIt->second.end();)
			{
				if (objsIt->type == MissionObjectType::Client && objsIt->id == clientId)
				{
					labels.push_back(labelsIt->first);
					objsIt = labelsIt->second.erase(objsIt);
				}
				else
					objsIt++;
			}
			if (labelsIt->second.empty())
				labelsIt = objectsByLabel.erase(labelsIt);
			else
				labelsIt++;
		}
		clientIds.erase(clientId);
		for (const auto& label : labels)
			EvaluateCountConditions(label);
	}
}