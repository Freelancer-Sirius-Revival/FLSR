#include <FLHook.h>
#include "Objectives.h"
#include "ObjDelay.h"
#include "ObjDock.h"
#include "ObjFollow.h"
#include "ObjGoto.h"
#include "../Mission.h"
#include "../Conditions/CndDistObj.h"
#include "../Conditions/CndDistVec.h"
#include "../Conditions/CndTimer.h"

namespace Missions
{
	class CndDistVecGoto : public CndDistVec
	{
	private:
		const uint objName;
	public:
		CndDistVecGoto(const ConditionParent& parent,
						const uint objNameOrLabel,
						const Vector& position,
						const float distance,
						const uint systemId) :
			CndDistVec(parent, objName, CndDistVec::DistanceCondition::Inside, position, distance, systemId),
			objName(objName)
		{}

		void ExecuteTrigger()
		{
			Unregister();

			const auto& missionEntry = missions.find(parent.missionId);
			if (missionEntry == missions.end())
				return;

			const auto& objectEntry = missionEntry->second.objectIdsByName.find(objName);
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
		const uint objName;
	public:
		CndDistObjGoto(const ConditionParent& parent,
			const uint objName,
			const float distance,
			const uint otherObjNameOrLabel) :
			CndDistObj(parent, objName, CndDistObj::DistanceCondition::Inside, distance, otherObjNameOrLabel),
			objName(objName)
		{}

		void ExecuteTrigger()
		{
			Unregister();

			const auto& missionEntry = missions.find(parent.missionId);
			if (missionEntry == missions.end())
				return;

			const auto& objectEntry = missionEntry->second.objectIdsByName.find(objName);
			if (objectEntry == missionEntry->second.objectIdsByName.end())
				return;

			const auto& objective = missionEntry->second.objectivesByObjectId.find(objectEntry->second);
			if (objective == missionEntry->second.objectivesByObjectId.end())
				return;

			objective->second.currentCondition = nullptr; // Has been unregistered.
			objective->second.Progress();
		}
	};

	class CndTimerDelay : public CndTimer
	{
	private:
		const uint objName;
	public:
		CndTimerDelay(const ConditionParent& parent, const uint objName, const float time) :
			CndTimer(parent, time, 0.0f), objName(objName)
		{}

		void ExecuteTrigger()
		{
			Unregister();

			const auto& missionEntry = missions.find(parent.missionId);
			if (missionEntry == missions.end())
				return;

			const auto& objectEntry = missionEntry->second.objectIdsByName.find(objName);
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
		const auto& missionEntry = missions.find(missionId);
		if (missionEntry == missions.end())
			return;
		uint objName = 0;
		for (const auto& entry : missionEntry->second.objectIdsByName)
		{
			if (entry.second == objId)
			{
				objName = entry.first;
				break;
			}
		}
		if (!objName)
			return;

		switch (entry.first)
		{
			case ObjectiveType::BreakFormation:
			{
				pub::AI::DirectiveIdleOp idleOp;
				idleOp.fireWeapons = true;
				pub::AI::SubmitDirective(objId, &idleOp);

				condition = ConditionPtr(new CndTimerDelay(ConditionParent(missionId, 0), objName, 0.0f));
				condition->Register();
				break;
			}

			case ObjectiveType::Delay:
			{
				const auto& delayArch = std::static_pointer_cast<ObjDelay>(entry.second);
				pub::AI::DirectiveDelayOp delayOp;
				delayOp.fireWeapons = false;
				delayOp.DelayTime = delayArch->timeInS;
				pub::AI::SubmitDirective(objId, &delayOp);

				condition = ConditionPtr(new CndTimerDelay(ConditionParent(missionId, 0), objName, delayArch->timeInS));
				condition->Register();
				break;
			}

			case ObjectiveType::Dock:
			{
				const auto& dockArch = std::static_pointer_cast<ObjDock>(entry.second);
				pub::AI::DirectiveDockOp dockOp;
				dockOp.fireWeapons = false;
				dockOp.dockTargetObjId = 0;
				if (const auto& objectEntry = missionEntry->second.objectIdsByName.find(dockArch->targetObjNameOrId); objectEntry != missionEntry->second.objectIdsByName.end())
					dockOp.dockTargetObjId = objectEntry->second;
				if (!dockOp.dockTargetObjId)
					dockOp.dockTargetObjId = dockArch->targetObjNameOrId;
				pub::AI::SubmitDirective(objId, &dockOp);

				// No condition. Never finishes.

				break;
			}

			case ObjectiveType::Follow:
			{
				const auto& missionEntry = missions.find(missionId);
				if (missionEntry == missions.end())
					return;

				IObjRW* inspect;
				StarSystem* starSystem;
				if (!(GetShipInspect(objId, inspect, starSystem) && (inspect->cobj->objectClass & CObject::CSHIP_OBJECT)))
					return;

				const auto& followArch = std::static_pointer_cast<ObjFollow>(entry.second);
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

				// No condition. Never finishes.

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

				const auto& gotoArch = std::static_pointer_cast<ObjGoto>(entry.second);

				if (gotoArch->type == pub::AI::GotoOpType::Vec || gotoArch->type == pub::AI::GotoOpType::Spline)
				{
					condition = ConditionPtr(new CndDistVecGoto(ConditionParent(missionId, 0),
						objName,
						gotoArch->type == pub::AI::GotoOpType::Vec ? gotoArch->position : gotoArch->spline[3],
						gotoArch->range + std::clamp(10.0f, gotoArch->range * 0.1f, 100.0f), // Add tolerance area to make sure NPCs will really trigger this.
						inspect->cobj->system
					));
					condition->Register();
				}
				else if (gotoArch->type == pub::AI::GotoOpType::Ship)
				{
					condition = ConditionPtr(new CndDistObjGoto(ConditionParent(missionId, 0),
						objName,
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
							gotoOp.targetId = objectEntry->second;
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