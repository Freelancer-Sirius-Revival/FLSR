#include "IniReader.h"
#include "CndBaseEnter.h"
#include "CndCloaked.h"
#include "CndDistVec.h"
#include "CndProjHitCount.h"
#include "CndSpaceEnter.h"
#include "CndSpaceExit.h"
#include "CndTimer.h"

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
			PrintErrorToConsole(L"Cnd_Cloaked", conditionParent, argNum, L"No target label. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
			cloaked = ini.get_value_bool(argNum);

		return new CndCloaked(conditionParent, objNameOrLabel, cloaked);
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
			argNum++;
		}

		return new CndDistVec(conditionParent, objNameOrLabel, reason, position, distance, systemId);
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
			argNum++;
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

		if (ini.is_value("Cnd_DistVec"))
			return ReadCndDistVec(conditionParent, ini);

		if (ini.is_value("Cnd_ProjHitCount"))
			return ReadCndProjHit(conditionParent, ini);

		if (ini.is_value("Cnd_SpaceEnter"))
			return ReadCndSpaceEnter(conditionParent, ini);

		if (ini.is_value("Cnd_SpaceExit"))
			return ReadCndSpaceExit(conditionParent, ini);

		if (ini.is_value("Cnd_Timer"))
			return ReadCndTimer(conditionParent, ini);

		return nullptr;
	}
}