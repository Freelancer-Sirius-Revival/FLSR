#include "ObjStayInRange.h"
#include "ObjCndTrue.h"

namespace Missions
{
	ObjStayInRange::ObjStayInRange(const ObjectiveParent& parent, const uint targetObjNameOrId, const Vector position, const float range, const bool active) :
		Objective(parent),
		targetObjNameOrId(targetObjNameOrId),
		position(position),
		range(range),
		active(active)
	{}

	enum ZoneType
	{
		Position = 0,
		SpaceObj = 2
	};

	enum ZoneSetup
	{
		Create = 0,
		Delete = 2
	};

	void ObjStayInRange::Execute(const ObjectiveState& state) const
	{
		if (pub::SpaceObj::ExistsAndAlive(state.objId) != 0)
			return;

		pub::AI::SetZoneBehaviorParams zoneParams;
		zoneParams.OpType = pub::AI::Buzz;
		zoneParams.fRadius = range;
		zoneParams.iDunno_0x10 = active ? ZoneSetup::Create : ZoneSetup::Delete;
		if (zoneParams.iDunno_0x10 != ZoneSetup::Delete)
		{
			zoneParams.iZoneType = targetObjNameOrId != 0 ? ZoneType::SpaceObj : ZoneType::Position;
			if (zoneParams.iZoneType == ZoneType::SpaceObj)
			{
				uint targetObjId = 0;
				const auto& mission = missions.at(parent.missionId);
				if (const auto& objectEntry = mission.objectIdsByName.find(targetObjNameOrId); objectEntry != mission.objectIdsByName.end())
					targetObjId = objectEntry->second;
				if (!targetObjId)
					targetObjId = targetObjNameOrId;

				if (targetObjId)
					zoneParams.iSpaceObj = targetObjId;
				else
					zoneParams.iDunno_0x10 = 2;
			}
			else
			{
				zoneParams.vPosition = position;
			}
		}
		pub::AI::SubmitState(state.objId, &zoneParams);

		RegisterCondition(state.objId, ConditionPtr(new ObjCndTrue(parent, state)));
	}
}