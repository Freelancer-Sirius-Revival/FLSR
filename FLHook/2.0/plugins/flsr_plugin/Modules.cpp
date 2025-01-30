#include "main.h"



namespace Modules {
	
    std::list<Modules::Module> lModules;

	void LoadModules()
	{
        // Konfigpfad
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + Globals::PLUGIN_CONFIG_FILE;

        //Load Module-Settings
        ConPrint(L"\n----------LOADING FLSR-Modules----------\n");
        Modules::lModules.clear();
        std::list<INISECTIONVALUE> lModulesSettings;
        IniGetSection(scPluginCfgFile, "Modules", lModulesSettings);

        for (std::list<INISECTIONVALUE>::iterator i = lModulesSettings.begin(); i != lModulesSettings.end(); i++) {
            Modules::Module NewModule;
            NewModule.scModuleName = i->scKey;
            NewModule.bModulestate = ToLower(i->scValue) == "true";
            Modules::lModules.push_back(NewModule);
        }
	}

	bool GetModuleState(std::string scModuleName) {
		for (std::list<Module>::iterator i = lModules.begin(); i != lModules.end(); i++) {
			if (i->scModuleName == scModuleName)
				return i->bModulestate;
		}
		return false;
	}

	bool SetModuleState(const std::string& scModuleName, bool bModuleState)
	{
		for (std::list<Modules::Module>::iterator it = Modules::lModules.begin(); it != Modules::lModules.end(); ++it)
		{
			if (it->scModuleName == scModuleName)
			{
				it->bModulestate = bModuleState;
				return true;
			}
		}
		return false;
	}

	void SwitchModuleState(std::string scModuleName) {
        // Iterate through the list of modules
        for (std::list<Module>::iterator i = lModules.begin(); i != lModules.end(); i++)
        {
            // Find the module with the specified name
            if (i->scModuleName == scModuleName)
            {
                // Set the new module state based on the current state
                i->bModulestate = !i->bModulestate; // Toggle the module state (true -> false, false -> true)
            }
        }
	}



}