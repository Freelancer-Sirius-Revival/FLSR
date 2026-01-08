#include "NameLimiter.h"
#include "Plugin.h"
#include <regex>

namespace NameLimiter
{
	const std::regex allowedCharacters(R"(((?![lI]{4,})[a-zA-Z0-9]|&|\(|\)|\[|\]|\{|\}|<|>|-|\.|:|=|_)+)");

	bool IsCharacterNameAllowed(const std::string& characterName)
	{
		return std::regex_match(characterName, allowedCharacters);
	}

	void __stdcall CreateNewCharacter(const SCreateCharacterInfo& scci, unsigned int clientId)
	{
		if (!IsCharacterNameAllowed(wstos(scci.wszCharname)))
		{
			Server.CharacterInfoReq(clientId, true);
			returncode = NOFUNCTIONCALL;
		}
		else
		{
			returncode = DEFAULT_RETURNCODE;
		}
	}
}