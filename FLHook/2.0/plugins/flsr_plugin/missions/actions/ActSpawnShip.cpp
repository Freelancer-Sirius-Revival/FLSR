#include "ActSpawnShip.h"
#include "../../Pilots.h"
#include "../NpcNames.h"
#include "../Objectives/Objectives.h"

/* Code made by Venemon to register NPCs properly to the system. */

DWORD dummy;

#define ProtectX(addr, size) VirtualProtect(addr, size, PAGE_EXECUTE_READWRITE, &dummy)

#define JUMP(from, to) \
	*from = 0xE9;      \
	*(DWORD*)(from + 1) = (PBYTE)to - from - 5

#define INDIRECT(from, to)        \
	to##_Old = **(PDWORD*)(from); \
	to##_New = (DWORD)to##_Hook;  \
	*(PDWORD*)(from) = &to##_New 

bool init = 0;
HMODULE ContentAddress;
HMODULE MacroAddress1;
HMODULE MacroAddress2;
HMODULE iShipTableConfg;
HMODULE iRepTableConfg;
HMODULE JumpAdress1;
HMODULE JumpAdress2;

struct FindPopulation
{
	uint iSystem;         // Must, crashes if invalid or null
	uint iZone;
	uint UnknownByte = 0;
};

struct iRepTable
{
	HMODULE ContentConfg;
	int EntityPhase; // 3 is being created, 2 is on launch, 1 is uh?
	int iRep;
};

struct iShipTable
{
	// This is a table that contains variables when ship was being created "mostly", majority of the stuff is not read after ship is created
	HMODULE ContentConfg;
	uint iShipID;
	iRepTable* iRepTab;
	DWORD UnknownData[512];
};

struct EmptyPadding
{
	int pad1;
	int pad2;
	int pad3;
	int pad4;
	int pad5;
	int pad6;
	int pad7;
};

FindPopulation Fndpop;
DWORD ResultTable;
iShipTable* iShipTableCurrent;
uint CurrentDestructediShip;
uint OurDestruction = 0;
std::vector<iShipTable*> StorediShipTables;

// Each system, uniquely, has its own information table, it contains amount of npcs and other related stuff
__declspec(naked) void GetSystemInformationTable()
{
	__asm
	{
		pushad						//preserve stack
		lea ecx, Fndpop				//load our struct 
		push ecx					//push into stack
		mov eax, ContentAddress		//get content.dll address
		add eax, 0xD5F30			//function where we go
		call eax					//call "Get vector" adress
		mov ResultTable, eax		//store the result in here
		pop eax						//pop up stack useless info result
		popad						//remove preserve
		ret							//return
	}
}

// We find where the population vector is stored in the Information table, and create a new entry through vector-estique function
__declspec(naked) void CreateNewVectorEntry()
{
	__asm
	{
		pushad							//preserve stack	

		mov ebp, ResultTable			//get result from previous call
		mov edi, [ebp + 0xE0]				//vanilla code majority (copied freelancer assembly to not re-hook sections)
		mov eax, [edi + 0x04]
		lea esi, [ebp + 0xDC]
		push eax
		push edi
		mov ecx, esi
		mov edx, ContentAddress
		add edx, 0xF3790
		call edx						//result will be EAX, points to new slot

		mov[edi + 0x04], eax
		mov ecx, [eax + 0x04]
		mov[ecx], eax
		lea ecx, iShipTableCurrent
		push ecx
		add eax, 0x08
		push eax
		mov edx, ContentAddress
		add edx, 0x16740
		call edx

		mov ebp, [esi + 0x08]
		inc ebp							//vanilla style increment vector size
		mov[esi + 0x08], ebp

		pop edx
		pop edx
		popad

		ret
	}
}

void CheckDestruction()
{
	int dummysize = StorediShipTables.size(); //Game::Entity::vectortest.size();

	for (int i = 0; i < dummysize; i++)
	{
		if (CurrentDestructediShip == StorediShipTables[i]->iShipID)
		{
			free((DWORD*)StorediShipTables[i]->UnknownData[93]);
			free((DWORD*)StorediShipTables[i]->UnknownData[84]);
			free((DWORD*)StorediShipTables[i]->UnknownData[72]);
			free(StorediShipTables[i]->iRepTab);
			free(StorediShipTables[i]);

			StorediShipTables.erase(StorediShipTables.begin() + i);
			OurDestruction = 1;
			break;
		}
	}
}

//Freelancer uses ancient functions, and "free"ing a different module heap causes access violation, we will manually free our own heap from here
__declspec(naked) void DestructionCheck()
{
	__asm
	{
		je Skip

		mov eax, [ecx + 0x4]
		mov CurrentDestructediShip, eax
		mov OurDestruction, 0x0

		pushad
		call CheckDestruction
		popad

		cmp OurDestruction, 0x1
		je Skip

		mov eax, [ecx]
			push 01
				jmp JumpAdress1


				Skip :
			jmp JumpAdress2
	}
}

//after a ship is being created, game assigns them into so called "population vectors" inside System Tables, they manage voices/loots and etc
void CreatePopulationEntry(uint iShipID, uint iRep, uint iSystemID)
{
	if (!init)
	{
		ContentAddress = GetModuleHandle("content.dll");
		MacroAddress1 = ContentAddress;
		MacroAddress2 = ContentAddress;
		iShipTableConfg = ContentAddress;
		iRepTableConfg = ContentAddress;
		JumpAdress1 = ContentAddress;
		JumpAdress2 = ContentAddress;
		__asm add MacroAddress1, 0xD4344;
		__asm add MacroAddress2, 0xD4349;
		__asm add iShipTableConfg, 0x1195A0;
		__asm add iRepTableConfg, 0x11940C;
		__asm add JumpAdress1, 0xD434A;
		__asm add JumpAdress2, 0xD434C;

		#define HookDestructor ((PBYTE)(MacroAddress1))
		#define HookDestructor2 ((PBYTE)(MacroAddress2))
		ProtectX(HookDestructor, 6);
		memcpy(HookDestructor2, "\x90", 1);
		JUMP(HookDestructor, DestructionCheck);

		init = true;
	}

	iShipTable* iShipTab = (iShipTable*)malloc(sizeof(iShipTable));
	iRepTable* iRepTab = (iRepTable*)malloc(sizeof(iRepTable));
	EmptyPadding* Padfill1 = (EmptyPadding*)malloc(sizeof(EmptyPadding));
	EmptyPadding* Padfill2 = (EmptyPadding*)malloc(sizeof(EmptyPadding));
	EmptyPadding* Padfill3 = (EmptyPadding*)malloc(sizeof(EmptyPadding));

	Padfill2->pad1 = (int)Padfill2;
	Padfill2->pad2 = (int)Padfill2;
	Padfill3->pad1 = (int)Padfill3;
	Padfill3->pad2 = (int)Padfill3;

	ZeroMemory(iShipTab, sizeof(iShipTable));

	iShipTab->ContentConfg = iShipTableConfg;
	iShipTab->iShipID = iShipID;
	iShipTab->iRepTab = iRepTab;
	iRepTab->ContentConfg = iRepTableConfg;
	iRepTab->EntityPhase = 0x2;
	iRepTab->iRep = iRep;

	iShipTab->UnknownData[72] = (DWORD)Padfill1;
	iShipTab->UnknownData[73] = (DWORD)Padfill1;
	iShipTab->UnknownData[75] = 1;
	iShipTab->UnknownData[83] = 0xE8;
	iShipTab->UnknownData[84] = (DWORD)Padfill2;
	iShipTab->UnknownData[85] = 1;
	iShipTab->UnknownData[91] = 0; // high chance this is a "if player" check -1 points to player and plays death music in the server lmao
	iShipTab->UnknownData[93] = (DWORD)Padfill3;
	iShipTab->UnknownData[94] = 0xFFFFFFFF;
	iShipTab->UnknownData[104] = 0x3ff00000;
	iShipTab->UnknownData[106] = 1;
	iShipTab->UnknownData[107] = 5;
	iShipTab->UnknownData[108] = 0xbf800000; // Life time drain, all important npcs use -1 so they dont get destroyed by the population manager
	iShipTab->UnknownData[109] = 0x43fa0000; // possible life drain area
	iShipTab->UnknownData[110] = NULL; //loadout hash
	iShipTab->UnknownData[111] = 0x41c00000; // life time duration

	Fndpop.iSystem = iSystemID;
	Fndpop.iZone = NULL;
	Fndpop.UnknownByte = NULL;

	iShipTableCurrent = iShipTab;

	StorediShipTables.push_back(iShipTab);

	GetSystemInformationTable();
	CreateNewVectorEntry();

	pub::Controller::_SendMessage((int)nullptr, 0x00001010, &iShipTableCurrent);
}

namespace Missions
{
	static uint TryCreateNpc(const pub::SpaceObj::ShipInfo& shipInfo)
	{
		__try
		{
			uint objId;
			pub::SpaceObj::Create(objId, shipInfo);
			return objId;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return 0;
		}
	}

	static uint CreateNPC(const ActSpawnShip& action, const MsnNpc& msnNpc, const Npc& npc)
	{
		pub::SpaceObj::ShipInfo shipInfo;
		std::memset(&shipInfo, 0, sizeof(shipInfo));
		shipInfo.iFlag = 1;
		shipInfo.iSystem = msnNpc.systemId;
		shipInfo.iShipArchetype = npc.archetypeId;
		shipInfo.vPos = action.position.x != std::numeric_limits<float>::infinity() ? action.position : msnNpc.position;
		shipInfo.mOrientation = action.orientation.data[0][0] != std::numeric_limits<float>::infinity() ? action.orientation : msnNpc.orientation;
		shipInfo.iLoadout = npc.loadoutId;
		shipInfo.Costume = npc.costume;
		shipInfo.iPilotVoice = npc.voiceId;
		shipInfo.iHitPointsLeft = msnNpc.hitpoints;
		shipInfo.iLevel = npc.level;
		// Do not set the cargo descriptors for ships. They must be probably allocated in "old ways" of FL's own code - incompatible to today's "new".

		// Formation name is displayed above the pilot name in wireframe display.
		// If this is given, the ship IDS Name will be used in scanner list. Keep empty to show Pilot Name in scanner list.
		FmtStr formationName(0, 0);
		formationName.begin_mad_lib(0);
		formationName.end_mad_lib();

		// Pilot name to be displayed when clicking on the ship/wireframe display.
		// Will be also displayed in scanner list if no formation name is given.
		FmtStr pilotName(0, 0);
		pilotName.begin_mad_lib(0);
		pilotName.end_mad_lib();

		if (msnNpc.idsName)
		{
			pilotName.begin_mad_lib(msnNpc.idsName);
			pilotName.end_mad_lib();
		}
		else
		{
			const auto& ship = Archetype::GetShip(npc.archetypeId);
			bool largeShip = ship->iArchType & (ObjectType::Gunboat | ObjectType::Cruiser | ObjectType::Transport | ObjectType::Capital | ObjectType::Mining);

			if (!npc.faction.empty())
			{
				if (largeShip)
				{
					const auto& result = NpcNames::GetRandomLargeShipName(CreateID(npc.faction.c_str()));
					pilotName.begin_mad_lib(16162); // "%s0 %s1 %s2"
					pilotName.end_mad_lib();
					pilotName.append_string(ship->iIdsName);
					pilotName.append_string(result.first);
					pilotName.append_string(result.second);
				}
				else if (npc.voiceId)
				{
					pilotName.begin_mad_lib(16163); // "%s0 %s1"
					pilotName.end_mad_lib();
					const auto& result = NpcNames::GetRandomName(CreateID(npc.faction.c_str()), npc.voiceId);
					pilotName.append_string(result.first);
					pilotName.append_string(result.second);
				}
			}
		}

		pub::Reputation::Alloc(shipInfo.iRep, formationName, pilotName);
		uint groupId;
		pub::Reputation::GetReputationGroup(groupId, npc.faction.c_str());
		pub::Reputation::SetAffiliation(shipInfo.iRep, groupId);

		uint objId = TryCreateNpc(shipInfo);
		if (objId == 0)
		{
			ConPrint(L"ERROR: MSN NPC " + std::to_wstring(msnNpc.id) + L" in system " + std::to_wstring(msnNpc.systemId) + L" at position " + std::to_wstring(msnNpc.position.x) + L", " + std::to_wstring(msnNpc.position.y) + L", " + std::to_wstring(msnNpc.position.z) + L"\n");
			return 0;
		}

		pub::AI::SetPersonalityParams personality;
		personality.personality = msnNpc.pilotJobId ? Pilots::GetPilotWithJob(npc.pilotId, msnNpc.pilotJobId) : Pilots::GetPilot(npc.pilotId);
		personality.state_graph = pub::StateGraph::get_state_graph(npc.stateGraph.c_str(), pub::StateGraph::TYPE_STANDARD);
		personality.state_id = true;
		personality.contentCallback = 0;
		personality.directiveCallback = 0;
		pub::AI::SubmitState(objId, &personality);

		CreatePopulationEntry(objId, shipInfo.iRep, shipInfo.iSystem);

		if (msnNpc.startingObjId)
		{
			uint launchObjId = msnNpc.startingObjId;
			IObjRW* inspect;
			StarSystem* starSystem;
			if (GetShipInspect(launchObjId, inspect, starSystem) && (inspect->cobj->objectClass & CObject::CSOLAR_OBJECT) == CObject::CSOLAR_OBJECT)
			{
				const auto solarArchetype = static_cast<Archetype::EqObj*>(inspect->cobj->archetype);
				if (solarArchetype->dockInfo.size() > 0)
					pub::SpaceObj::Launch(objId, msnNpc.startingObjId, 0);
			}
		}
		return objId;
	}

	void ActSpawnShip::Execute(Mission& mission, const MissionObject& activator) const
	{
		if (mission.objectIdsByName.contains(msnNpcId))
			return;

		const auto& msnNpcEntry = mission.msnNpcs.find(msnNpcId);
		if (msnNpcEntry == mission.msnNpcs.end())
		{
			ConPrint(L"ERROR: MSN NPC " + std::to_wstring(msnNpcId) + L" not found.\n");
			return;
		}

		const auto& npcEntry = mission.npcs.find(msnNpcEntry->second.npcId);
		if (npcEntry == mission.npcs.end())
		{
			ConPrint(L"ERROR: NPC " + std::to_wstring(msnNpcId) + L" not found.\n");
			return;
		}

		const uint objId = CreateNPC(*this, msnNpcEntry->second, npcEntry->second);
		if (objId)
		{
			mission.AddObject(objId, msnNpcId, msnNpcEntry->second.labels);

			if (msnNpcEntry->second.startingObjId)
			{
				const auto& foundObjectEntry = mission.objectIdsByName.find(msnNpcEntry->second.startingObjId);
				uint launchObjId = foundObjectEntry != mission.objectIdsByName.end() ? foundObjectEntry->second : msnNpcEntry->second.startingObjId;
				IObjRW* inspect;
				StarSystem* starSystem;
				if (GetShipInspect(launchObjId, inspect, starSystem) && (inspect->cobj->objectClass & CObject::CSOLAR_OBJECT) == CObject::CSOLAR_OBJECT)
				{
					const auto solarArchetype = static_cast<Archetype::EqObj*>(inspect->cobj->archetype);
					if (solarArchetype->dockInfo.size() > 0)
						pub::SpaceObj::Launch(objId, launchObjId, 0);
				}
			}

			if (const auto& objectivesEntry = mission.objectives.find(objectivesId); objectivesEntry != mission.objectives.end())
			{
				mission.objectivesByObjectId.try_emplace(objId, mission.id, objId, objectivesEntry->second.objectives);
				mission.objectivesByObjectId.at(objId).Progress();
			}
		}
	}
}