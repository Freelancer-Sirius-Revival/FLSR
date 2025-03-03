#include "ActChangeState.h"

namespace Missions
{
	ActChangeState::ActChangeState(Trigger* parentTrigger, const ActChangeStateArchetypePtr actionArchetype) :
		Action(parentTrigger, TriggerAction::Act_ChangeState),
		archetype(actionArchetype)
	{}

	void ActChangeState::Execute()
	{
		ConPrint(stows(trigger->mission->archetype->name) + L"->" + stows(trigger->archetype->name) + L": Act_ChangeState to " + std::to_wstring(archetype->state) + L"\n");
	}
}