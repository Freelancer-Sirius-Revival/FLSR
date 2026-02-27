#pragma once
#include "Action.h"
#include "../conditions/CndCommComplete.h"

namespace Missions
{
	class ActDialogCndCommComplete : public CndCommComplete
	{
	private:
		Mission& mission;
		const MissionObject originalActivator;
		const Dialog& dialog;
		const int lineIndex;

	public:
		ActDialogCndCommComplete(Mission& mission, const MissionObject& originalActivator, const Dialog& dialog, const int lineIndex);
		void ExecuteTrigger();
	};

	struct ActStartDialog : Action
	{
		uint dialogId = 0;

		void Execute(Mission& mission, const MissionObject& activator) const;
	};
	typedef std::shared_ptr<ActStartDialog> ActStartDialogPtr;
}