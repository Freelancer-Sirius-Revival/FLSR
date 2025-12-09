#include "CndIniReader.h"
#include "CndBaseEnter.h"
#include "CndBaseExit.h"
#include "CndCloaked.h"
#include "CndCommComplete.h"
#include "CndCount.h"
#include "CndDestroyed.h"
#include "CndDistObj.h"
#include "CndDistVec.h"
#include "CndHasCargo.h"
#include "CndHealthDec.h"
#include "CndHealthInc.h"
#include "CndInSpace.h"
#include "CndInSystem.h"
#include "CndInZone.h"
#include "CndJumpInComplete.h"
#include "CndLaunchComplete.h"
#include "CndLeaveMsn.h"
#include "CndOnBase.h"
#include "CndProjHitCount.h"
#include "CndSystemSpaceEnter.h"
#include "CndSystemSpaceExit.h"
#include "CndTimer.h"
#include "CndTrue.h"

namespace Missions
{
	const uint RootGroup = CreateID("root");

	static uint CreateIdOrNull(const char* str)
	{
		return strlen(str) > 0 ? CreateID(str) : 0;
	}

	static void PrintErrorToConsole(const std::wstring& entryName, const ConditionParent& conditionParent, const uint argNum, const std::wstring& error)
	{
		ConPrint(L"ERROR: " + entryName + L" of Msn:" + std::to_wstring(conditionParent.missionId) + L" Trig:" + std::to_wstring(conditionParent.triggerId) + L" Arg " + std::to_wstring(argNum + 1) + L" " + error + L"\n");
	}

	static CndBaseEnter* ReadCndBaseEnter(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		std::unordered_set<uint> baseIds;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_BaseEnter", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				baseIds.insert(value);
			argNum++;
		}

		return new CndBaseEnter(conditionParent, label, baseIds);
	}

	static CndBaseExit* ReadCndBaseExit(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		std::unordered_set<uint> baseIds;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_BaseExit", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				baseIds.insert(value);
			argNum++;
		}

		return new CndBaseExit(conditionParent, label, baseIds);
	}

	static CndCloaked* ReadCndCloaked(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint objNameOrLabel = 0;
		bool cloaked = false;

		uint argNum = 0;
		objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (objNameOrLabel == 0)
		{
			PrintErrorToConsole(L"Cnd_Cloaked", conditionParent, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
			cloaked = ini.get_value_bool(argNum);

		return new CndCloaked(conditionParent, objNameOrLabel, cloaked);
	}

	static CndCommComplete* ReadCndCommComplete(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint commName = 0;

		uint argNum = 0;
		commName = CreateIdOrNull(ini.get_value_string(argNum));
		if (commName == 0)
		{
			PrintErrorToConsole(L"Cnd_CommComplete", conditionParent, argNum, L"No comm label. Aborting!");
			return nullptr;
		}

		return new CndCommComplete(conditionParent, commName);
	}

	static CndCount* ReadCndCount(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		int targetCount = 0;
		CndCount::CountComparator comparator = CndCount::CountComparator::Equal;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_Count", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_int(argNum);
			if (value >= 0)
				targetCount = value;
			else
				PrintErrorToConsole(L"Cnd_Count", conditionParent, argNum, L"Invalid target count. Must be equal or greater than 0 Defaulting to 0.");
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "less")
				comparator = CndCount::CountComparator::Less;
			else if (value == "greater")
				comparator = CndCount::CountComparator::Greater;
			else if (value != "equal")
				PrintErrorToConsole(L"Cnd_Count", conditionParent, argNum, L"Invalid comparator. Must be LESS, GREATER, or EQUAL. Defaulting to EQUAL.");
		}

		return new CndCount(conditionParent, label, targetCount, comparator);
	}

	static CndDestroyed* ReadCndDestroyed(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint objNameOrLabel = 0;
		CndDestroyed::DestroyCondition reason = CndDestroyed::DestroyCondition::Any;
		uint killerNameOrLabel = 0;
		int targetCount = -1;
		bool destroyedIsActivator = false;

		uint argNum = 0;
		objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (objNameOrLabel == 0)
		{
			PrintErrorToConsole(L"Cnd_Destroyed", conditionParent, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
			targetCount = ini.get_value_int(argNum++);

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "explode")
				reason = CndDestroyed::DestroyCondition::Explode;
			else if (value == "vanish")
				reason = CndDestroyed::DestroyCondition::Vanish;
			else if (value != "any")
				PrintErrorToConsole(L"Cnd_Destroyed", conditionParent, argNum, L"Invalid destruction type. Must be EXPLODE, VANISH, or ANY. Defaulting to ANY.");
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				killerNameOrLabel = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "destroyed")
				destroyedIsActivator = true;
			else if (value != "killer")
				PrintErrorToConsole(L"Cnd_Destroyed", conditionParent, argNum, L"Invalid activator target. Must be KILLER, or DESTROYED. Defaulting to INFLICTOR.");
		}

		return new CndDestroyed(conditionParent, objNameOrLabel, reason, killerNameOrLabel, targetCount, destroyedIsActivator);
	}

	static CndDistObj* ReadCndDistObj(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint objNameOrLabel = 0;
		CndDistObj::DistanceCondition reason = CndDistObj::DistanceCondition::Inside;
		float distance = 0;
		uint otherObjNameOrLabel = 0;

		uint argNum = 0;
		objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (objNameOrLabel == 0)
		{
			PrintErrorToConsole(L"Cnd_DistObj", conditionParent, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		otherObjNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (otherObjNameOrLabel == 0)
		{
			PrintErrorToConsole(L"Cnd_DistObj", conditionParent, argNum, L"No other ship. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			distance = ini.get_value_float(argNum);
			argNum++;
		}
		else
		{
			PrintErrorToConsole(L"Cnd_DistObj", conditionParent, argNum, L"No distance. Aborting!");
			return nullptr;
		}

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "outside")
				reason = CndDistObj::DistanceCondition::Outside;
			else if (value != "inside")
				PrintErrorToConsole(L"Cnd_DistObj", conditionParent, argNum, L"Invalid distance relation. Must be INSIDE, or OUTSIDE. Defaulting to INSIDE.");
		}

		return new CndDistObj(conditionParent, objNameOrLabel, reason, distance, otherObjNameOrLabel);
	}

	static CndDistVec* ReadCndDistVec(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint objNameOrLabel = 0;
		CndDistVec::DistanceCondition reason = CndDistVec::DistanceCondition::Inside;
		Vector position = { 0, 0, 0 };
		float distance = 0;
		uint systemId = 0;
		std::string hardpoint = "";

		uint argNum = 0;
		objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (objNameOrLabel == 0)
		{
			PrintErrorToConsole(L"Cnd_DistVec", conditionParent, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum + 2)
		{
			position.x = ini.get_value_float(argNum);
			position.y = ini.get_value_float(argNum + 1);
			position.z = ini.get_value_float(argNum + 2);
			argNum += 3;
		}
		else
		{
			PrintErrorToConsole(L"Cnd_DistVec", conditionParent, argNum, L"No position vector. Aborting!");
			return nullptr;
		}

		if (ini.get_num_parameters() > argNum)
		{
			distance = ini.get_value_float(argNum);
			argNum++;
		}
		else
		{
			PrintErrorToConsole(L"Cnd_DistVec", conditionParent, argNum, L"No distance. Aborting!");
			return nullptr;
		}

		systemId = CreateIdOrNull(ini.get_value_string(argNum));
		if (systemId == 0)
		{
			PrintErrorToConsole(L"Cnd_DistVec", conditionParent, argNum, L"No system. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "outside")
				reason = CndDistVec::DistanceCondition::Outside;
			else if (value != "inside")
				PrintErrorToConsole(L"Cnd_DistVec", conditionParent, argNum, L"Invalid distance relation. Must be INSIDE, or OUTSIDE. Defaulting to INSIDE.");
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			hardpoint = ToLower(ini.get_value_string(argNum));
			if (hardpoint.empty())
				PrintErrorToConsole(L"Cnd_DistVec", conditionParent, argNum, L"Invalid hardpoint. Defaulting to none.");
		}

		return new CndDistVec(conditionParent, objNameOrLabel, reason, position, distance, systemId, hardpoint);
	}

	static CndHasCargo* ReadCndHasCargo(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		std::unordered_map<uint, uint> countPerCargo;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_HasCargo", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		const auto maxArgs = ini.get_num_parameters();
		while (argNum < maxArgs)
		{
			const uint cargoId = CreateIdOrNull(ini.get_value_string(argNum++));
			if (cargoId == 0)
				PrintErrorToConsole(L"Cnd_HasCargo", conditionParent, argNum, L"Invalid archetype name. Ignoring.");

			const int count = ini.get_value_int(argNum++);
			if (count <= 0)
				PrintErrorToConsole(L"Cnd_HasCargo", conditionParent, argNum, L"Invalid quantity. Defaulting to 1.");

			if (cargoId)
				countPerCargo.insert({ cargoId, std::max<uint>(count, 1) });
		}

		return new CndHasCargo(conditionParent, label, countPerCargo);
	}

	static CndHealthDec* ReadCndHealthDec(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint objNameOrLabel = 0;
		float relativeHitpointsThreshold = 0;
		std::unordered_set<uint> colGrpIds;
		bool damagedIsActivator = false;

		uint argNum = 0;
		objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (objNameOrLabel == 0)
		{
			PrintErrorToConsole(L"Cnd_HealthDec", conditionParent, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
			relativeHitpointsThreshold = ini.get_value_float(argNum);
		else
		{
			PrintErrorToConsole(L"Cnd_HealthDec", conditionParent, argNum, L"No lost hitpoints threshold. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "damaged")
				damagedIsActivator = true;
			else if (value != "inflictor")
				PrintErrorToConsole(L"Cnd_HealthDec", conditionParent, argNum, L"Invalid activator target. Must be INFLICTOR, or DAMAGED. Defaulting to INFLICTOR.");
		}
		argNum++;

		for (const auto maxArgs = ini.get_num_parameters(); argNum < maxArgs; argNum++)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value == 0)
				PrintErrorToConsole(L"Cnd_HealthDec", conditionParent, argNum, L"Invalid collision group name. Ignoring.");
			else
				colGrpIds.insert(value);
		}
		if (colGrpIds.empty())
			colGrpIds.insert(RootGroup);

		return new CndHealthDec(conditionParent, objNameOrLabel, relativeHitpointsThreshold, colGrpIds, damagedIsActivator);
	}

	static CndHealthInc* ReadCndHealthInc(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint objNameOrLabel = 0;
		float relativeHitpointsThreshold = 0;
		std::unordered_set<uint> colGrpIds;
		bool repairedIsActivator = false;

		uint argNum = 0;
		objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (objNameOrLabel == 0)
		{
			PrintErrorToConsole(L"Cnd_HealthInc", conditionParent, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
			relativeHitpointsThreshold = ini.get_value_float(argNum);
		else
		{
			PrintErrorToConsole(L"Cnd_HealthInc", conditionParent, argNum, L"No gained hitpoints threshold. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "repaired")
				repairedIsActivator = true;
			else if (value != "inflictor")
				PrintErrorToConsole(L"Cnd_HealthInc", conditionParent, argNum, L"Invalid activator target. Must be INFLICTOR, or REPAIRED. Defaulting to INFLICTOR.");
		}
		argNum++;

		for (const auto maxArgs = ini.get_num_parameters(); argNum < maxArgs; argNum++)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value == 0)
				PrintErrorToConsole(L"Cnd_HealthInc", conditionParent, argNum, L"Invalid collision group name. Ignoring.");
			else
				colGrpIds.insert(value);
		}
		if (colGrpIds.empty())
			colGrpIds.insert(RootGroup);

		return new CndHealthInc(conditionParent, objNameOrLabel, relativeHitpointsThreshold, colGrpIds, repairedIsActivator);
	}

	static CndInSpace* ReadCndInSpace(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		std::unordered_set<uint> systemIds;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_InSpace", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				systemIds.insert(value);
			argNum++;
		}

		return new CndInSpace(conditionParent, label, systemIds);
	}

	static CndInSystem* ReadCndInSystem(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		std::unordered_set<uint> systemIds;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_InSystem", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				systemIds.insert(value);
			argNum++;
		}

		if (systemIds.empty())
		{
			PrintErrorToConsole(L"Cnd_InSystem", conditionParent, 1, L"No system given. Aborting!");
			return nullptr;
		}

		return new CndInSystem(conditionParent, label, systemIds);
	}

	static CndInZone* ReadCndInZone(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		std::unordered_set<uint> zoneIds;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_InZone", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				zoneIds.insert(value);
			argNum++;
		}

		if (zoneIds.empty())
		{
			PrintErrorToConsole(L"Cnd_InZone", conditionParent, 1, L"No zones given. Aborting!");
			return nullptr;
		}

		return new CndInZone(conditionParent, label, zoneIds);
	}

	static CndJumpInComplete* ReadCndJumpInComplete(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		std::unordered_set<uint> systemIds;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_JumpInComplete", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				systemIds.insert(value);
			argNum++;
		}

		return new CndJumpInComplete(conditionParent, label, systemIds);
	}

	static CndLaunchComplete* ReadCndLaunchComplete(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		std::unordered_set<uint> baseIds;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_LaunchComplete", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				baseIds.insert(value);
			argNum++;
		}

		return new CndLaunchComplete(conditionParent, label, baseIds);
	}

	static CndLeaveMsn* ReadCndLeaveMsn(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;

		const uint argNum = 0;
		if (ini.get_num_parameters() > argNum)
		{
			const std::string value = ToLower(ini.get_value_string(argNum));
			if (value != "any")
				label = CreateIdOrNull(value.c_str());
		}
		else
			PrintErrorToConsole(L"Cnd_LeaveMsn", conditionParent, argNum, L"No target label. Defaulting to ANY.");

		return new CndLeaveMsn(conditionParent, label);
	}

	static CndOnBase* ReadCndOnBase(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		std::unordered_set<uint> baseIds;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_OnBase", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				baseIds.insert(value);
			argNum++;
		}

		return new CndOnBase(conditionParent, label, baseIds);
	}

	static CndProjHitCount* ReadCndProjHit(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint damagedObjNameOrLabel = 0;
		CndProjHitCount::DamagedSurface targetSurface = CndProjHitCount::DamagedSurface::Any;
		CndProjHitCount::DamageType damageType = CndProjHitCount::DamageType::Any;
		uint targetHitCount = 1;
		uint inflictorObjNameOrLabel = 0;
		bool damagedIsActivator = false;

		uint argNum = 0;
		damagedObjNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (damagedObjNameOrLabel == 0)
		{
			PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_int(argNum);
			if (value > 0)
				targetHitCount = value;
			else
				PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"Target hit count must be greater than 0. Defaulting to " + std::to_wstring(targetHitCount) + L".");
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "hull")
				targetSurface = CndProjHitCount::DamagedSurface::Hull;
			else if (value == "shield")
				targetSurface = CndProjHitCount::DamagedSurface::Shield;
			else if (value != "any")
				PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"Invalid damage surface. Must be ANY, HULL, or SHIELD. Defaulting to ANY.");
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "projectile")
				damageType = CndProjHitCount::DamageType::Projectile;
			else if (value == "explosion")
				damageType = CndProjHitCount::DamageType::Explosion;
			else if (value != "any")
				PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"Invalid damage type. Must be ANY, PROJECTILE, or EXPLOSION. Defaulting to ANY.");
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				inflictorObjNameOrLabel = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "damaged")
				damagedIsActivator = true;
			else if (value != "inflictor")
				PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"Invalid activator target. Must be INFLICTOR, or DAMAGED. Defaulting to INFLICTOR.");
		}

		return new CndProjHitCount(conditionParent, damagedObjNameOrLabel, targetSurface, damageType, targetHitCount, inflictorObjNameOrLabel, damagedIsActivator);
	}

	static CndSystemSpaceEnter* ReadCndSystemSpaceEnter(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		CndSystemSpaceEnter::SystemEnterCondition reason = CndSystemSpaceEnter::SystemEnterCondition::Any;
		std::unordered_set<uint> systemIds;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_SystemSpaceEnter", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "jump")
				reason = CndSystemSpaceEnter::SystemEnterCondition::Jump;
			else if (value == "launch")
				reason = CndSystemSpaceEnter::SystemEnterCondition::Launch;
			else if (value == "spawn")
				reason = CndSystemSpaceEnter::SystemEnterCondition::Spawn;
			else if (value != "any")
				PrintErrorToConsole(L"Cnd_SystemSpaceEnter", conditionParent, argNum, L"Invalid enter type. Must be JUMP, LAUNCH, SPAWN, or ANY. Defaulting to ANY.");
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				systemIds.insert(value);
			argNum++;
		}

		return new CndSystemSpaceEnter(conditionParent, label, reason, systemIds);
	}

	static CndSystemSpaceExit* ReadCndSystemSpaceExit(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		CndSystemSpaceExit::SystemExitCondition reason = CndSystemSpaceExit::SystemExitCondition::Any;
		std::unordered_set<uint> systemIds;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_SystemSpaceExit", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "jump")
				reason = CndSystemSpaceExit::SystemExitCondition::Jump;
			else if (value == "vanish")
				reason = CndSystemSpaceExit::SystemExitCondition::Vanish;
			else if (value == "explode")
				reason = CndSystemSpaceExit::SystemExitCondition::Explode;
			else if (value == "dock")
				reason = CndSystemSpaceExit::SystemExitCondition::Dock;
			else if (value != "any")
				PrintErrorToConsole(L"Cnd_SystemSpaceExit", conditionParent, argNum, L"Invalid enter type. Must be JUMP, DOCK, EXPLDOE, VANISH, or ANY. Defaulting to ANY.");
		}
		argNum++;

		while (argNum < ini.get_num_parameters())
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				systemIds.insert(value);
			argNum++;
		}

		return new CndSystemSpaceExit(conditionParent, label, reason, systemIds);
	}

	static CndTimer* ReadCndTimer(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		float lowerTimeInS = 0.0f;
		float upperTimeInS = 0.0f;

		uint argNum = 0;
		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value >= 0.0f)
				lowerTimeInS = value;
			else
				PrintErrorToConsole(L"Cnd_Timer", conditionParent, argNum, L"Lower boundary must be equal or greater than 0. Defaulting to 0.");
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value >= 0.0f)
				upperTimeInS = value;
			else
				PrintErrorToConsole(L"Cnd_Timer", conditionParent, argNum, L"Upper boundary must be equal or greater than 0. Defaulting to 0.");
		}

		return new CndTimer(conditionParent, lowerTimeInS, upperTimeInS);
	}

	Condition* TryReadConditionFromIni(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		if (ini.is_value("Cnd_BaseEnter"))
			return ReadCndBaseEnter(conditionParent, ini);

		if (ini.is_value("Cnd_BaseExit"))
			return ReadCndBaseExit(conditionParent, ini);

		if (ini.is_value("Cnd_Cloaked"))
			return ReadCndCloaked(conditionParent, ini);

		if (ini.is_value("Cnd_CommComplete"))
			return ReadCndCommComplete(conditionParent, ini);

		if (ini.is_value("Cnd_Count"))
			return ReadCndCount(conditionParent, ini);

		if (ini.is_value("Cnd_Destroyed"))
			return ReadCndDestroyed(conditionParent, ini);

		if (ini.is_value("Cnd_DistObj"))
			return ReadCndDistObj(conditionParent, ini);

		if (ini.is_value("Cnd_DistVec"))
			return ReadCndDistVec(conditionParent, ini);

		if (ini.is_value("Cnd_HasCargo"))
			return ReadCndHasCargo(conditionParent, ini);

		if (ini.is_value("Cnd_HealthDec"))
			return ReadCndHealthDec(conditionParent, ini);

		if (ini.is_value("Cnd_HealthInc"))
			return ReadCndHealthInc(conditionParent, ini);

		if (ini.is_value("Cnd_InSpace"))
			return ReadCndInSpace(conditionParent, ini);

		if (ini.is_value("Cnd_InSystem"))
			return ReadCndInSystem(conditionParent, ini);

		if (ini.is_value("Cnd_InZone"))
			return ReadCndInZone(conditionParent, ini);

		if (ini.is_value("Cnd_JumpInComplete"))
			return ReadCndJumpInComplete(conditionParent, ini);

		if (ini.is_value("Cnd_LaunchComplete"))
			return ReadCndLaunchComplete(conditionParent, ini);

		if (ini.is_value("Cnd_LeaveMsn"))
			return ReadCndLeaveMsn(conditionParent, ini);

		if (ini.is_value("Cnd_OnBase"))
			return ReadCndOnBase(conditionParent, ini);

		if (ini.is_value("Cnd_ProjHitCount"))
			return ReadCndProjHit(conditionParent, ini);

		if (ini.is_value("Cnd_SystemSpaceEnter"))
			return ReadCndSystemSpaceEnter(conditionParent, ini);

		if (ini.is_value("Cnd_SystemSpaceExit"))
			return ReadCndSystemSpaceExit(conditionParent, ini);

		if (ini.is_value("Cnd_Timer"))
			return ReadCndTimer(conditionParent, ini);

		if (ini.is_value("Cnd_True"))
			return new CndTrue(conditionParent);

		return nullptr;
	}
}