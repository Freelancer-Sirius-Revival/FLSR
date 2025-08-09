#include "ObjMakeNewFormation.h"
#include "ObjCndTrue.h"
#include "../Mission.h"
#include "../Formations.h"
#include "../ShipSpawning.h"

namespace Missions
{
	ObjMakeNewFormation::ObjMakeNewFormation(const ObjectiveParent& parent, const int objectiveIndex, const uint formationId, const std::vector<uint>& objNames) :
		Objective(parent, objectiveIndex),
		formationId(formationId),
		objNames(objNames)
	{}

	void ObjMakeNewFormation::Execute(const uint objId) const
	{
		if (pub::SpaceObj::ExistsAndAlive(objId) != 0)
			return;

		Objective::Execute(objId);

		auto& mission = missions.at(parent.missionId);
		const auto& formation = Formations::GetFormation(formationId);

		std::vector<uint> memberNames({ mission.FindObjNameByObjId(objId) });
		memberNames.insert(memberNames.end(), objNames.begin(), objNames.end());

		std::unordered_map<uint, std::string> stateGraphByMemberName;
		for (const auto& name : memberNames)
		{
			if (!mission.objectIdsByName.contains(name))
				continue;

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

			pub::AI::update_formation_state(memberId, objId, formation[index]);
			AssignToWing(memberId, objId);
		}

		const auto& condition = ConditionPtr(new ObjCndTrue(parent, objectiveIndex, objId));
		mission.objectiveConditionByObjectId.insert({ objId, condition });
		condition->Register();
	}
}