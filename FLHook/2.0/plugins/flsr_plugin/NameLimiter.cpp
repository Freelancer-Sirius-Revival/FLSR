#include "NameLimiter.h"
#include "Plugin.h"
#include <regex>

namespace NameLimiter
{
	const std::regex allowedCharacters(R"(((?![lI]{4,})[a-zA-Z0-9]|&|\(|\)|\[|\]|\{|\}|<|>|-|\.|:|=|_|\s)+)");

	void __stdcall CreateNewCharacter(const SCreateCharacterInfo& scci, unsigned int clientId)
	{
		if (!std::regex_match(wstos(scci.wszCharname), allowedCharacters))
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