#pragma once
#include "Condition.h"

namespace Missions
{
	struct DistVecMatchEntry
	{
		uint systemId;
		Vector position;
	};

	class CndDistVec : public Condition
	{
	public:
		enum class DistanceCondition
		{
			Inside,
			Outside
		};

	//private:
		const uint objNameOrLabel;
		const DistanceCondition condition;
		const Vector position;
		const float distance;
		const uint systemId;

	public:
		CndDistVec(const ConditionParent& parent,
					const uint objNameOrLabel,
					const DistanceCondition condition,
					const Vector& position,
					const float distance,
					const uint systemId);
		~CndDistVec();
		void Register();
		void Unregister();
		bool Matches(const std::unordered_map<uint, DistVecMatchEntry>& clientsByClientId, const std::unordered_map<uint, DistVecMatchEntry>& objectsByObjId);
	};

	extern std::unordered_set<CndDistVec*> distVecConditions;
}