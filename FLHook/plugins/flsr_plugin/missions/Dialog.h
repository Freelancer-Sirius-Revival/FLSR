#pragma once
#include <FLHook.h>

namespace Missions
{
	struct DialogEtherSender
	{
		uint id;
		uint voiceId = 0;
		uint idsName = 0;
		Costume costume;
	};

	struct DialogLine
	{
		uint id;
		uint senderEtherSenderOrObjName = 0;
		uint receiverObjNameOrLabel = 0;
		std::vector<uint> lines;
		float delay = 0.5f;
		bool global = false;
	};

	struct Dialog
	{
		uint id;
		std::unordered_map<uint, DialogEtherSender> etherSenders;
		std::vector<DialogLine> lines;
	};
}