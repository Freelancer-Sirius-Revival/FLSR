#pragma once
#include "Condition.h"

namespace Missions
{
	class CndInZone : public Condition
	{
	private:
		const uint objNameOrLabel;
		const std::unordered_set<uint> zoneIds;
		std::unordered_map<uint, uint> systemIdByZoneIds;

		bool IsInZone(uint objId);
	public:
		CndInZone(const ConditionParent& parent, const uint objNameOrLabel, const std::unordered_set<uint>& zoneIds);
		~CndInZone();
		void Register();
		void Unregister();
		bool Matches(const MissionObject& object);
	};

	namespace Hooks
	{
		namespace CndInZone
		{
			void __stdcall Elapse_Time_AFTER(float seconds);
		}
	}
}