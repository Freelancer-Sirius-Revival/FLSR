#include <FLHook.h>
#include "ActPlayMusic.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActPlayMusic::ActPlayMusic(Trigger* parentTrigger, const ActPlayMusicArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_PlayMusic),
		archetype(actionArchetype)
	{}

	void ActPlayMusic::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_PlayMusic for " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger->condition->activator;
			if (activator.type == MissionObjectType::Client && activator.id)
			{
				pub::Audio::SetMusic(activator.id, archetype->music);
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
						pub::Audio::SetMusic(object.id, archetype->music);
						ConPrint(L" client[" + std::to_wstring(object.id) + L"]");
					}
				}
			}
		}
		ConPrint(L"\n");
	}
}