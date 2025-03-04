#include <FLHook.h>
#include "ActPlaySoundEffect.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActPlaySoundEffect::ActPlaySoundEffect(Trigger* parentTrigger, const ActPlaySoundEffectArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_PlaySoundEffect),
		archetype(actionArchetype)
	{}

	void ActPlaySoundEffect::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_PlaySoundEffect " + std::to_wstring(archetype->soundId) + L" for " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			auto& activator = trigger->condition->activator;
			if (activator.type == MissionObjectType::Client && activator.id)
			{
				pub::Audio::PlaySoundEffect(activator.id, archetype->soundId);
				ConPrint(L" client[" + std::to_wstring(activator.id) + L"]");
			}
		}
		else
		{
			if (const auto& objectsByLabel = trigger->mission->objectsByLabel.find(archetype->objNameOrLabel); objectsByLabel != trigger->mission->objectsByLabel.end())
			{
				for (const auto& object : objectsByLabel->second)
				{
					if (object.type == MissionObjectType::Client)
					{
						pub::Audio::PlaySoundEffect(object.id, archetype->soundId);
						ConPrint(L" client[" + std::to_wstring(object.id) + L"]");
					}
				}
			}
		}
		ConPrint(L"\n");
	}
}