#include "Mission.h"
#include "Trigger.h"
#include "Actions/ActChangeStateArch.h"
#include "Actions/ActDestroyArch.h"

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

	bool StartMission(const std::string& missionName)
	{
		const auto foundMission = missionArchetypesByName.find(missionName);
		if (foundMission == missionArchetypesByName.end())
			return false;

		for (const Mission* mission : activeMissions)
		{
			if (mission->name == missionName)
			{
				return false;
			}
		}
		activeMissions.push_back(new Mission(foundMission->second));
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
				triggerArch.name = "Manual Abort";

				std::shared_ptr<ActChangeStateArchetype> actChangeArchetype(new ActChangeStateArchetype());
				actChangeArchetype->state = MissionState::ABORT;
				triggerArch.actions.push_back({ TriggerAction::Act_ChangeState, actChangeArchetype });

				for (const auto& object : mission->objects)
				{
					std::shared_ptr<ActDestroyArchetype> actDestroyArchetype(new ActDestroyArchetype());
					actDestroyArchetype->destroyType = DestroyType::VANISH;
					actDestroyArchetype->objNameOrLabel = object.name;
					triggerArch.actions.push_back({ TriggerAction::Act_Destroy, actDestroyArchetype });
				}

				Trigger* abortionTrigger = new Trigger(mission, triggerArch);
				abortionTrigger->QueueExecution();
				return true;
			}
		}
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