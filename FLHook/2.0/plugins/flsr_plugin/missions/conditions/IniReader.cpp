#include "IniReader.h"
#include "CndBaseEnter.h"
#include "CndCloaked.h"
#include "CndCommComplete.h"
#include "CndCount.h"
#include "CndDestroyed.h"
#include "CndDistObj.h"
#include "CndDistVec.h"
#include "CndHealthDec.h"
#include "CndProjHitCount.h"
#include "CndSpaceEnter.h"
#include "CndSpaceExit.h"
#include "CndTimer.h"
#include "CndTrue.h"

namespace Missions
{
	static uint CreateIdOrNull(const char* str)
	{
		return strlen(str) > 0 ? CreateID(str) : 0;
	}

	static void PrintErrorToConsole(const std::wstring& entryName, const ConditionParent& conditionParent, const uint argNum, const std::wstring& error)
	{
		ConPrint(L"ERROR: " + entryName + L" of Msn:" + std::to_wstring(conditionParent.missionId) + L" Trig:" + std::to_wstring(conditionParent.triggerId) + L" Arg " + std::to_wstring(argNum + 1)  + L" " + error + L"\n");
	}

	static CndBaseEnter* ReadCndBaseEnter(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		uint baseId = 0;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_BaseEnter", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				baseId = value;
			else
				PrintErrorToConsole(L"Cnd_BaseEnter", conditionParent, argNum, L"Invalid target base. Defaulting to any base.");
		}

		return new CndBaseEnter(conditionParent, label, baseId);
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
			const auto& value = ini.get_value_bool(argNum);
			if (value >= 0)
				targetCount = value;
			else
				PrintErrorToConsole(L"Cnd_Count", conditionParent, argNum, L"Invalid target count. Must be equal or greater than 0 Defaulting to 0.");
			argNum++;
		}

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
			argNum++;
		}

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				killerNameOrLabel = value;
			else
				PrintErrorToConsole(L"Cnd_Destroyed", conditionParent, argNum, L"Invalid killer obj name or label. Defaulting to all mission objects.");
		}

		return new CndDestroyed(conditionParent, objNameOrLabel, reason, killerNameOrLabel, targetCount);
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
			distance = ini.get_value_float(argNum);
		else
		{
			PrintErrorToConsole(L"Cnd_DistObj", conditionParent, argNum, L"No distance. Aborting!");
			return nullptr;
		}
		argNum++;

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
		Vector position;
		float distance = 0;
		uint systemId = 0;

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
		}
		else
		{
			PrintErrorToConsole(L"Cnd_DistVec", conditionParent, argNum, L"No position vector. Aborting!");
			return nullptr;
		}
		argNum += 3;

		if (ini.get_num_parameters() > argNum)
			distance = ini.get_value_float(argNum);
		else
		{
			PrintErrorToConsole(L"Cnd_DistVec", conditionParent, argNum, L"No distance. Aborting!");
			return nullptr;
		}
		argNum++;

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

		return new CndDistVec(conditionParent, objNameOrLabel, reason, position, distance, systemId);
	}

	static CndHealthDec* ReadCndHealthDec(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint objNameOrLabel;
		float remainingHitpoints;
		std::unordered_set<uint> colGrpIds;

		uint argNum = 0;
		objNameOrLabel = CreateIdOrNull(ini.get_value_string(argNum));
		if (objNameOrLabel == 0)
		{
			PrintErrorToConsole(L"Cnd_HealthDec", conditionParent, argNum, L"No target obj name or label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
			remainingHitpoints = ini.get_value_float(argNum);
		else
		{
			PrintErrorToConsole(L"Cnd_HealthDec", conditionParent, argNum, L"No lost hitpoints threshold. Aborting!");
			return nullptr;
		}
		argNum++;

		for (int maxArgs = ini.get_num_parameters(); argNum < maxArgs; argNum++)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value == 0)
				PrintErrorToConsole(L"Cnd_HealthDec", conditionParent, argNum, L"Invalid collision group name. Ignoring.");
			else
				colGrpIds.insert(value);
		}
		if (colGrpIds.empty())
			colGrpIds.insert(RootGroup);

		return new CndHealthDec(conditionParent, objNameOrLabel, remainingHitpoints, colGrpIds);
	}

	static CndProjHitCount* ReadCndProjHit(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint damagedObjNameOrLabel = 0;
		CndProjHitCount::DamagedSurface targetSurface = CndProjHitCount::DamagedSurface::Any;
		CndProjHitCount::DamageType damageType = CndProjHitCount::DamageType::Any;
		uint targetHitCount = 1;
		uint inflictorObjNameOrLabel = 0;

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
			argNum++;
		}

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "hull")
				targetSurface = CndProjHitCount::DamagedSurface::Hull;
			else if (value == "shield")
				targetSurface = CndProjHitCount::DamagedSurface::Shield;
			else if (value != "any")
				PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"Invalid damage surface. Must be ANY, HULL, or SHIELD. Defaulting to ANY.");
			argNum++;
		}

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "projectile")
				damageType = CndProjHitCount::DamageType::Projectile;
			else if (value == "explosion")
				damageType = CndProjHitCount::DamageType::Explosion;
			else if (value != "any")
				PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"Invalid damage type. Must be ANY, PROJECTILE, or EXPLOSION. Defaulting to ANY.");
			argNum++;
		}

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				inflictorObjNameOrLabel = value;
			else
				PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"Invalid damage inflictor obj name or label. Defaulting to all mission objects.");
		}

		return new CndProjHitCount(conditionParent, damagedObjNameOrLabel, targetSurface, damageType, targetHitCount, inflictorObjNameOrLabel);
	}

	static CndSpaceEnter* ReadCndSpaceEnter(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		uint systemId = 0;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_SpaceEnter", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				systemId = value;
			else
				PrintErrorToConsole(L"Cnd_SpaceEnter", conditionParent, argNum, L"Invalid target system. Defaulting to any system.");
		}

		return new CndSpaceEnter(conditionParent, label, systemId);
	}

	static CndSpaceExit* ReadCndSpaceExit(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint label = 0;
		uint systemId = 0;

		uint argNum = 0;
		label = CreateIdOrNull(ini.get_value_string(argNum));
		if (label == 0)
		{
			PrintErrorToConsole(L"Cnd_SpaceExit", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				systemId = value;
			else
				PrintErrorToConsole(L"Cnd_SpaceExit", conditionParent, argNum, L"Invalid target system. Defaulting to any system.");
		}

		return new CndSpaceExit(conditionParent, label, systemId);
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
			argNum++;
		}

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

		if (ini.is_value("Cnd_HealthDec"))
			return ReadCndHealthDec(conditionParent, ini);

		if (ini.is_value("Cnd_ProjHitCount"))
			return ReadCndProjHit(conditionParent, ini);

		if (ini.is_value("Cnd_SpaceEnter"))
			return ReadCndSpaceEnter(conditionParent, ini);

		if (ini.is_value("Cnd_SpaceExit"))
			return ReadCndSpaceExit(conditionParent, ini);

		if (ini.is_value("Cnd_Timer"))
			return ReadCndTimer(conditionParent, ini);

		if (ini.is_value("Cnd_True"))
			return new CndTrue(conditionParent);

		return nullptr;
	}
}