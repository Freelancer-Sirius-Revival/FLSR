#include "main.h"
#include <codecvt>
#include <regex>

namespace Discord {

	std::string scDiscordBotToken;
	std::string scDiscordServerID;
	std::string scUVChatChannelID;


	std::list<ChatMessage> lChatMessages;
	std::list <LastSelectClick> lLastSelectClick;
	std::map<std::string, DiscordUser> userDataMap;
	int iOnlinePlayers;

	bool LoadSettings()
	{
		// Konfigpfad
		char szCurDir[MAX_PATH];
		GetCurrentDirectory(sizeof(szCurDir), szCurDir);
		std::string scPluginCfgFile = std::string(szCurDir) + Globals::PLUGIN_CONFIG_FILE;

		scDiscordBotToken = IniGetS(scPluginCfgFile, "Discord", "BotToken", "");
		if (scDiscordBotToken.empty()) {
			ConPrint(L"ERROR: No Discord BotToken found in config file!\n");
			return false;
		}

		scDiscordServerID = IniGetS(scPluginCfgFile, "Discord", "ServerID", "");
		if (scDiscordServerID.empty()) {
			ConPrint(L"ERROR: No Discord ServerID found in config file!\n");
			return false;
		}

		scUVChatChannelID = IniGetS(scPluginCfgFile, "Discord", "UVChatChannelID", "");
		if (scUVChatChannelID.empty()) {
			ConPrint(L"ERROR: No Discord UVChatChannelID found in config file!\n");
			return false;
		}
		return true;
	}

	void StartUp() { //Discord Bot

		dpp::cluster DiscordBot(scDiscordBotToken);

		DiscordBot.intents = DiscordBot.intents | dpp::intents::i_guild_members;

		DiscordBot.on_log(dpp::utility::cout_logger());

		//Timers	
		dpp::timer CheckChat = DiscordBot.start_timer([&](dpp::timer timer_handle)
		{
			{
				// Mutex sperren
				std::lock_guard<std::mutex> lock(m_Mutex);

				if (!lChatMessages.empty()) {
					for (auto it = Discord::lChatMessages.begin(); it != Discord::lChatMessages.end(); ) {
						// Zugriff auf einzelne Chat-Nachricht
						ChatMessage& chatMsg = *it;
						std::string scCharname = wstring_to_utf8(chatMsg.wscCharname);
						std::string scChatMsg = wstring_to_utf8(chatMsg.wscChatMessage);

						DiscordBot.message_create(dpp::message(scUVChatChannelID, scCharname + ": " + scChatMsg));

						// Nachdem die Aktionen durchgeführt wurden, Chat-Nachricht aus der Liste löschen
						it = Discord::lChatMessages.erase(it);
					}
				}
			} // Mutex wird hier automatisch freigegeben
		}, 1);


		dpp::timer BotStatus = DiscordBot.start_timer([&](dpp::timer timer_handle) {
			
			{
			// Mutex sperren
				std::lock_guard<std::mutex> lock(m_Mutex);

				//Count players
				struct PlayerData* pPD = 0;
				int iPlayers = 0;
				while (pPD = Players.traverse_active(pPD))
					iPlayers++;

				iOnlinePlayers = iPlayers;

				//Set Status
				dpp::activity activity = dpp::activity(dpp::activity_type::at_game, "FL:SR | " + std::to_string(iPlayers) + "/" + std::to_string(Players.GetMaxPlayerCount()) + " Online", "", "");
				dpp::presence presence = dpp::presence(dpp::presence_status::ps_online, activity);
				DiscordBot.set_presence(presence);

			} // Mutex wird hier automatisch freigegeben

		}, 10);

		//On Ready
		DiscordBot.on_ready([&DiscordBot](const dpp::ready_t& event) {
			// Register global commands	
			if (dpp::run_once<struct register_bot_commands>())
			{
				//UV Chat
				dpp::slashcommand uv_cmd("uv", "Send a message to Universe Chat", DiscordBot.me.id);
				uv_cmd.add_option(dpp::command_option(dpp::co_string, "message", "Chat Message", true));
				uv_cmd.set_dm_permission(false);
				DiscordBot.global_command_create(uv_cmd);

				//ContextMenu - Serverstatus
				DiscordBot.guild_command_create(
					dpp::slashcommand()
					.set_dm_permission(true)
					.set_type(dpp::ctxm_user)
					.set_name("Serverstatus")
					.set_application_id(DiscordBot.me.id),
					scDiscordServerID
				);

				dpp::slashcommand serverstatus_cmd("serverstatus", "Show online Players", DiscordBot.me.id);
				serverstatus_cmd.set_dm_permission(true);
				DiscordBot.global_command_create(serverstatus_cmd);
			}	

			ConPrint(L"Discord Bot is Online!\n");
		});

		//On Slash Command
		DiscordBot.on_slashcommand([](const dpp::slashcommand_t& event) 
		{
			// UV Chat
			if (event.command.get_command_name() == "uv") {
				std::string sMessage = std::get<std::string>(event.get_parameter("message"));
				dpp::user DiscordUser = event.command.usr;
				dpp::guild_member ServerUser = event.command.member;

				// Regex to remove emojis
				std::regex emojiRegex(R"(<:[^:]+:\d+>)");
				sMessage = std::regex_replace(sMessage, emojiRegex, "");

				//Get Username
				std::string scUsername = (ServerUser.nickname.empty() ? DiscordUser.username : ServerUser.nickname);

				//Discord
				std::string scReplyMessage = scUsername + ": " + sMessage;
				event.reply(scReplyMessage);

				//FL
				std::wstring wscChatMessage = Utf8ToWString(sMessage);
				std::wstring wscUsername = Utf8ToWString(scUsername);
				
				{
				// Mutex sperren
					std::lock_guard<std::mutex> lock(m_Mutex);

					Chat::HkSendUChat(wscUsername, wscChatMessage);

				} // Mutex wird hier automatisch freigegeben
			}

			// Serverstatus
			if (event.command.get_command_name() == "serverstatus") {
				GetServerstatus(event);
			}
		});

		/* Use the on_user_context_menu event to look for user context menu actions */
		DiscordBot.on_user_context_menu([&](const dpp::user_context_menu_t& event) {
			if (event.command.get_command_name() == "Serverstatus") {
				GetServerstatus(event);
			}
		});


		/* When a user clicks your select menu , the on_select_click event will fire,
		 * containing the custom_id you defined in your select menu.
		 */
		DiscordBot.on_select_click([&DiscordBot](const dpp::select_click_t& event) {
			/* Select clicks are still interactions, and must be replied to in some form to
			 * prevent the "this interaction has failed" message from Discord to the user.
			 */

			//Update LastSelectData
			LastSelectClick NewSelectClick(event.command.usr, event);
			lLastSelectClick.push_back(NewSelectClick);

			event.reply();
		});

		DiscordBot.start(dpp::st_wait);
		
	} //Discord Bot END
	

	//Embeds
	template<typename T>
	void GetServerstatus(const T& event)
	{
		if (iOnlinePlayers > 0)
		{
			std::vector<std::string> charnames;
			std::vector<std::string> ships;
			std::vector<std::string> pings;

			{
				// Mutex sperren
				std::lock_guard<std::mutex> lock(m_Mutex);

				struct PlayerData* pPD = 0;

				while (pPD = Players.traverse_active(pPD))
				{
					if (!HkIsInCharSelectMenu(pPD->iOnlineID)) {
						int iRank = pPD->iRank;
						uint iShipArch = pPD->iShipArchetype;
						//Charname
						std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(pPD->iOnlineID);
						std::string scCharname = wstring_to_utf8(wscCharname);
						//Ping
						HKPLAYERINFO pi;
						HkGetPlayerInfo(wscCharname, pi, false);
						auto ping = static_cast<int>(pi.ci.dwRoundTripLatencyMS);


						charnames.push_back(scCharname);
						ships.push_back(std::to_string(iShipArch));
						pings.push_back(std::to_string(ping));
					}

				}

			} // Mutex wird hier automatisch freigegeben

			dpp::message msg("");
			dpp::embed embed = dpp::embed().
				set_title("Serverstatus").
				set_description("Online Players: " + std::to_string(iOnlinePlayers)).
				set_color(0x00FFFF);

			std::string charnamesLine;
			for (size_t i = 0; i < charnames.size(); ++i)
			{
				if (i > 0)
					charnamesLine += "\n";
				charnamesLine += charnames[i];
			}

			HkLoadStringDLLs();
			std::string shipsLine;
			for (size_t i = 0; i < ships.size(); ++i)
			{
				if (i > 0)
					shipsLine += "\n";

				Archetype::Ship* ship = Archetype::GetShip(std::stoul(ships[i]));
				std::wstring wscShipName = HkGetWStringFromIDS(ship->iIdsName).c_str();

				shipsLine += wstring_to_utf8(wscShipName);
			}

			std::string pingsLine;
			for (size_t i = 0; i < pings.size(); ++i)
			{
				if (i > 0)
					pingsLine += "\n";
				pingsLine += pings[i] + " ms";
			}

			embed.add_field("Charname", charnamesLine, true)
				.add_field("Ship", shipsLine, true)
				.add_field("Ping", pingsLine, true);

			msg.add_embed(embed);

			event.reply(msg);
		}
		else
		{
			event.reply("Currently there are no players online.");
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//Helper

	std::string wstring_to_utf8(const std::wstring& wstr) {
		std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
		return converter.to_bytes(wstr);
	}

	std::wstring Utf8ToWString(const std::string& utf8Str) {
		std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
		return converter.from_bytes(utf8Str);
	}
}
