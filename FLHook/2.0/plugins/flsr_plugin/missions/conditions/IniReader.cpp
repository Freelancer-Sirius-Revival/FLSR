#include "IniReader.h"
#include "CndProjHitCount.h"

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

	static CndProjHitCount* ReadCndProjHit(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		uint damagedObjNameOrLabel = 0;
		CndProjHitCount::DamagedSurface targetSurface = CndProjHitCount::DamagedSurface::Any;
		CndProjHitCount::DamageType damageType = CndProjHitCount::DamageType::Any;
		uint targetHitCount = 1;
		uint inflictorObjNameOrLabel = 0;

		uint argNum = 0;
		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				damagedObjNameOrLabel = value;
			else
			{
				PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"No target obj name or label. Aborting!");
				return nullptr;
			}
			argNum++;
		}

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
				PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"Invalid damage surface. Must be ANY, HULL, SHIELD. Defaulting to ANY.");
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
				PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"Invalid damage type. Must be ANY, PROJECTILE, EXPLOSION. Defaulting to ANY.");
			argNum++;
		}

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value != 0)
				inflictorObjNameOrLabel = value;
			else
				PrintErrorToConsole(L"Cnd_ProjHitCount", conditionParent, argNum, L"No damage inflictor obj name or label. Defaulting to all mission objects.");
			argNum++;
		}

		return new CndProjHitCount(conditionParent, damagedObjNameOrLabel, targetSurface, damageType, targetHitCount, inflictorObjNameOrLabel);
	}

	Condition* TryReadConditionFromIni(const ConditionParent& conditionParent, INI_Reader& ini)
	{
		if (ini.is_value("Cnd_ProjHitCount"))
			return ReadCndProjHit(conditionParent, ini);
		return nullptr;
	}
}