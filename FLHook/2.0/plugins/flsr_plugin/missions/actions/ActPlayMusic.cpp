#include <FLHook.h>
#include "ActPlayMusic.h"
#include "../Conditions/Condition.h"

namespace Missions
{
	ActPlayMusic::ActPlayMusic(const ActionParent& parent, const ActPlayMusicArchetypePtr actionArchetype) :
		Action(parent, TriggerAction::Act_PlayMusic),
		archetype(actionArchetype)
	{}

	static void PlayMusic(const uint clientId, const pub::Audio::Tryptich& music)
	{
		if (!HkIsValidClientID(clientId) || HkIsInCharSelectMenu(clientId))
			return;
		pub::Audio::SetMusic(clientId, music);
	}

	void ActPlayMusic::Execute()
	{
		ConPrint(stows(missions[parent.missionId].archetype->name) + L"->" + stows(triggers[parent.triggerId].archetype->name) + L": Act_PlayMusic for " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = triggers[parent.triggerId].condition->activator;
			if (activator.type == MissionObjectType::Client && activator.id)
			{
				PlayMusic(activator.id, archetype->music);
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
						PlayMusic(object.id, archetype->music);
						ConPrint(L" client[" + std::to_wstring(object.id) + L"]");
					}
				}
			}
		}
		ConPrint(L"\n");
	}
}