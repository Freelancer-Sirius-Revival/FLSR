#include "main.h"
#include <codecvt>
#include <regex>
#include <random>
#include <unordered_set>

namespace Discord {

	std::string scDiscordBotToken;
	std::string scDiscordServerID;
	std::string scUVChatChannelID;
	std::string scModRequestChannelID;
	std::string scModGroupID;
	std::string scNewsChannelID;
	std::string scEventChannelID;

	int iRenameCost;


	std::list<ChatMessage> lChatMessages;
	std::list<ChatMessage> lModMessages;
	std::list<DMMessage> lDMMessages;
	std::list<MessageListEntry> lNewsList;
	std::list<MessageListEntry> lEventList;
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

		scModRequestChannelID = IniGetS(scPluginCfgFile, "Discord", "ModRequestChannelID", "");
		if (scModRequestChannelID.empty()) {
			ConPrint(L"ERROR: No Discord ModRequestChannelID found in config file!\n");
			return false;
		}

		scModGroupID = IniGetS(scPluginCfgFile, "Discord", "ModGroupID", "");
		if (scModGroupID.empty()) {
			ConPrint(L"ERROR: No Discord ModGroupID found in config file!\n");
			return false;
		}

		scNewsChannelID = IniGetS(scPluginCfgFile, "Discord", "NewsChannelID", "");
		if (scNewsChannelID.empty()) {
			ConPrint(L"ERROR: No Discord NewsChannelID found in config file!\n");
			return false;
		}

		scEventChannelID = IniGetS(scPluginCfgFile, "Discord", "EventChannelID", "");
		if (scEventChannelID.empty()) {
			ConPrint(L"ERROR: No Discord EventChannelID found in config file!\n");
			return false;
		}

		iRenameCost = IniGetI(scPluginCfgFile, "Discord", "RenameCost", 0);

		return true;
	}

	void StartUp() { //Discord Bot

		//Start Bot
		dpp::cluster DiscordBot(scDiscordBotToken);

		DiscordBot.intents = DiscordBot.intents | dpp::intents::i_guild_members;

		DiscordBot.on_log(dpp::utility::cout_logger());

		//Timers	
		dpp::timer CheckChat = DiscordBot.start_timer([&](dpp::timer timer_handle) {

			{
				// Mutex sperren
				std::lock_guard<std::mutex> lock(m_Mutex);

				// Überprüfen, ob es neue Chat-Nachrichten gibt
				if (!lChatMessages.empty()) {
					// Neue Chat-Nachrichten vorhanden

					// Schleife über die Chat-Nachrichten
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

				// Überprüfen, ob es neue Mod-Nachrichten gibt
				if (!lModMessages.empty()) {
					// Neue Chat-Nachrichten vorhanden

					// Schleife über die Chat-Nachrichten
					for (auto it = Discord::lModMessages.begin(); it != Discord::lModMessages.end(); ) {
						// Zugriff auf einzelne Chat-Nachricht
						ChatMessage& chatMsg = *it;
						std::string scCharname = wstring_to_utf8(chatMsg.wscCharname);
						std::string scChatMsg = wstring_to_utf8(chatMsg.wscChatMessage);
						DiscordBot.message_create(dpp::message(scModRequestChannelID, "<@&"+ scModGroupID + "> " + scCharname + ": " + scChatMsg));

						// Nachdem die Aktionen durchgeführt wurden, Chat-Nachricht aus der Liste löschen
						it = Discord::lModMessages.erase(it);
					}
				}

				// Überprüfen, ob es neue DM-Nachrichten gibt
				if (!lDMMessages.empty()) {
					// Neue Nachrichten vorhanden

					// Schleife über die DM-Nachrichten
					for (auto it = Discord::lDMMessages.begin(); it != Discord::lDMMessages.end(); ) {
						// Zugriff auf einzelne Nachricht

						// Send DM
						DiscordBot.direct_message_create(it->DiscordUserID, it->DiscordMessage);

						// Nachdem die Aktionen durchgeführt wurden, Dm-Nachricht aus der Liste löschen
						it = Discord::lDMMessages.erase(it);
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

		//1h Timer for News/Events
		dpp::timer NewsTimer = DiscordBot.start_timer([&](dpp::timer timer_handle) {

			//Update Events & News
			{
				ConPrint(L"Fetching News/Events ");
				// Mutex sperren
				std::lock_guard<std::mutex> lock(m_Mutex);

				Update_NewsList(DiscordBot);
				Update_EventList(DiscordBot);

				ConPrint(L"- done\n");

			} // Mutex wird hier automatisch freigegeben

		}, 3600);

		//On Ready
		DiscordBot.on_ready([&DiscordBot](const dpp::ready_t& event) {
			// Register global commands	
			if (dpp::run_once<struct register_bot_commands>())
			{
				//Ping
				dpp::slashcommand ping_cmd("ping", "Display average ping", DiscordBot.me.id);
				ping_cmd.set_dm_permission(true);
				DiscordBot.global_command_create(ping_cmd);

				//Uptime
				dpp::slashcommand uptime_cmd("uptime", "Display Server Uptime", DiscordBot.me.id);
				uptime_cmd.set_dm_permission(true);
				DiscordBot.global_command_create(uptime_cmd);

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

				//Serverstatus
				dpp::slashcommand serverstatus_cmd("serverstatus", "Show online Players", DiscordBot.me.id);
				serverstatus_cmd.set_dm_permission(true);
				DiscordBot.global_command_create(serverstatus_cmd);

				//ContextMenu - Link Char
				DiscordBot.guild_command_create(
					dpp::slashcommand()
					.set_dm_permission(true)
					.set_type(dpp::ctxm_user)
					.set_name("Link Character")
					.set_application_id(DiscordBot.me.id),
					scDiscordServerID
				);

				//Link Char
				dpp::slashcommand link_cmd("link", "Link your in-game id with your Discord account", DiscordBot.me.id);
				link_cmd.set_dm_permission(true);
				DiscordBot.global_command_create(link_cmd);

				//ContextMenu - Rename Char
				DiscordBot.guild_command_create(
					dpp::slashcommand()
					.set_dm_permission(true)
					.set_type(dpp::ctxm_user)
					.set_name("Rename Character")
					.set_application_id(DiscordBot.me.id),
					scDiscordServerID
				);

				//Rename Char
				std::string scRenameDesc;
				if (iRenameCost == 0) {
					scRenameDesc = "Rename your Character";
				}
				else {
					scRenameDesc = "Rename your Character, " + std::to_string(iRenameCost) + " Credits";
				}

				dpp::slashcommand rename_cmd("rename", scRenameDesc, DiscordBot.me.id);
				rename_cmd.set_dm_permission(true);
				DiscordBot.global_command_create(rename_cmd);

				//ContextMenu - Bank
				DiscordBot.guild_command_create(
					dpp::slashcommand()
					.set_dm_permission(true)
					.set_type(dpp::ctxm_user)
					.set_name("Bank")
					.set_application_id(DiscordBot.me.id),
					scDiscordServerID
				);

				//Bank
				dpp::slashcommand bank_cmd("bank", "Manage your Discord-Bank", DiscordBot.me.id);
				bank_cmd.set_dm_permission(true);
				DiscordBot.global_command_create(bank_cmd);
			}	


			//Update Events & News
			{
				ConPrint(L"Fetching News/Events ");
				// Mutex sperren
				std::lock_guard<std::mutex> lock(m_Mutex);

				Update_NewsList(DiscordBot);
				Update_EventList(DiscordBot);

				ConPrint(L"- done\n");

			} // Mutex wird hier automatisch freigegeben

			ConPrint(L"Discord Bot is Online!\n");
		});

		//On Slash Command
		DiscordBot.on_slashcommand([](const dpp::slashcommand_t& event) 
		{
			// Ping
			if (event.command.get_command_name() == "ping") {
				float averagePing = Tools::GetAveragePingOfAllPlayers();
				int roundedPing = static_cast<int>(averagePing);

				std::string pingString = std::to_string(roundedPing);
				pingString = pingString.substr(0, pingString.find('.'));

				std::string reply = "Average Ping of all players: " + pingString + " ms";
				event.reply(reply);
			}

			// Uptime
			if (event.command.get_command_name() == "uptime") {
				CommandUptime(event);
			}

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



			//Link Modal
			if (event.command.get_command_name() == "link") {

				//Show Link Modal
				LinkModal(event);

			}

			//Rename Modal
			if (event.command.get_command_name() == "rename") {

				//Show Rename Modal
				CharRenameModal(event);

			}

			//Bank Embed
			if (event.command.get_command_name() == "bank") {
				
				//Show Bank Embed
				BankEmbed(event);

			}

		});

		/* Use the on_user_context_menu event to look for user context menu actions */
		DiscordBot.on_user_context_menu([&](const dpp::user_context_menu_t& event) {
			if (event.command.get_command_name() == "Link Character") {		
				//Show Link Modal
				LinkModal(event);
			}
			if (event.command.get_command_name() == "Rename Character") {
				//Show Link Modal
				CharRenameModal(event);
			}
			if (event.command.get_command_name() == "Bank") {

				//Show Bank Embed
				BankEmbed(event);

			}
			if (event.command.get_command_name() == "Serverstatus") {
				GetServerstatus(event);
			}

		});


		DiscordBot.on_button_click([&](const dpp::button_click_t& event) {

			if (event.custom_id == "transfer_to_discord")
			{

				BankTransferModal(event, "BankTransferDiscordAccount");
			}
			if (event.custom_id == "transfer_to_char")
			{

				BankTransferModal(event, "BankTransferIngameChar");

			}


			ConPrint(L"on_form_submit: " + stows(event.custom_id) + L"\n");

		});

		/* This event handles form submission for the modal dialog we create above */
		DiscordBot.on_form_submit([&](const dpp::form_submit_t& event) {

			//Check if modal is Charlinking Modal
			if (event.custom_id == "link_modal_p1") {
				//Vars
				std::string scCharname;

				//Get Charname
				if (event.components[0].components[0].custom_id == "link_modal_p1_charname")
				{
					scCharname = std::get<std::string>(event.components[0].components[0].value);
				}

				//Show new Modal with Charname
				if (!scCharname.empty())
				{
					// Generate password
					std::string password = GeneratePassword(); // Replace with your password generation logic

					// Send DM to the user
					dpp::guild_member ServerUser = event.command.member;
					dpp::snowflake user_id = ServerUser.user_id;

					std::wstring wscFile;
					{
					// Mutex sperren
						std::lock_guard<std::mutex> lock(m_Mutex);

						//CharFile
						HkGetCharFileName(Utf8ToWString(scCharname), wscFile);

					} // Mutex wird hier automatisch freigegeben

					//Check & Insert
					std::string screturn = CharManager_Insert(wstos(wscFile), std::to_string(user_id), Tools::sha1(password));
					if (screturn == "OK")
					{
						// Compose DM message
						std::string message_content = "The character " + scCharname + " has been marked for linking.\n";
						message_content += "Log in with the character and enter the following password using /link " + password + " command.";
						dpp::message dm;
						dm.content = message_content;

						// Send DM
						DiscordBot.direct_message_create(user_id, dm);
						event.reply("");

					}
					else {
						// Compose DM message
						std::string message_content = screturn;
						dpp::message dm;
						dm.content = message_content;

						// Send DM
						DiscordBot.direct_message_create(user_id, dm);
						event.reply("");
					}


				}

			}

			//Check if modal is Rename Modal
			if (event.custom_id == "rename_modal_p1") {
				//Vars
				std::string scCharnameOld;
				std::string scCharnameNew;

				//Get Charname
				scCharnameOld = GetFormComponentValue(event, "rename_modal_charname_old");
				scCharnameNew = GetFormComponentValue(event, "rename_modal_charname_new");

				if (containsWhitespace(scCharnameOld) || containsWhitespace(scCharnameNew))
				{
					dpp::message dm;
					dm.content = "The new character name must not contain any spaces!";
					DiscordBot.direct_message_create(event.command.usr.id, dm);
					event.reply();
					return;
				}

				
				if (scCharnameOld == "" || scCharnameNew == "")
					return;

				//WString
				std::wstring wscCharname = Utf8ToWString(scCharnameOld);
				std::wstring wscNewCharname = Utf8ToWString(scCharnameNew);
				//DiscordBot.direct_message_create(event.command.usr.id, "Old: " + scCharnameOld + " New: " + scCharnameNew);

				//Get DiscordUser
				dpp::guild_member ServerUser = event.command.member;
				dpp::snowflake user_id = ServerUser.user_id;

				std::wstring wscoldFile;
				std::wstring wscnewFile;

				{
					// Mutex sperren
					std::lock_guard<std::mutex> lock(m_Mutex);


					//Get old Charfile
					HkGetCharFileName(wscCharname, wscoldFile);

					//Get new Charfile
					HkGetCharFileName(wscNewCharname, wscnewFile);

				} // Mutex wird hier automatisch freigegeben

				//Test if Charname is linked
				if (!IsCharacterLinkedWithDiscordID(std::to_string(user_id), wstos(wscoldFile)))
				{
					std::string errorMessage = "Sorry, you can only rename characters that are linked to your Discord account.\n\nPlease check your input or link the character to your Discord account.\n\nCharacter to be renamed: " + scCharnameOld;
					DiscordBot.direct_message_create(event.command.usr.id, errorMessage);
					event.reply();
					return;
				}

				HK_ERROR renameResult;

				//Check if new Charname has enough Money
				int iCash;
				HkGetCash(wscCharname, iCash);
				if (iCash < iRenameCost)
				{
					int requiredCash = iRenameCost - iCash;
					std::string errorMessage = "Sorry, you don't have enough money to rename your character.\n\n"
						"You need an additional " + std::to_string(requiredCash) + " credits.\n"
						"The total cost to rename your character is " + wstos(ToMoneyStr(iRenameCost)) + " credits.\n\n"
						"Character to be renamed: " + scCharnameOld;
					DiscordBot.direct_message_create(event.command.usr.id, errorMessage);
					event.reply();
					return;
				}

				{
					// Mutex sperren
					std::lock_guard<std::mutex> lock(m_Mutex);

					//Calc new Cash

					// The last error.
					HK_ERROR err;

					if ((err = HkAddCash(wscCharname, -iRenameCost)) != HKE_OK) {
						std::string errorMessage = "Sorry, an error occurred while renaming your character.\n\n"
							"Please try again later.\n\n";

						DiscordBot.direct_message_create(event.command.usr.id, errorMessage);
						event.reply();

						return;
					}


					//FLHook Rename
					renameResult = HkRename(wscCharname, wscNewCharname, false);

				} // Mutex wird hier automatisch freigegeben



				if (renameResult == HKE_OK)
				{
					//Update Database
					bool databaseUpdateResult = CharManager_Rename(wstos(wscoldFile), wstos(wscnewFile));
					bool databaseUpdateResult2 = CharManager_UpdateCharname(wstos(wscnewFile), wstos(wscNewCharname));

					if (databaseUpdateResult && databaseUpdateResult2)
					{
						
						std::string successMessage = "Character renamed successfully!";
						DiscordBot.direct_message_create(event.command.usr.id, successMessage);
					}
					else
					{
						std::string errorMessage = "Error: Failed to update the database.";
						DiscordBot.direct_message_create(event.command.usr.id, errorMessage);
					}
				}
				else
				{
					std::string errorMessage;
					if (renameResult == HKE_CHAR_DOES_NOT_EXIST)
						errorMessage = "Error: Character does not exist.";
					else if (renameResult == HKE_CHARNAME_ALREADY_EXISTS)
						errorMessage = "Error: The new character name already exists.";
					else if (renameResult == HKE_CHARNAME_TOO_LONG)
						errorMessage = "Error: The new character name is too long.";
					else if (renameResult == HKE_CHARNAME_TOO_SHORT)
						errorMessage = "Error: The new character name is too short.";
					else if (renameResult == HKE_MPNEWCHARACTERFILE_NOT_FOUND_OR_INVALID)
						errorMessage = "Error: MPNEWCHARACTERFILE not found or invalid.";
					else if (renameResult == HKE_COULD_NOT_ENCODE_CHARFILE)
						errorMessage = "Error: Could not encode the character file.";
					else
						errorMessage = "Error: An unknown error occurred during character renaming.";

					DiscordBot.direct_message_create(event.command.usr.id, errorMessage);
				}


				event.reply();
			}

			//Check if Modal is Bank Transfer Modal
			if (event.custom_id == "bank_transfer_modal") {

				//Vars
				bool isDiscordToDiscordTransfer;
				std::string scTarget;
				std::string scAmount;
				std::string userID;

				//Get Data from Modal
				if ((scTarget = GetFormComponentValue(event, "BankTransferDiscordAccountrec")) != "")
				{
					isDiscordToDiscordTransfer = true;
					scAmount = GetFormComponentValue(event, "BankTransferDiscordAccountamount");
				}
				else
				{
					isDiscordToDiscordTransfer = false;
					scTarget = GetFormComponentValue(event, "BankTransferIngameCharrec");
					scAmount = GetFormComponentValue(event, "BankTransferIngameCharamount");
				}

				if (isDiscordToDiscordTransfer)
				{
					userID = GetUserIDByDiscordName(scTarget);
					//No Discord Account found
					if (userID.empty())
					{
						event.reply("Discord Account not found!");
						return;
					}

					//Has Discord Account linked Chars
					if (!DoesDiscordAccountHaveValidChars(userID))
					{
						event.reply("Discord Account has no linked characters!");
						return;
					}

					//Check if User want to transfer to himself
					if (event.command.usr.id == userID)
					{
						event.reply("You can't transfer money to yourself!");
						return;
					}
				}


				//Check if User has enough money
				std::string AccountCash = GetCreditsForDiscordAccount(std::to_string(event.command.usr.id));
				int iAccountCash = std::stoi(AccountCash);
				if (scAmount.empty())
				{
					event.reply("Please enter an amount!");
					return;
				}

				//Check if Amount is numeric
				bool isNumeric = true;
				for (char c : scAmount)
				{
					if (!std::isdigit(c))
					{
						isNumeric = false;
						break;
					}
				}
				if (!isNumeric)
				{
					event.reply("Please enter a valid amount!");
					return;
				}
				int iAmount = std::stoi(scAmount);

				//Chcek if Amount is positive and not 0
				if (iAmount <= 0)
				{
					event.reply("Please enter a valid amount!");
					return;
				}

				//Check if User has enough money
				if (iAccountCash < iAmount)
				{
					event.reply("You don't have enough money!");
					return;
				}
				
				if (isDiscordToDiscordTransfer)
				{
					if (UpdateCreditsForDiscordAccount(std::to_string(event.command.usr.id), scAmount, false))
					{
						if (UpdateCreditsForDiscordAccount(userID, scAmount, true))
						{
							// Send Discord message to the target user
							dpp::message dm;
							
							std::string scCredits = GetCreditsForDiscordAccount(userID);

							//Time
							time_t _tm = time(NULL);
							struct tm* curtime = localtime(&_tm);
							std::string scTime = asctime(curtime);

							//Add Embed to Message
							dm.add_embed(dpp::embed().
								set_title(":bank: Incoming Transfer").
								set_description("New Balance: " + scCredits + " Credits").
								set_color(0x00FFFF).
								add_field("Sender: " + GetDiscordUsername(event.command.usr), "").
								add_field("Amount: " + scAmount, "").
								add_field("Date: " + scTime, "")
							);
							
							//Send Message	
							DiscordBot.direct_message_create(userID, dm);

							event.reply("Credits transfered successfully!");
							return;
						}
						else
						{
							event.reply("Error: Failed to update the database.");
							return;
						}
					}
					else
					{
						event.reply("Error: Failed to update the database.");
						return;
					}
				}
				else {
					//Discord To IngameChar

					
						{
							// Mutex sperren
							std::lock_guard<std::mutex> lock(m_Mutex);

							//Add Cash to Char
							// The last error.
							HK_ERROR err;

							if ((err = HkAddCash(stows(scTarget), iAmount)) != HKE_OK) {

								if (err == HKE_CHAR_DOES_NOT_EXIST)
								{
									std::string errorMessage = "Sorry, the character does not exist.";

									DiscordBot.direct_message_create(event.command.usr.id, errorMessage);
									event.reply();

									return;
								}

								std::string errorMessage = "Sorry, an error occurred.\n\n"
								"Please try again later.\n\n";

								DiscordBot.direct_message_create(event.command.usr.id, errorMessage);
								event.reply();

								return;
							}

							if (UpdateCreditsForDiscordAccount(std::to_string(event.command.usr.id), scAmount, false))
							{
		
								//Send Discord Message
								event.reply("Credits transfered successfully!");
								return;
							}
							else
							{
								HkAddCash(stows(scTarget), -iAmount);
								event.reply("Error: Failed to update the database.");
								return;
							}



						} // Mutex wird hier automatisch freigegeben
				}

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

		//Start Bot
		DiscordBot.start(dpp::st_wait);
		
	} //Discord Bot END
	
	

	//Menus
	void CharManagerPageMenu(dpp::cluster& DiscordBot, const dpp::button_click_t& event) {
		// Delete the original message
		DiscordBot.message_delete(event.command.message_id, event.command.channel_id);

		// Get the selected value
		std::string selectedValue = event.custom_id;

		//ConPrint(L"CharManagerPageMenu: " + stows(selectedValue) + L"\n");

		dpp::message pageContent(event.command.channel_id, "Please select an option!");

		if (selectedValue == "charmanager_page") {
			// Seite für CharManager
			
			pageContent.add_component(
				dpp::component().add_component(
					dpp::component().set_type(dpp::cot_selectmenu).
					set_placeholder("charmanager_page").
					add_select_option(dpp::select_option("CharManager", "charmanager_page", "Show the CharManager Menu")).
					add_select_option(dpp::select_option("DepotManager", "depotmanager_page", "Show the DepotManager Menu")).
					add_select_option(dpp::select_option("FL:SR League", "flsrleague_page", "Show the FL:SR League Menu"))
					.set_id("charmanager_page")
				)
			);
		}
		else if (selectedValue == "depotmanager_page") {
			// Seite für DepotManager
			pageContent.add_component(
				dpp::component().add_component(
					dpp::component().set_type(dpp::cot_selectmenu).
					set_placeholder("depotmanager_page").
					add_select_option(dpp::select_option("CharManager", "charmanager_page", "Show the CharManager Menu")).
					add_select_option(dpp::select_option("DepotManager", "depotmanager_page", "Show the DepotManager Menu")).
					add_select_option(dpp::select_option("FL:SR League", "flsrleague_page", "Show the FL:SR League Menu"))
					.set_id("depotmanager_page")
				)
			);
		}
		else if (selectedValue == "flsrleague_page") {
			// Seite für FL:SR League
			pageContent.add_component(
				dpp::component().add_component(
					dpp::component().set_type(dpp::cot_selectmenu).
					set_placeholder("flsrleague_page").
					add_select_option(dpp::select_option("CharManager", "charmanager_page", "Show the CharManager Menu")).
					add_select_option(dpp::select_option("DepotManager", "depotmanager_page", "Show the DepotManager Menu")).
					add_select_option(dpp::select_option("FL:SR League", "flsrleague_page", "Show the FL:SR League Menu"))
					.set_id("flsrleague_page")
				)
			);
		}
		else {
			// Ungültiger Wert
			pageContent.add_component(
				dpp::component().add_component(
					dpp::component().set_type(dpp::cot_selectmenu).
					set_placeholder("Select Setting").
					add_select_option(dpp::select_option("CharManager", "charmanager_page", "Show the CharManager Menu")).
					add_select_option(dpp::select_option("DepotManager", "depotmanager_page", "Show the DepotManager Menu")).
					add_select_option(dpp::select_option("FL:SR League", "flsrleague_page", "Show the FL:SR League Menu"))
					.set_id("CharManager_Main")
				)
			);
		}
		
		DiscordBot.direct_message_create(event.command.usr.id, pageContent);
	}

	//Embeds
	template<typename T>
	void BankEmbed(const T& event)
	{
		//New Direct Message
		DMMessage NewMessage1;
		NewMessage1.DiscordUserID = std::to_string(event.command.usr.id);
		NewMessage1.DiscordMessage = dpp::message("Use /bank <amount> ingame to transfer Credits to your Account!");
		lDMMessages.push_back(NewMessage1);

		//Get User Credits
		std::string scCredits = GetCreditsForDiscordAccount(std::to_string(event.command.usr.id));

		//Create Message
		dpp::message msg("");

		//Add Embed to Message
		msg.add_embed(dpp::embed().
			set_title("Bank").
			set_description("Your current balance: " + scCredits + " Credits").
			set_color(0x00FFFF)
		);

		//Add Buttons to Message
		msg.add_component(
			dpp::component().add_component(
				dpp::component().set_type(dpp::cot_button).
				set_label("Transfer to Discord").
				set_style(dpp::cos_primary).
				set_id("transfer_to_discord")
			).add_component(
				dpp::component().set_type(dpp::cot_button).
				set_label("Transfer to Character").
				set_style(dpp::cos_primary).
				set_id("transfer_to_char")
			)
		);

		//New Direct Message
		DMMessage NewMessage;
		NewMessage.DiscordUserID = std::to_string(event.command.usr.id);
		NewMessage.DiscordMessage = msg;
		lDMMessages.push_back(NewMessage);
		event.reply("");

	}

	//Modals
	template<typename T>
	void LinkModal(const T& event)
	{
		dpp::interaction_modal_response link_modal_p1("link_modal_p1", "Charlinking");
		link_modal_p1.add_component(
			dpp::component().
			set_label("Enter Charname to link:").
			set_id("link_modal_p1_charname").
			set_type(dpp::cot_text).
			set_placeholder("Charname").
			set_min_length(1).
			set_max_length(23).
			set_text_style(dpp::text_short)
		);

		event.dialog(link_modal_p1);
	}

	template<typename T>
	void CharRenameModal(const T& event)
	{
		dpp::interaction_modal_response rename_modal_p1("rename_modal_p1", "Character Rename");
		rename_modal_p1.add_component(
			dpp::component().
			set_label("Enter old Charname:").
			set_id("rename_modal_charname_old").
			set_type(dpp::cot_text).
			set_placeholder("Charname").
			set_min_length(1).
			set_max_length(23).
			set_text_style(dpp::text_short)
		);

		rename_modal_p1.add_row();

		rename_modal_p1.add_component(
			dpp::component().
			set_label("Enter new Charname:").
			set_id("rename_modal_charname_new").
			set_type(dpp::cot_text).
			set_placeholder("Charname").
			set_min_length(1).
			set_max_length(23).
			set_text_style(dpp::text_short)
		);

		event.dialog(rename_modal_p1);

	}

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
					int iRank = pPD->iRank;
					uint iShipArch = pPD->iShipArchetype;
					uint iClientID = HkGetClientIdFromPD(pPD);
					//Charname
					std::wstring wscCharname = (wchar_t*)Players.GetActiveCharacterName(iClientID);
					std::string scCharname = wstring_to_utf8(wscCharname);
					//Ping
					HKPLAYERINFO pi;
					HkGetPlayerInfo(wscCharname, pi, false);
					auto ping = static_cast<int>(pi.ci.dwRoundTripLatencyMS);


					charnames.push_back(scCharname);
					ships.push_back(std::to_string(iShipArch));
					pings.push_back(std::to_string(ping));


				}

			} // Mutex wird hier automatisch freigegeben

			// Erstellen der Discord-Nachricht
			dpp::message msg("");
			dpp::embed embed = dpp::embed().
				set_title("Serverstatus").
				set_description("Online Players: " + std::to_string(iOnlinePlayers)).
				set_color(0x00FFFF);

			// Charakternamen zu einer Zeile zusammenfügen
			std::string charnamesLine;
			for (size_t i = 0; i < charnames.size(); ++i)
			{
				if (i > 0)
					charnamesLine += "\n";
				charnamesLine += charnames[i];
			}

			// Schiffe zu einer Zeile zusammenfügen
			HkLoadStringDLLs();
			std::string shipsLine;
			for (size_t i = 0; i < ships.size(); ++i)
			{
				if (i > 0)
					shipsLine += "\n";

				Archetype::Ship* ship = Archetype::GetShip(std::stoul(ships[i]));
				std::wstring wscShipName = HkGetWStringFromIDS(ship->iIdsName).c_str();

				shipsLine += wstos(wscShipName);
			}

			// Pings zu einer Zeile zusammenfügen
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

			// Senden der Nachricht
			event.reply(msg);
		}
		else
		{
			event.reply("Currently there are no players online.");
		}


	}

	template<typename T>
	void BankTransferModal(const T& event, const std::string modal_id)
	{

		std::string scLabel = "Bank Transfer";
		if (modal_id == "BankTransferDiscordAccount")
		{
			scLabel = scLabel +  " Discord Account";
		}
		else if (modal_id == "BankTransferIngameChar")
		{
			scLabel = scLabel + " Ingame Character";
			
		}


		dpp::interaction_modal_response bank_transfer_modal("bank_transfer_modal", scLabel);
		bank_transfer_modal.add_component(
			dpp::component().
			set_label("Recipient:").
			set_id(modal_id+"rec").
			set_type(dpp::cot_text).
			set_placeholder("Recipient").
			set_min_length(1).
			set_max_length(23).
			set_required(true).
			set_text_style(dpp::text_short)
		);
		bank_transfer_modal.add_row();
		bank_transfer_modal.add_component(
			dpp::component().
			set_label("Amount (Credits):").
			set_id(modal_id + "amount").
			set_type(dpp::cot_text).
			set_placeholder("Amount").
			set_min_length(1).
			set_max_length(10).
			set_required(true).
			set_text_style(dpp::text_short)
		);

		event.dialog(bank_transfer_modal);
	}

	//Commands
	void CommandUptime(const dpp::slashcommand_t& event)
	{
		// calculate uptime
		FILETIME ftCreation;
		FILETIME ft;
		GetProcessTimes(GetCurrentProcess(), &ftCreation, &ft, &ft, &ft);
		SYSTEMTIME st;
		GetSystemTime(&st);
		FILETIME ftNow;
		SystemTimeToFileTime(&st, &ftNow);
		__int64 iTimeCreation =
			(((__int64)ftCreation.dwHighDateTime) << 32) + ftCreation.dwLowDateTime;
		__int64 iTimeNow =
			(((__int64)ftNow.dwHighDateTime) << 32) + ftNow.dwLowDateTime;

		uint iUptime = (uint)((iTimeNow - iTimeCreation) / 10000000);
		uint iDays = (iUptime / (60 * 60 * 24));
		iUptime %= (60 * 60 * 24);
		uint iHours = (iUptime / (60 * 60));
		iUptime %= (60 * 60);
		uint iMinutes = (iUptime / 60);
		iUptime %= (60);
		uint iSeconds = iUptime;
		wchar_t wszUptime[16];
		swprintf_s(wszUptime, L"%.1u:%.2u:%.2u:%.2u", iDays, iHours, iMinutes,
			iSeconds);
		std::string sUptime = wstos(wszUptime);
		event.reply ("ServerUptime: " + sUptime);

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

	std::string GeneratePassword() {
		std::string password;
		std::string validCharacters = "abcdefghjkmnpqrstuvwxyzABCDEFGHJKMNPQRSTUVWXYZ23456789";

		std::random_device rd;
		std::mt19937 generator(rd());
		std::uniform_int_distribution<> distribution(0, validCharacters.length() - 1);

		for (int i = 0; i < 5; i++) {
			password += validCharacters[distribution(generator)];
		}


		return password;
	}

	std::string GetFormComponentValue(const dpp::form_submit_t& event, const std::string& customId)
	{
		for (const auto& component : event.components)
		{
			// Durchsuche die Komponenten der aktuellen Ebene
			for (const auto& subComponent : component.components)
			{
				// Überprüfe, ob die Subkomponente das gewünschte custom_id hat
				if (subComponent.custom_id == customId)
				{
					if (subComponent.type == dpp::cot_text)
					{
						return std::get<std::string>(subComponent.value);
					}
					// Handle other component types if needed
				}
			}
			

			// Überprüfe, ob die Komponente das gewünschte custom_id hat
			if (component.custom_id == customId)
			{
				if (component.type == dpp::cot_text)
				{
					return std::get<std::string>(component.value);
				}
				// Handle other component types if needed
			}

		}

		// Komponente mit dem angegebenen custom_id wurde nicht gefunden
		return ""; // oder eine andere geeignete Standard-Rückgabewert
	}




	//SQL
	std::string CharManager_Insert(const std::string& charfile, const std::string& discordID, const std::string& password)
	{
		try
		{
			// Open a database file with UTF-8 encoding
			SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

			// Prüfe, ob bereits ein Eintrag mit dem Charakterdateinamen existiert
			SQLite::Statement checkQuery(db, R"(SELECT COUNT(*) FROM "CharManager" WHERE Charfile = ?;)");
			checkQuery.bind(1, charfile);
			if (checkQuery.executeStep() && checkQuery.getColumn(0).getInt() > 0)
			{
				// Eintrag bereits vorhanden
				return "Charlinking - Error: Character is already linked to a Discord account or in a linking Process.";
			}

			// Vorbereitung eines INSERT-Statements mit Platzhaltern
			SQLite::Statement insertQuery(db, R"(INSERT INTO "CharManager" (Charfile, DiscordAccount, Validation) VALUES (?, ?, ?);)");

			// Binden der Werte an die Platzhalter
			insertQuery.bind(1, charfile);
			insertQuery.bind(2, discordID);
			insertQuery.bind(3, password);

			// Ausführen des INSERT-Statements
			insertQuery.exec();

			return "OK";
		}
		catch (std::exception& e)
		{
			std::string error = e.what();
			ConPrint(L"SQLERROR: " + stows(error) + L"\n");
			// Fehler, bitte kontaktiere den Administrator für Unterstützung
			return "Charlinking - Error: Please contact an administrator for further assistance.";
		}

	}

	void CharManager_DeleteInvalidEntries()
	{
		try
		{
			// Öffne die Datenbankdatei
			SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

			// Vorbereitung des DELETE-Statements
			SQLite::Statement query(db, R"(DELETE FROM "CharManager" WHERE "Validation" != 'TRUE';)");

			// Ausführen des DELETE-Statements
			query.exec();
		}
		catch (std::exception& e)
		{
			std::string error = e.what();
			ConPrint(L"SQLERROR: " + stows(error) + L"\n");
		}
	}

	std::string GetValidationForChar(const std::string& charfile)
	{
		try
		{
			// Öffne die Datenbankdatei
			SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READONLY);

			// Vorbereitung eines SELECT-Statements mit einem Parameter
			SQLite::Statement query(db, R"(SELECT "Validation" FROM "CharManager" WHERE Charfile = ?;)");

			// Binden des Charfile-Werts an den Parameter
			query.bind(1, charfile);

			// Überprüfen, ob ein Ergebnis vorhanden ist
			if (query.executeStep())
			{
				// Lesen des Validation-Werts aus der Ergebniszeile
				std::string result = query.getColumn(0);

				// Rückgabe des Validation-Werts
				return result;
			}
		}
		catch (std::exception& e)
		{
			std::string error = e.what();
			ConPrint(L"SQLERROR: " + stows(error) + L"\n");
		}

		// Rückgabe eines leeren Strings, wenn kein Ergebnis vorhanden oder ein Fehler aufgetreten ist
		return "";
	}

	void UpdateValidationForChar(const std::string& charfile)
	{
		try
		{
			// Öffne die Datenbankdatei
			SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

			// Vorbereitung eines UPDATE-Statements mit einem Parameter
			SQLite::Statement query(db, R"(UPDATE "CharManager" SET "Validation" = 'TRUE' WHERE Charfile = ?;)");

			// Binden des Charfile-Werts an den Parameter
			query.bind(1, charfile);

			// Ausführen des UPDATE-Statements
			query.exec();
		}
		catch (std::exception& e)
		{
			std::string error = e.what();
			ConPrint(L"SQLERROR: " + stows(error) + L"\n");
		}
	}

	std::string GetDiscordIDForChar(const std::string& charfile)
	{
		try
		{
			// Öffne die Datenbankdatei
			SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READONLY);

			// Vorbereitung eines SELECT-Statements mit einem Parameter
			SQLite::Statement query(db, R"(SELECT DiscordAccount FROM CharManager WHERE Charfile = ?;)");

			// Binden des Charfile-Werts an den Parameter
			query.bind(1, charfile);

			// Überprüfen, ob ein Ergebnis vorhanden ist
			if (query.executeStep())
			{
				// Lesen der Discord-ID aus der Ergebniszeile
				return query.getColumn(0).getString();
			}
		}
		catch (std::exception& e)
		{
			std::string error = e.what();
			ConPrint(L"SQLERROR: " + stows(error) + L"\n");
		}

		// Rückgabe eines leeren Strings, falls keine Discord-ID gefunden wurde oder ein Fehler aufgetreten ist
		return "";
	}

	bool IsCharacterLinkedWithDiscordID(const std::string& discordID, const std::string& charfile)
	{
		try
		{
			// Öffne die Datenbankverbindung
			SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

			// Erstelle die SQL-Abfrage
			std::string sql = "SELECT * FROM CharManager WHERE DiscordAccount = ? AND Charfile = ? AND Validation = ?"; //WTF
			SQLite::Statement query(db, sql);
			query.bind(1, discordID);
			query.bind(2, charfile);
			query.bind(3, "TRUE");


			// Überprüfen, ob ein Ergebnis vorhanden ist
			if (query.executeStep())
			{
				// Lesen der Discord-ID aus der Ergebniszeile
				//ConPrint(stows(query.getColumn(0).getString()) + L"\n");
				return true;
			}
		}
		catch (const std::exception& e)
		{
			// Fehler beim Ausführen der Abfrage
			std::cerr << "Error: " << e.what() << std::endl;
		}

		return false;
	}

	bool CharManager_Rename(const std::string& oldcharfile, const std::string& newcharfile)
	{
		try
		{
			// Öffne die Datenbankverbindung
			SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

			// Erstelle die SQL-Abfrage zum Aktualisieren des Charakternamens
			std::string sql = "UPDATE CharManager SET Charfile = ? WHERE Charfile = ?";
			SQLite::Statement query(db, sql);
			query.bind(1, newcharfile);
			query.bind(2, oldcharfile);

			// Führe die SQL-Abfrage aus
			if (query.exec() > 0)
			{
				// Das Update war erfolgreich
				return true;
			}
		}
		catch (const std::exception& e)
		{
			// Fehler beim Ausführen der Abfrage
			std::cerr << "Error: " << e.what() << std::endl;
		}

		return false;
	}

	bool CharManager_UpdateCharname(const std::string& charfile, const std::string& charname)
	{

		try {
			// Verbindung zur Datenbank herstellen
			SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

			// SQL-Update-Statement vorbereiten
			SQLite::Statement query(db, "UPDATE CharManager SET Charname = ? WHERE Charfile = ?");

			// Parameter binden
			query.bind(1, charname);
			query.bind(2, charfile);

			// Update ausführen
			query.exec();

			// Anzahl der aktualisierten Zeilen überprüfen
			if (db.getChanges() > 0) {
				return true; // Erfolgreich aktualisiert
			}
		}
		catch (const std::exception& e) {
			// Bei Fehlern in der Datenbankverbindung oder dem Update-Vorgang entsprechend reagieren
			std::cerr << "Database error: " << e.what() << std::endl;
		}

		return false; // Aktualisierung fehlgeschlagen
	}

	std::string GetCreditsForDiscordAccount(const std::string& discordAccount)
	{
		try
		{
			// Öffne die Datenbankverbindung
			SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

			// Erstelle die SQL-Abfrage
			std::string sql = "SELECT Credits FROM Bank WHERE DiscordAccount = ?";
			SQLite::Statement query(db, sql);
			query.bind(1, discordAccount);

			// Führe die SQL-Abfrage aus
			if (query.executeStep())
			{
				std::string credits = query.getColumn(0).getString();
				return credits;
			}
		}
		catch (const std::exception& e)
		{
			// Fehler beim Ausführen der Abfrage
			std::cerr << "Error: " << e.what() << std::endl;
		}

		// Rückgabe von "0", falls kein Eintrag gefunden wurde
		return "0";
	}

	bool UpdateCreditsForDiscordAccount(const std::string& discordAccount, const std::string& credits, bool bAdd)
	{
		try
		{
			// Öffne die Datenbankverbindung
			SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READWRITE);

			// Überprüfen, ob der Discord-Account in der Datenbank existiert
			std::string sqlCheckAccount = "SELECT COUNT(*) FROM Bank WHERE DiscordAccount = ?";
			SQLite::Statement queryCheckAccount(db, sqlCheckAccount);
			queryCheckAccount.bind(1, discordAccount);

			if (queryCheckAccount.executeStep())
			{
				int count = queryCheckAccount.getColumn(0).getInt();

				if (count == 0)
				{
					// Discord-Account existiert nicht in der Datenbank, neuen Eintrag erstellen
					std::string sqlInsert = "INSERT INTO Bank (DiscordAccount, Credits) VALUES (?, ?)";
					SQLite::Statement queryInsert(db, sqlInsert);
					queryInsert.bind(1, discordAccount);
					queryInsert.bind(2, credits);
					queryInsert.exec();
				}
				else
				{
					// Discord-Account existiert bereits, Betrag aktualisieren
					// Aktuellen Betrag abrufen und in int umwandeln
					int intCredits = std::stoi(GetCreditsForDiscordAccount(discordAccount));

					// Betrag addieren oder subtrahieren
					if (bAdd)
					{
						intCredits += std::stoi(credits);
					}
					else
					{
						int creditsToSubtract = std::stoi(credits);
						if (creditsToSubtract <= intCredits)
						{
							intCredits -= creditsToSubtract;
						}
					}

					// Balance darf nicht negativ sein
					if (intCredits < 0)
					{
						intCredits = 0;
					}

					// Aktualisierten Betrag in der Datenbank speichern
					std::string sqlUpdateCredits = "UPDATE Bank SET Credits = ? WHERE DiscordAccount = ?";
					SQLite::Statement queryUpdateCredits(db, sqlUpdateCredits);
					queryUpdateCredits.bind(1, std::to_string(intCredits));
					queryUpdateCredits.bind(2, discordAccount);
					queryUpdateCredits.exec();
				}

				// Erfolgsmeldung zurückgeben
				return true;
			}
		}
		catch (const std::exception& e)
		{
			// Fehler beim Ausführen der Abfrage
			std::cerr << "Error: " << e.what() << std::endl;
		}

		return false;
	}

	bool DoesDiscordAccountHaveValidChars(const std::string& discordAccount)
	{
		try
		{
			// Öffne die Datenbankverbindung
			SQLite::Database db(SQL::scDbName, Globals::SQLOpenFlags::OPEN_READONLY);

			// Erstelle die SQL-Abfrage
			std::string sql = "SELECT COUNT(*) FROM CharManager WHERE DiscordAccount = ? AND Validation = 'TRUE'";
			SQLite::Statement query(db, sql);
			query.bind(1, discordAccount);

			// Führe die SQL-Abfrage aus
			if (query.executeStep())
			{
				int count = query.getColumn(0).getInt();
				return count > 0;
			}
		}
		catch (const std::exception& e)
		{
			// Fehler beim Ausführen der Abfrage
			std::cerr << "Error: " << e.what() << std::endl;
		}

		return false;
	}

	std::string GetUserIDByDiscordName(const std::string& discordName)
	{

		dpp::snowflake user_id;

		dpp::guild* server = dpp::find_guild(scDiscordServerID);
		for (auto& member : server->members)
		{
			// Zugriff auf das Member-Objekt
			dpp::guild_member& guildMember = member.second;
			std::string DiscordUsername = guildMember.get_user()->username;
			std::string ServerUsername = guildMember.nickname;
			std::string GlobalUsername = guildMember.get_user()->global_name;

			if (DiscordUsername == discordName || ServerUsername == discordName || GlobalUsername == discordName)
			{
				user_id = guildMember.user_id;

				break;
			}
			//ConPrint(L"DiscordUsername: " + stows(DiscordUsername) + L"\n");

		}

		return std::to_string(user_id);

	}

	void Update_NewsList(dpp::cluster &DiscordBot)
	{


		DiscordBot.messages_get(scNewsChannelID, 0, 0, 0, 5, [&DiscordBot](const dpp::confirmation_callback_t& callback) {
			if (!callback.is_error()) {

				lNewsList.clear();

				auto messages = get<dpp::message_map>(callback.value);

				std::vector<dpp::message> reversedMessages;

				// Fülle die Liste mit Nachrichten in umgekehrter Reihenfolge
				for (const auto& pair : messages) {
					reversedMessages.insert(reversedMessages.begin(), pair.second);
				}

				// Sortiere die Nachrichten nach dem Erstellungsdatum
				std::sort(reversedMessages.begin(), reversedMessages.end(), [](const dpp::message& msg1, const dpp::message& msg2) {
					return msg1.get_creation_time() < msg2.get_creation_time();
					});

				for (const auto& msg : reversedMessages) {
					MessageListEntry MessageEntry;
					
					MessageEntry.Nickname = GetDiscordUsername(msg.author);
					MessageEntry.Message = msg;

					// Füge die Nachricht der Liste hinzu
					lNewsList.push_back(MessageEntry);
				}
			}
		});

	}

	void Update_EventList(dpp::cluster &DiscordBot)
	{


		DiscordBot.messages_get(scEventChannelID, 0, 0, 0, 5, [&DiscordBot](const dpp::confirmation_callback_t& callback) {
			if (!callback.is_error()) {

				lEventList.clear();

				auto messages = get<dpp::message_map>(callback.value);

				std::vector<dpp::message> reversedMessages;

				// Fülle die Liste mit Nachrichten in umgekehrter Reihenfolge
				for (const auto& pair : messages) {
					reversedMessages.insert(reversedMessages.begin(), pair.second);
				}

				// Sortiere die Nachrichten nach dem Erstellungsdatum
				std::sort(reversedMessages.begin(), reversedMessages.end(), [](const dpp::message& msg1, const dpp::message& msg2) {
					return msg1.get_creation_time() < msg2.get_creation_time();
					});

				for (const auto& msg : reversedMessages) {
					MessageListEntry MessageEntry;


					MessageEntry.Nickname = GetDiscordUsername(msg.author);
					MessageEntry.Message = msg;

					lEventList.push_back(MessageEntry);
				}
			}
		});

	}

	std::string GetDiscordUsername(const dpp::user& dppUser)
	{

		std::string scServerUsername;

		dpp::guild* server = dpp::find_guild(scDiscordServerID);
		for (auto& member : server->members)
		{
			// Zugriff auf das Member-Objekt
			dpp::guild_member& guildMember = member.second;

			if (guildMember.get_user()->id == dppUser.id) {
				scServerUsername = guildMember.nickname;
				break;
			}

		}


		if (!scServerUsername.empty()) {
			//ConPrint (stows(scServerUsername) + L"\n");
			return scServerUsername;
		}
		else if (!dppUser.global_name.empty()) {
			//ConPrint(stows(dppUser.global_name) + L"\n");

			return dppUser.global_name;
		}
		else {
			//ConPrint(stows(dppUser.username) + L"\n");

			return dppUser.username;
		}


	}
		
		
	bool containsWhitespace(const std::string& str) {
		return std::any_of(str.begin(), str.end(), [](char c) {
			return std::isspace(static_cast<unsigned char>(c)) != 0 ||
				(c == '\u00A0' || c == '\u1680' || c == '\u180E' ||
					(c >= '\u2000' && c <= '\u200B') || c == '\u202F' ||
					c == '\u205F' || c == '\u3000' || c == '\uFEFF');
			});
	}

} // namespace DiscordBot