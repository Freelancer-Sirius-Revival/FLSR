#include <FLHook.h>
#include "Objectives.h"
#include "ObjFollowArch.h"
#include "ObjGotoArch.h"
#include "../Mission.h"
#include "../Conditions/CndDistObj.h"
#include "../Conditions/CndDistVec.h"

namespace Missions
{
	class CndDistVecGoto : public CndDistVec
	{
	private:
		const uint objNameOrLabel;
	public:
		CndDistVecGoto(const ConditionParent& parent,
						const uint objNameOrLabel,
						const Vector& position,
						const float distance,
						const uint systemId) :
			CndDistVec(parent, objNameOrLabel, CndDistVec::DistanceCondition::Inside, position, distance, systemId),
			objNameOrLabel(objNameOrLabel)
		{}

		void ExecuteTrigger()
		{
			Unregister();

			const auto& missionEntry = missions.find(parent.missionId);
			if (missionEntry == missions.end())
				return;

			const auto& objectEntry = missionEntry->second.objectIdsByName.find(objNameOrLabel);
			if (objectEntry == missionEntry->second.objectIdsByName.end())
				return;

			const auto & objective = missionEntry->second.objectivesByObjectId.find(objectEntry->second); 
			if (objective == missionEntry->second.objectivesByObjectId.end())
				return;

			objective->second.currentCondition = nullptr; // Has been unregistered.
			objective->second.Progress();
		}
	};

	class CndDistObjGoto : public CndDistObj
	{
	private:
		const uint objNameOrLabel;
	public:
		CndDistObjGoto(const ConditionParent& parent,
			const uint objNameOrLabel,
			const float distance,
			const uint otherObjNameOrLabel) :
			CndDistObj(parent, objNameOrLabel, CndDistObj::DistanceCondition::Inside, distance, otherObjNameOrLabel),
			objNameOrLabel(objNameOrLabel)
		{}

		void ExecuteTrigger()
		{
			Unregister();

			const auto& missionEntry = missions.find(parent.missionId);
			if (missionEntry == missions.end())
				return;

			const auto& objectEntry = missionEntry->second.objectIdsByName.find(objNameOrLabel);
			if (objectEntry == missionEntry->second.objectIdsByName.end())
				return;

			const auto& objective = missionEntry->second.objectivesByObjectId.find(objectEntry->second);
			if (objective == missionEntry->second.objectivesByObjectId.end())
				return;

			objective->second.currentCondition = nullptr; // Has been unregistered.
			objective->second.Progress();
		}
	};


	Objectives::Objectives(const uint missionId, const uint objId, const std::vector<ObjectiveEntry>& objectives) :
		missionId(missionId),
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

	static void ApplyObjective(uint objId, const uint missionId, const ObjectiveEntry& entry, ConditionPtr& condition)
	{
		switch (entry.first)
		{
			case ObjectiveType::Follow:
			{
				const auto& missionEntry = missions.find(missionId);
				if (missionEntry == missions.end())
					return;

				IObjRW* inspect;
				StarSystem* starSystem;
				if (!(GetShipInspect(objId, inspect, starSystem) && (inspect->cobj->objectClass & CObject::CSHIP_OBJECT)))
					return;

				const auto& followArch = std::static_pointer_cast<ObjFollowArchetype>(entry.second);
				const auto& foundObject = missionEntry->second.objectIdsByName.find(followArch->objName);
				if (foundObject == missionEntry->second.objectIdsByName.end())
					return;

				pub::AI::DirectiveFollowOp followOp;
				followOp.fireWeapons = false;
				followOp.followSpaceObj = foundObject->second;
				followOp.maxDistance = followArch->maxDistance;
				followOp.offset = followArch->relativePosition;
				followOp.dunno2 = followArch->unk;
				pub::AI::SubmitDirective(objId, &followOp);
				break;
			}
			case ObjectiveType::Goto:
			{
				const auto& missionEntry = missions.find(missionId);
				if (missionEntry == missions.end())
					return;

				IObjRW* inspect;
				StarSystem* starSystem;
				if (!(GetShipInspect(objId, inspect, starSystem) && (inspect->cobj->objectClass & CObject::CSHIP_OBJECT)))
					return;

				const auto& gotoArch = std::static_pointer_cast<ObjGotoArchetype>(entry.second);

				if (gotoArch->type == pub::AI::GotoOpType::Vec || gotoArch->type == pub::AI::GotoOpType::Spline)
				{
					uint objNameOrLabel = 0;
					for (const auto& entry : missionEntry->second.objectIdsByName)
					{
						if (entry.second == objId)
						{
							objNameOrLabel = entry.first;
							break;
						}
					}
					if (!objNameOrLabel)
						return;

					condition = ConditionPtr(new CndDistVecGoto(ConditionParent(missionId, 0),
						objNameOrLabel,
						gotoArch->type == pub::AI::GotoOpType::Vec ? gotoArch->position : gotoArch->spline[3],
						gotoArch->range + std::clamp(10.0f, gotoArch->range * 0.1f, 100.0f), // Add tolerance area to make sure NPCs will really trigger this.
						inspect->cobj->system
					));
					condition->Register();
				}
				else if (gotoArch->type == pub::AI::GotoOpType::Ship)
				{
					uint objNameOrLabel = 0;
					for (const auto& entry : missionEntry->second.objectIdsByName)
					{
						if (entry.second == objId)
						{
							objNameOrLabel = entry.first;
							break;
						}
					}
					if (!objNameOrLabel)
						return;

					condition = ConditionPtr(new CndDistObjGoto(ConditionParent(missionId, 0),
						objNameOrLabel,
						gotoArch->range + std::clamp(10.0f, gotoArch->range * 0.1f, 100.0f), // Add tolerance area to make sure NPCs will really trigger this.
						gotoArch->targetObjNameOrId
					));
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
			ApplyObjective(objId, missionId, objectives.front(), currentCondition);
			objectives.pop();
		}
		else if (const auto& missionEntry = missions.find(missionId); missionEntry != missions.end())
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