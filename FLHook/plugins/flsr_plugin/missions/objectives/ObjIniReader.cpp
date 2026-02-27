#pragma once
#include "ObjIniReader.h"
#include "ObjBreakFormation.h"
#include "ObjDelay.h"
#include "ObjDock.h"
#include "ObjFollow.h"
#include "ObjGotoObj.h"
#include "ObjGotoSpline.h"
#include "ObjGotoVec.h"
#include "ObjIdle.h"
#include "ObjMakeNewFormation.h"
#include "ObjSetLifeTime.h"
#include "ObjSetPriority.h"
#include "ObjStayInRange.h"

namespace Missions
{
	static uint CreateIdOrNull(const char* str)
	{
		return strlen(str) > 0 ? CreateID(str) : 0;
	}

	static void PrintErrorToConsole(const std::wstring& entryName, const ObjectiveParent& objectiveParent, const uint argNum, const std::wstring& error)
	{
		ConPrint(L"ERROR: " + entryName + L" of Msn:" + std::to_wstring(objectiveParent.missionId) + L" Obj:" + std::to_wstring(objectiveParent.objectivesId) + L" Arg " + std::to_wstring(argNum + 1) + L" " + error + L"\n");
	}

	static ObjDelay* ReadObjDelay(const ObjectiveParent& objectiveParent, INI_Reader& ini)
	{
		float timeInS = 0;

		uint argNum = 0;
		const auto& value = ini.get_value_float(argNum);
		if (value < 0)
			PrintErrorToConsole(L"Delay", objectiveParent, argNum, L"Time below 0. Defaulting to " + std::to_wstring(timeInS) + L".");
		else
			timeInS = value;

		return new ObjDelay(objectiveParent, timeInS);
	}

	static ObjDock* ReadObjDock(const ObjectiveParent& objectiveParent, INI_Reader& ini)
	{
		uint argNum = 0;
		const uint targetObjNameOrId = CreateIdOrNull(ini.get_value_string(argNum));
		if (targetObjNameOrId == 0)
		{
			PrintErrorToConsole(L"Dock", objectiveParent, argNum, L"No target obj name. Aborting!");
			return nullptr;
		}

		return new ObjDock(objectiveParent, targetObjNameOrId);
	}

	static ObjFollow* ReadObjFollow(const ObjectiveParent& objectiveParent, INI_Reader& ini)
	{
		uint targetObjName = 0;
		float maxDistance = 100.0f;
		Vector position = { 0, 0, 0 };

		uint argNum = 0;
		targetObjName = CreateIdOrNull(ini.get_value_string(argNum));
		if (targetObjName == 0)
		{
			PrintErrorToConsole(L"Follow", objectiveParent, argNum, L"No target obj name. Aborting!");
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
			PrintErrorToConsole(L"Follow", objectiveParent, argNum, L"No position vector. Aborting!");
			return nullptr;
		}
		argNum += 3;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value < 1.0f)
				PrintErrorToConsole(L"Follow", objectiveParent, argNum, L"Max distance below 1. Defaulting to " + std::to_wstring(maxDistance) + L".");
			else
				maxDistance = value;
		}

		return new ObjFollow(objectiveParent, targetObjName, maxDistance, position);
	}

	static ObjGotoObj* ReadObjGotoObj(const ObjectiveParent& objectiveParent, INI_Reader& ini)
	{
		uint targetObjNameOrId = 0;
		bool noCruise = false;
		float range = 100.0f;
		float thrust = -1.0f;
		uint objNameToWaitFor = 0;
		float startWaitDistance = 200.0f;
		float endWaitDistance = 500.0f;

		uint argNum = 0;
		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "goto_no_cruise")
				noCruise = true;
			else if (value != "goto_cruise")
				PrintErrorToConsole(L"GotoObj", objectiveParent, argNum, L"Invalid cruise setting. Must be GOTO_CRUISE, or GOTO_NO_CRUISE. Defaulting to GOTO_CRUISE.");
		}
		argNum++;

		targetObjNameOrId = CreateIdOrNull(ini.get_value_string(argNum));
		if (targetObjNameOrId == 0)
		{
			PrintErrorToConsole(L"GotoObj", objectiveParent, argNum, L"No target obj name. Aborting!");
			return nullptr;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value < 1.0f)
				PrintErrorToConsole(L"GotoObj", objectiveParent, argNum, L"Target range below 1. Defaulting to " + std::to_wstring(range) + L".");
			else
				range = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			thrust = ini.get_value_float(argNum);
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value == 0)
				PrintErrorToConsole(L"GotoObj", objectiveParent, argNum, L"Obj name to wait for is undefined. Defaulting to none.");
			else
				objNameToWaitFor = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value < 1.0f)
				PrintErrorToConsole(L"GotoObj", objectiveParent, argNum, L"Distance to start waiting for obj " + std::to_wstring(objNameToWaitFor) + L" is below 1. Defaulting to " + std::to_wstring(startWaitDistance) + L".");
			else
				startWaitDistance = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value < 1.0f)
				PrintErrorToConsole(L"GotoObj", objectiveParent, argNum, L"Distance to end waiting for obj " + std::to_wstring(objNameToWaitFor) + L" is below 1. Defaulting to " + std::to_wstring(endWaitDistance) + L".");
			else
				endWaitDistance = value;
		}

		return new ObjGotoObj(objectiveParent, targetObjNameOrId, noCruise, range, thrust, objNameToWaitFor, startWaitDistance, endWaitDistance);
	}

	static ObjGotoSpline* ReadObjGotoSpline(const ObjectiveParent& objectiveParent, INI_Reader& ini)
	{
		Vector spline[4] = { { 0, 0, 0}, { 0, 0, 0}, { 0, 0, 0}, { 0, 0, 0} };
		bool noCruise = false;
		float range = 100.0f;
		float thrust = -1.0f;
		uint objNameToWaitFor = 0;
		float startWaitDistance = 200.0f;
		float endWaitDistance = 500.0f;

		uint argNum = 0;
		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "goto_no_cruise")
				noCruise = true;
			else if (value != "goto_cruise")
				PrintErrorToConsole(L"GotoSpline", objectiveParent, argNum, L"Invalid cruise setting. Must be GOTO_CRUISE, or GOTO_NO_CRUISE. Defaulting to GOTO_CRUISE.");
		}
		argNum++;

		if (ini.get_num_parameters() > argNum + 11)
		{
			spline[0].x = ini.get_value_float(argNum);
			spline[0].y = ini.get_value_float(argNum + 1);
			spline[0].z = ini.get_value_float(argNum + 2);
			spline[1].x = ini.get_value_float(argNum + 3);
			spline[1].y = ini.get_value_float(argNum + 4);
			spline[1].z = ini.get_value_float(argNum + 5);
			spline[2].x = ini.get_value_float(argNum + 6);
			spline[2].y = ini.get_value_float(argNum + 7);
			spline[2].z = ini.get_value_float(argNum + 8);
			spline[3].x = ini.get_value_float(argNum + 9);
			spline[3].y = ini.get_value_float(argNum + 10);
			spline[3].z = ini.get_value_float(argNum + 11);
		}
		else
		{
			PrintErrorToConsole(L"GotoSpline", objectiveParent, argNum, L"No spline vectors. Aborting!");
			return nullptr;
		}
		argNum += 12;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value < 1.0f)
				PrintErrorToConsole(L"GotoSpline", objectiveParent, argNum, L"Target range below 1. Defaulting to " + std::to_wstring(range) + L".");
			else
				range = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			thrust = ini.get_value_float(argNum);
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value == 0)
				PrintErrorToConsole(L"GotoSpline", objectiveParent, argNum, L"Obj name to wait for is undefined. Defaulting to none.");
			else
				objNameToWaitFor = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value < 1.0f)
				PrintErrorToConsole(L"GotoSpline", objectiveParent, argNum, L"Distance to start waiting for obj " + std::to_wstring(objNameToWaitFor) + L" is below 1. Defaulting to " + std::to_wstring(startWaitDistance) + L".");
			else
				startWaitDistance = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value < 1.0f)
				PrintErrorToConsole(L"GotoSpline", objectiveParent, argNum, L"Distance to end waiting for obj " + std::to_wstring(objNameToWaitFor) + L" is below 1. Defaulting to " + std::to_wstring(endWaitDistance) + L".");
			else
				endWaitDistance = value;
		}

		return new ObjGotoSpline(objectiveParent, spline, noCruise, range, thrust, objNameToWaitFor, startWaitDistance, endWaitDistance);
	}

	static ObjGotoVec* ReadObjGotoVec(const ObjectiveParent& objectiveParent, INI_Reader& ini)
	{
		Vector position = { 0, 0, 0 };
		bool noCruise = false;
		float range = 100.0f;
		float thrust = -1.0f;
		uint objNameToWaitFor = 0;
		float startWaitDistance = 200.0f;
		float endWaitDistance = 500.0f;

		uint argNum = 0;
		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ToLower(ini.get_value_string(argNum));
			if (value == "goto_no_cruise")
				noCruise = true;
			else if (value != "goto_cruise")
				PrintErrorToConsole(L"GotoVec", objectiveParent, argNum, L"Invalid cruise setting. Must be GOTO_CRUISE, or GOTO_NO_CRUISE. Defaulting to GOTO_CRUISE.");
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
			PrintErrorToConsole(L"GotoVec", objectiveParent, argNum, L"No target position. Aborting!");
			return nullptr;
		}
		argNum += 3;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value < 1.0f)
				PrintErrorToConsole(L"GotoVec", objectiveParent, argNum, L"Target range below 1. Defaulting to " + std::to_wstring(range) + L".");
			else
				range = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			thrust = ini.get_value_float(argNum);
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value == 0)
				PrintErrorToConsole(L"GotoVec", objectiveParent, argNum, L"Obj name to wait for is undefined. Defaulting to none.");
			else
				objNameToWaitFor = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value < 1.0f)
				PrintErrorToConsole(L"GotoVec", objectiveParent, argNum, L"Distance to start waiting for obj " + std::to_wstring(objNameToWaitFor) + L" is below 1. Defaulting to " + std::to_wstring(startWaitDistance) + L".");
			else
				startWaitDistance = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value < 1.0f)
				PrintErrorToConsole(L"GotoVec", objectiveParent, argNum, L"Distance to end waiting for obj " + std::to_wstring(objNameToWaitFor) + L" is below 1. Defaulting to " + std::to_wstring(endWaitDistance) + L".");
			else
				endWaitDistance = value;
		}

		return new ObjGotoVec(objectiveParent, position, noCruise, range, thrust, objNameToWaitFor, startWaitDistance, endWaitDistance);
	}

	static ObjMakeNewFormation* ReadObjMakeNewFormation(const ObjectiveParent& objectiveParent, INI_Reader& ini)
	{
		uint formationId = 0;
		std::vector<uint> objNames;

		uint argNum = 0;
		formationId = CreateIdOrNull(ini.get_value_string(argNum));
		if (formationId == 0)
		{
			PrintErrorToConsole(L"MakeNewFormation", objectiveParent, argNum, L"No formation name. Aborting!");
			return nullptr;
		}
		argNum++;

		for (const auto maxArgs = ini.get_num_parameters(); argNum < maxArgs; argNum++)
		{
			const uint value = CreateIdOrNull(ini.get_value_string(argNum));
			if (value == 0)
				PrintErrorToConsole(L"MakeNewFormation", objectiveParent, argNum, L"Formation member name undefined. Ignoring.");
			else
				objNames.push_back(value);
		}

		return new ObjMakeNewFormation(objectiveParent, formationId, objNames);
	}

	static ObjSetPriority* ReadObjSetPriority(const ObjectiveParent& objectiveParent, INI_Reader& ini)
	{
		bool enforceObjectives = false;

		uint argNum = 0;
		std::string priority = ToLower(ini.get_value_string(argNum));
		if (priority == "always_execute")
			enforceObjectives = true;
		else if (priority != "normal")
			PrintErrorToConsole(L"SetPriority", objectiveParent, argNum, L"Invalid priority. Defaulting to Normal!");

		return new ObjSetPriority(objectiveParent, enforceObjectives);
	}

	static ObjStayInRange* ReadObjStayInRange(const ObjectiveParent& objectiveParent, INI_Reader& ini)
	{
		uint targetObjNameOrId = 0;
		Vector position = { 0, 0, 0 };
		float range = 100.0f;
		bool active = true;

		uint argNum = 0;
		const char* val = ini.get_value_string(argNum);
		char* end;
		strtol(val, &end, 10);
		if (end == val)
		{
			targetObjNameOrId = CreateIdOrNull(val);
			argNum++;
		}
		else if (ini.get_num_parameters() > argNum + 2)
		{
			position.x = ini.get_value_float(argNum);
			position.y = ini.get_value_float(argNum + 1);
			position.z = ini.get_value_float(argNum + 2);
			argNum += 3;
		}
		else
		{
			PrintErrorToConsole(L"StayInRange", objectiveParent, argNum, L"No target position. Aborting!");
			return nullptr;
		}

		if (ini.get_num_parameters() > argNum)
		{
			const auto& value = ini.get_value_float(argNum);
			if (value < 1.0f)
				PrintErrorToConsole(L"StayInRange", objectiveParent, argNum, L"Distance to stay in range is below 1. Defaulting to " + std::to_wstring(range) + L".");
			else
				range = value;
		}
		argNum++;

		if (ini.get_num_parameters() > argNum)
			active  = ini.get_value_bool(argNum);

		return new ObjStayInRange(objectiveParent, targetObjNameOrId, position, range, active);
	}

	Objective* TryReadObjectiveFromIni(const ObjectiveParent& objectiveParent, INI_Reader& ini)
	{
		if (ini.is_value("BreakFormation"))
			return new ObjBreakFormation(objectiveParent);

		if (ini.is_value("Delay"))
			return ReadObjDelay(objectiveParent, ini);

		if (ini.is_value("Dock"))
			return ReadObjDock(objectiveParent, ini);

		if (ini.is_value("Follow"))
			return ReadObjFollow(objectiveParent, ini);

		if (ini.is_value("GotoObj"))
			return ReadObjGotoObj(objectiveParent, ini);

		if (ini.is_value("GotoSpline"))
			return ReadObjGotoSpline(objectiveParent, ini);

		if (ini.is_value("GotoVec"))
			return ReadObjGotoVec(objectiveParent, ini);

		if (ini.is_value("Idle"))
			return new ObjIdle(objectiveParent);

		if (ini.is_value("MakeNewFormation"))
			return ReadObjMakeNewFormation(objectiveParent, ini);

		if (ini.is_value("SetLifeTime"))
			return new ObjSetLifeTime(objectiveParent, ini.get_value_float(0));

		if (ini.is_value("SetPriority"))
			return ReadObjSetPriority(objectiveParent, ini);

		if (ini.is_value("StayInRange"))
			return ReadObjStayInRange(objectiveParent, ini);

		return nullptr;
	}
}