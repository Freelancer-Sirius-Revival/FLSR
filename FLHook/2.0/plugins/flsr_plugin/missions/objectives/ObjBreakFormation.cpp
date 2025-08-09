#include "ObjBreakFormation.h"
#include "ObjCndTrue.h"
#include "../Mission.h"
#include "../ShipSpawning.h"

namespace Missions
{
	ObjBreakFormation::ObjBreakFormation(const ObjectiveParent& parent, const int objectiveIndex) :
		Objective(parent, objectiveIndex)
	{}

	static void OrderBreakFormation(const uint objId)
	{
		pub::AI::DirectiveCancelOp cancelOp;
		cancelOp.fireWeapons = true;
		pub::AI::SubmitDirective(objId, &cancelOp);

		pub::AI::SetPersonalityParams personality;
		pub::AI::get_personality(objId, personality.personality);
		const uint graphId = pub::AI::get_state_graph_id(objId);
		personality.state_graph = pub::StateGraph::get_state_graph(graphId, pub::StateGraph::TYPE_LEADER);
		personality.state_id = true;
		personality.contentCallback = 0;
		personality.directiveCallback = 0;
		pub::AI::SubmitState(objId, &personality);

		pub::AI::update_formation_state(objId, objId, { 0, 0, 0 });
		UnassignFromWing(objId);
	}

	void ObjBreakFormation::Execute(const uint objId) const
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		Objective::Execute(objId);

		uint shipId = objId;
		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(shipId, inspect, system))
			return;
		IObjRW* formationLeader;
		inspect->get_formation_leader(formationLeader);
		if (formationLeader == inspect)
		{
			IObjRW** followers = nullptr;
			uint followerCount = 0;
			inspect->get_formation_followers(followers, followerCount);
			for (uint index = 0; index < followerCount; index++)
				OrderBreakFormation(followers[index]->get_id());
		}

		OrderBreakFormation(objId);

		RegisterCondition(objId, ConditionPtr(new ObjCndTrue(parent, objectiveIndex, objId)));
	}
}