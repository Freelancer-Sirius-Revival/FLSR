#include "ObjBreakFormation.h"
#include "ObjCndTrue.h"
#include "../ShipSpawning.h"

namespace Missions
{
	ObjBreakFormation::ObjBreakFormation(const ObjectiveParent& parent) : Objective(parent)
	{}

	static void OrderBreakFormation(const uint objId)
	{
		pub::AI::DirectiveCancelOp cancelOp;
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
		ShipSpawning::UnassignFromWing(objId);
	}

	void ObjBreakFormation::Execute(const ObjectiveState& state) const
	{
		if (pub::SpaceObj::ExistsAndAlive(state.objId) != 0)
			return;

		Objective::Execute(state);

		uint shipId = state.objId;
		IObjRW* inspect;
		StarSystem* system;
		if (!GetShipInspect(shipId, inspect, system))
			return;
		IObjRW* formationLeader;
		inspect->get_formation_leader(formationLeader);
		if (formationLeader == inspect)
		{
			auto followers = std::vector<IObjRW*>(inspect->get_formation_follower_count());
			inspect->get_formation_followers(followers.data(), followers.size());
			for (uint index = 0, length = followers.size(); index < length; index++)
				OrderBreakFormation(followers[index]->get_id());
		}

		OrderBreakFormation(state.objId);

		RegisterCondition(state.objId, ConditionPtr(new ObjCndTrue(parent, state)));
	}
}