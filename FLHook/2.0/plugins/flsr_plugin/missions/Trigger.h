#pragma once
#include <memory>
#include <FLHook.h>
#include "MissionObject.h"

namespace Missions
{
	struct Condition;
	typedef std::shared_ptr<Condition> ConditionPtr;

	struct Action;
	typedef std::shared_ptr<Action> ActionPtr;

	class Trigger
	{
	public:
		enum class TriggerRepeatable
		{
			Off,
			Auto,
			Manual
		};

		const std::string name;
		const uint nameId;
		const uint id;
		const uint missionId;
		const bool initiallyActive;
		const TriggerRepeatable repeatable;

	private:
		enum class TriggerState
		{
			AwaitingInitialActivation,
			Inactive,
			Active,
			Finished
		};
		TriggerState state;

	public:
		ConditionPtr condition;
		std::vector<ActionPtr> actions;

		Trigger(const std::string name,
				const uint id,
				const uint missionId,
				const bool initiallyActive,
				const TriggerRepeatable repeatable);
		virtual ~Trigger();
		bool IsAwaitingInitialActivation() const;
		void Reset();
		void Activate();
		void Deactivate();
		void Execute(const MissionObject& activator);
	};
}