#include <FLHook.h>
#include "ActPlayMusic.h"
#include "../Mission.h"

namespace Missions
{
	ActPlayMusic::ActPlayMusic(const ActionParent& parent, const ActPlayMusicArchetypePtr actionArchetype) :
		Action(parent, ActionType::Act_PlayMusic),
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
		auto& mission = missions.at(parent.missionId);
		const auto& trigger = mission.triggers.at(parent.triggerId);
		ConPrint(stows(mission.archetype->name) + L"->" + stows(trigger.archetype->name) + L": Act_PlayMusic for " + std::to_wstring(archetype->objNameOrLabel));
		if (archetype->objNameOrLabel == Activator)
		{
			const auto& activator = trigger.activator;
			if (activator.type == MissionObjectType::Client && activator.id)
			{
				PlayMusic(activator.id, archetype->music);
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
						PlayMusic(object.id, archetype->music);
						ConPrint(L" client[" + std::to_wstring(object.id) + L"]");
					}
				}
			}
		}
		ConPrint(L"\n");
	}
}