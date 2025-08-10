#include "ShipSpawning.h"
#include "NpcNames.h"
#include "../Pilots.h"

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

struct iShipTable // 1456 bytes
{
	// This is a table that contains variables when ship was being created "mostly", majority of the stuff is not read after ship is created
	HMODULE ContentConfg;
	uint iShipID;
	iRepTable* iRepTab;
	DWORD unk1[75];
	uint archetypeId;
	DWORD unk2[6];
	uint wingId;
	DWORD unk3[25];
	float lifetime;
	DWORD unk4;
	uint loadoutId;
	float currentLifetime;
	DWORD unk5;
	pub::AI::Personality personality; // offset 0x1D0, size 984bytes
	uint unk6[2];
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

std::unordered_map<uint, iShipTable*> shipTableByShipId;

void CheckDestruction()
{
	shipTableByShipId.erase(CurrentDestructediShip);

	int dummysize = StorediShipTables.size(); //Game::Entity::vectortest.size();

	for (int i = 0; i < dummysize; i++)
	{
		if (CurrentDestructediShip == StorediShipTables[i]->iShipID)
		{
			free((DWORD*)StorediShipTables[i]->unk3[10]);
			free((DWORD*)StorediShipTables[i]->unk3[1]);
			free((DWORD*)StorediShipTables[i]->unk1[72]);
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

uint wingId = UINT_MAX;

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

	iShipTab->unk1[72] = (DWORD)Padfill1;
	iShipTab->unk1[73] = (DWORD)Padfill1;
	uint archetypeId;
	pub::SpaceObj::GetArchetypeID(iShipID, archetypeId);
	iShipTab->archetypeId = archetypeId;
	iShipTab->wingId = wingId--; // WingId - all of the same wing ID get despawned once a member reaches lifetime 0.
	iShipTab->unk3[0] = 0xE8;
	iShipTab->unk3[1] = (DWORD)Padfill2;
	iShipTab->unk3[2] = 1;
	iShipTab->unk3[8] = 0; // high chance this is a "if player" check -1 points to player and plays death music in the server lmao
	iShipTab->unk3[10] = (DWORD)Padfill3;
	iShipTab->unk3[11] = 1;
	iShipTab->unk3[21] = 0xBFF00000; // -1.875f
	iShipTab->unk3[23] = 0;
	iShipTab->unk3[24] = 5;
	iShipTab->lifetime = -1; // -1 makes the NPC persistent.
	iShipTab->loadoutId = NULL; //loadout hash
	iShipTab->currentLifetime = 24; // Resets to initial lifetime when player comes back into range.
	pub::AI::get_personality(iShipID, iShipTab->personality);

	Fndpop.iSystem = iSystemID;
	Fndpop.iZone = NULL;
	Fndpop.UnknownByte = NULL;

	iShipTableCurrent = iShipTab;

	StorediShipTables.push_back(iShipTab);
	shipTableByShipId.insert({ iShipID, iShipTab });

	GetSystemInformationTable();
	CreateNewVectorEntry();

	pub::Controller::_SendMessage((int)nullptr, 0x00001010, &iShipTableCurrent);
}

/* End of Venemon's code */

void AssignToWing(const uint shipId, const uint wingLeaderShipId)
{
	const auto& foundShipEntry = shipTableByShipId.find(shipId);
	if (foundShipEntry == shipTableByShipId.end())
		return;
	
	const auto& foundLeaderEntry = shipTableByShipId.find(wingLeaderShipId);
	if (foundLeaderEntry == shipTableByShipId.end())
		return;

	foundShipEntry->second->wingId = foundLeaderEntry->second->wingId;
}

void UnassignFromWing(const uint shipId)
{
	const auto& foundShipEntry = shipTableByShipId.find(shipId);
	if (foundShipEntry != shipTableByShipId.end())
		foundShipEntry->second->wingId = wingId--;
}

void SetLifeTime(const uint shipId, const float lifeTime)
{
	const auto& foundShipEntry = shipTableByShipId.find(shipId);
	if (foundShipEntry != shipTableByShipId.end())
	{
		foundShipEntry->second->lifetime = lifeTime < 0.0f ? -1.0 : lifeTime * 15.0f; // 15 being the max decrement per second
		foundShipEntry->second->currentLifetime = lifeTime <= 0.0f ? 1.0f : lifeTime * 15.0f;
	}
}

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

static void CreatePilotName(const uint shipArchetypeId, const std::string& faction, const uint voiceId, FmtStr& output)
{
	if (faction.empty())
		return;

	const auto& ship = Archetype::GetShip(shipArchetypeId);
	bool largeShip = ship->iArchType & (ObjectType::Gunboat | ObjectType::Cruiser | ObjectType::Transport | ObjectType::Capital | ObjectType::Mining);
	
	if (largeShip)
	{
		const auto& result = NpcNames::GetRandomLargeShipName(CreateID(faction.c_str()));
		output.begin_mad_lib(16162); // "%s0 %s1 %s2"
		output.end_mad_lib();
		output.append_string(ship->iIdsName);
		output.append_string(result.first);
		output.append_string(result.second);
	}
	else if (voiceId)
	{
		output.begin_mad_lib(16163); // "%s0 %s1"
		output.end_mad_lib();
		const auto& result = NpcNames::GetRandomName(CreateID(faction.c_str()), voiceId);
		output.append_string(result.first);
		output.append_string(result.second);
	}
}

static void SetPersonality(const uint shipId, const std::string& stateGraph, const uint pilotId, const uint overrideJobId)
{
	pub::AI::SetPersonalityParams personality;
	personality.personality = overrideJobId ? Pilots::GetPilotWithJob(pilotId, overrideJobId) : Pilots::GetPilot(pilotId);
	personality.state_graph = pub::StateGraph::get_state_graph(stateGraph.c_str(), pub::StateGraph::TYPE_LEADER);
	personality.state_id = true;
	personality.contentCallback = 0;
	personality.directiveCallback = 0;
	pub::AI::SubmitState(shipId, &personality);
}

static void LaunchNpcFromObj(const uint shipId, uint launchObjId)
{
	IObjRW* inspect;
	StarSystem* starSystem;
	if (GetShipInspect(launchObjId, inspect, starSystem) && (inspect->cobj->objectClass & CObject::CSOLAR_OBJECT))
	{
		const auto solarArchetype = static_cast<Archetype::EqObj*>(inspect->cobj->archetype);
		if (solarArchetype->dockInfo.size() > 0)
		{
			pub::SpaceObj::Launch(shipId, launchObjId, 0);
			pub::AI::DirectiveLaunchOp launchOp;
			launchOp.fireWeapons = true;
			launchOp.launchFromObject = launchObjId;
			launchOp.x10 = 2;
			launchOp.x14 = 1;
			pub::AI::SubmitDirective(shipId, &launchOp);
		}
	}
}

uint CreateNPC(const NpcCreationParams& params)
{
	pub::SpaceObj::ShipInfo shipInfo;
	std::memset(&shipInfo, 0, sizeof(shipInfo));
	shipInfo.iFlag = 1;
	shipInfo.iSystem = params.systemId;
	shipInfo.iShipArchetype = params.archetypeId;
	shipInfo.vPos = params.position;
	shipInfo.mOrientation = params.orientation;
	shipInfo.iLoadout = params.loadoutId;
	shipInfo.Costume = params.costume;
	shipInfo.iPilotVoice = params.voiceId;
	shipInfo.iHitPointsLeft = params.hitpoints;
	shipInfo.iLevel = params.level;
	// Do not set the cargo descriptors for ships. They must be probably allocated in "old ways" of FL's own code - incompatible to today's "new".

	// Formation name is displayed above the pilot name in wireframe display.
	// If this is given, the ship IDS Name will be used in scanner list. Keep empty to show Pilot Name in scanner list.
	FmtStr formationName(0, 0);
	formationName.begin_mad_lib(0);
	formationName.end_mad_lib();

	// Pilot name to be displayed when clicking on the ship/wireframe display.
	// Will be also displayed in scanner list if no formation name is given.
	FmtStr pilotName(0, 0);
	if (params.idsName)
	{
		pilotName.begin_mad_lib(params.idsName);
		pilotName.end_mad_lib();
	}
	else
		CreatePilotName(params.archetypeId, params.faction, params.voiceId, pilotName);

	pub::Reputation::Alloc(shipInfo.iRep, formationName, pilotName);
	uint groupId;
	pub::Reputation::GetReputationGroup(groupId, params.faction.c_str());
	pub::Reputation::SetAffiliation(shipInfo.iRep, groupId);

	uint objId = TryCreateNpc(shipInfo);
	if (objId == 0)
		return objId;

	SetPersonality(objId, params.stateGraphName, params.pilotId, params.pilotJobId);
	CreatePopulationEntry(objId, shipInfo.iRep, shipInfo.iSystem);

	pub::AI::DirectiveCancelOp cancelOp;
	pub::AI::SubmitDirective(objId, &cancelOp);

	pub::AI::update_formation_state(objId, objId, { 0, 0, 0 });

	if (params.launchObjId)
		LaunchNpcFromObj(objId, params.launchObjId);

	return objId;
}