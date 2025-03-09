#include <FLHook.h>
#include "ActPlaySoundEffect.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActPlaySoundEffect::ActPlaySoundEffect(const ActionParent& parent, const ActPlaySoundEffectArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_PlaySoundEffect),
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
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_PlaySoundEffect " + std::to_wstring(archetype->soundId) + L" for " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = triggers[parent.triggerId].activator;
			if (activator.type == MissionObjectType::Client && activator.id)
			{
				PlaySoundEffect(activator.id, archetype->soundId);
				ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else
		{
			if (const auto& objectsByLabel = missions[parent.missionId].objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != missions[parent.missionId].objectsByLabel.end())
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