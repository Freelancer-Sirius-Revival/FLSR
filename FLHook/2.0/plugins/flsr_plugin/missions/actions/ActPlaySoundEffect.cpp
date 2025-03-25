#include <FLHook.h>
#include "ActPlaySoundEffect.h"
#include "../Mission.h"

namespace Missions
{
	ActPlaySoundEffect::ActPlaySoundEffect(const ActionParent& parent, const ActPlaySoundEffectArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_PlaySoundEffect),
		archetype(actionArchetype)
	{}

	static void PlaySoundEffect(const uint clientId, const uint soundId)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;
		pub::Audio::PlaySoundEffect(clientId, soundId);
	}

	void ActPlaySoundEffect::Execute()
	{
		auto& mission = missions.at(parent.missionId);
		const auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_PlaySoundEffect " + std::to_wstring(archetype->soundId) + L" for " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger.activator;
			if (activator.type == MissionObjectType::Client && activator.id)
			{
				PlaySoundEffect(activator.id, archetype->soundId);
				ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else
		{
			if (const auto& objectsByLabel = mission.objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != mission.objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.type == MissionObjectType::Client)
					{
						PlaySoundEffect(object.id, archetype->soundId);
						ConPrint(L" client[" + std::to_wstring(object.id) + L"]");
					}
				}
			}
		}
		ConPrint(L"\n");
	}
}