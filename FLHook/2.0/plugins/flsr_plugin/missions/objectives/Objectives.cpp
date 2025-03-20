#include <FLHook.h>
#include "Objectives.h"
#include "ObjGotoArch.h"
#include "../Mission.h"
#include "../Conditions/CndDistVec.h"

namespace Missions
{
	struct CndDistVecGoto : CndDistVec
	{
		CndDistVecGoto(const uint parentMissionId, const CndDistVecArchetypePtr conditionArchetype) :
			CndDistVec(ConditionParent(parentMissionId, 0), conditionArchetype)
		{}

		void ExecuteTrigger()
		{
			const auto& missionEntry = missions.find(parent.missionId);
			if (missionEntry == missions.end())
				return;

			const auto& objectEntry = missionEntry->second.objectIdsByName.find(archetype->objNameOrLabel);
			if (objectEntry == missionEntry->second.objectIdsByName.end())
				return;

			const auto & objective = missionEntry->second.objectivesByObjectId.find(objectEntry->second); 
			if (objective == missionEntry->second.objectivesByObjectId.end())
				return;

			objective->second.Progress();
		}
	};

	Objectives::Objectives() :
		parentMissionId(0),
		objId(0),
		currentCondition(nullptr)
	{}

	Objectives::Objectives(const unsigned int parentMissionId, const unsigned int objId, const std::vector<ObjectiveEntry>& objectives) :
		parentMissionId(parentMissionId),
		objId(objId)
	{
		for (const auto& entry : objectives)
			this->objectives.push(entry);
	}

	Objectives::~Objectives()
	{
		if (currentCondition != nullptr)
			currentCondition->Unregister();
	}

	static void ApplyObjective(uint objId, const uint parentMissionId, const ObjectiveEntry& entry, ConditionPtr& condition)
	{
		switch (entry.first)
		{
			case ObjectiveType::Goto:
			{
				const auto& missionEntry = missions.find(parentMissionId);
				if (missionEntry == missions.end())
					return;

				IObjRW* inspect;
				StarSystem* starSystem;
				if (!(GetShipInspect(objId, inspect, starSystem) && (inspect->cobj->objectClass & CObject::CEQOBJ_MASK)))
					return;

				const auto& gotoArch = std::static_pointer_cast<ObjGotoArchetype>(entry.second);

				if (gotoArch->type == pub::AI::GotoOpType::Vec || gotoArch->type == pub::AI::GotoOpType::Spline)
				{
					CndDistVecArchetypePtr cndDistVecArch(new CndDistVecArchetype());
					for (const auto& entry : missionEntry->second.objectIdsByName)
					{
						if (entry.second == objId)
							cndDistVecArch->objNameOrLabel = entry.first;
					}
					cndDistVecArch->position = gotoArch->type == pub::AI::GotoOpType::Vec ? gotoArch->position : gotoArch->spline[3];
					cndDistVecArch->distance = gotoArch->range + std::clamp(10.0f, gotoArch->range * 0.1f, 100.0f); // Add tolerance area to make sure NPCs will really trigger this.
					cndDistVecArch->systemId = inspect->cobj->system;
					condition = std::shared_ptr<CndDistVecGoto>(new CndDistVecGoto(parentMissionId, cndDistVecArch));
					condition->Register();
				}

				pub::AI::DirectiveGotoOp gotoOp;
				gotoOp.fireWeapons = false;
				gotoOp.gotoType = gotoArch->type;
				switch (gotoArch->type)
				{
					case pub::AI::GotoOpType::Ship:
					{
						gotoOp.targetId = 0;
						if (const auto& objectEntry = missionEntry->second.objectIdsByName.find(gotoArch->targetObjNameOrId); objectEntry != missionEntry->second.objectIdsByName.end())
						{
							gotoOp.targetId = objectEntry->second;
						}
						if (!gotoOp.targetId)
							gotoOp.targetId = gotoArch->targetObjNameOrId;
						break;
					}

					case pub::AI::GotoOpType::Vec:
						gotoOp.pos = gotoArch->position;
						break;

					case pub::AI::GotoOpType::Spline:
					{
						for (byte index = 0; index < 4; index++)
							gotoOp.spline[index] = gotoArch->spline[index];
						break;
					}

					default:
						break;
				}
				gotoOp.range = gotoArch->range;
				gotoOp.thrust = gotoArch->thrust;
				gotoOp.goToCruise = gotoArch->movement == GotoMovement::Cruise;
				gotoOp.goToNoCruise = gotoArch->movement == GotoMovement::NoCruise;
				gotoOp.thrust = gotoArch->thrust;
				if (gotoArch->objNameToWaitFor)
				{
					if (const auto& objectEntry = missionEntry->second.objectIdsByName.find(gotoArch->objNameToWaitFor); objectEntry != missionEntry->second.objectIdsByName.end())
					{
						gotoOp.objIdToWaitFor = objectEntry->second;
					}
				}
				gotoOp.startWaitDistance = gotoArch->startWaitDistance;
				gotoOp.endWaitDistance = gotoArch->endWaitDistance;
				pub::AI::SubmitDirective(objId, &gotoOp);
			}
			break;

			default:
				break;
		}
	}

	void Objectives::Progress()
	{
		Cancel();
		if (!objectives.empty())
		{
			ApplyObjective(objId, parentMissionId, objectives.front(), currentCondition);
			objectives.pop();
		}
		else if (const auto& missionEntry = missions.find(parentMissionId); missionEntry != missions.end())
			missionEntry->second.objectivesByObjectId.erase(objId);
	}

	void Objectives::Cancel()
	{
		if (currentCondition != nullptr)
		{
			currentCondition->Unregister();
			currentCondition.reset();
		}
		pub::AI::DirectiveCancelOp cancelOp;
		pub::AI::SubmitDirective(objId, &cancelOp);
	}
}