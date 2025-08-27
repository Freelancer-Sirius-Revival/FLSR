#include "ObjMakeNewFormation.h"
#include "ObjCndTrue.h"
#include "../Formations.h"
#include "../ShipSpawning.h"

namespace Missions
{
	ObjMakeNewFormation::ObjMakeNewFormation(const ObjectiveParent& parent, const uint formationId, const std::vector<uint>& objNames) :
		Objective(parent),
		formationId(formationId),
		objNames(objNames)
	{}

	void ObjMakeNewFormation::Execute(const ObjectiveState& state) const
	{
		if (pub::SpaceObj::ExistsAndAlive(state.objId) != 0)
			return;

		Objective::Execute(state);

		const auto& mission = missions.at(parent.missionId);
		const auto& formation = Formations::GetFormation(formationId);

		std::vector<uint> memberNames({ mission.FindObjNameByObjId(state.objId) });
		for (const auto& memberName : objNames)
		{
			if (mission.objectIdsByName.contains(memberName))
				memberNames.push_back(memberName);
		}

		std::unordered_map<uint, std::string> stateGraphByMemberName;
		for (const auto& name : memberNames)
		{
			if (const auto& msnNpcEntry = mission.msnNpcs.find(name); msnNpcEntry != mission.msnNpcs.end())
				if (const auto& npcEntry = mission.npcs.find(msnNpcEntry->second.npcId); npcEntry != mission.npcs.end())
					stateGraphByMemberName.insert({ name, npcEntry->second.stateGraph });
		}

		for (int index = 0, length = min(formation.size(), memberNames.size()); index < length; index++)
		{
			const uint memberId = mission.objectIdsByName.at(memberNames[index]);
			pub::AI::DirectiveCancelOp cancelOp;
			pub::AI::SubmitDirective(memberId, &cancelOp);

			pub::AI::SetPersonalityParams personality;
			pub::AI::get_personality(memberId, personality.personality);
			personality.state_graph = pub::StateGraph::get_state_graph(stateGraphByMemberName[memberNames[index]].c_str(), index == 0 ? pub::StateGraph::TYPE_LEADER : pub::StateGraph::TYPE_ESCORT);
			personality.state_id = true;
			personality.contentCallback = 0;
			personality.directiveCallback = 0;
			pub::AI::SubmitState(memberId, &personality);

			pub::AI::update_formation_state(memberId, state.objId, formation[index]);
			ShipSpawning::AssignToWing(memberId, state.objId);
		}

		RegisterCondition(state.objId, ConditionPtr(new ObjCndTrue(parent, state)));
	}
}