#include <FLHook.h>
#include "ActChangeState.h"

namespace Missions
{
	ActChangeState::ActChangeState(Trigger* parentTrigger, const ActChangeStateArchetype* archetype) :
		Action(parentTrigger, TriggerAction::Act_ChangeState),
		state(archetype->state),
		failTextId(archetype->failTextId)
	{}

	void ActChangeState::Execute()
	{
		ConPrint(stows(trigger->mission->name) + L"->" + stows(trigger->name) + L": Act_ChangeState to " + std::to_wstring(state) + L"\n");
	}
}