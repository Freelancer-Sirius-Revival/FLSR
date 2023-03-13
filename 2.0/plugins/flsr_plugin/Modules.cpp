#include "main.h"



namespace Modules {
	
    std::list<Modules::Module> lModules;

	void LoadModules()
	{
        // Konfigpfad
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + PLUGIN_CONFIG_FILE;

        //Load Module-Settings
        ConPrint(L"\n----------LOADING FLSR-Modules----------\n");
        Modules::lModules.clear();
        std::list<INISECTIONVALUE> lModulesSettings;
        IniGetSection(scPluginCfgFile, "Modules", lModulesSettings);

        for (std::list<INISECTIONVALUE>::iterator i = lModulesSettings.begin(); i != lModulesSettings.end(); i++) {
            Modules::Module NewModule;
            NewModule.scModuleName = i->scKey;
            NewModule.bModulestate = Tools::GetB(i->scValue);
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

	void SwitchModuleState(std::string scModuleName) {
		for (std::list<Module>::iterator i = lModules.begin(); i != lModules.end(); i++) {
			if (i->scModuleName == scModuleName) {
				bool bLastState = i->bModulestate;
				lModules.erase(i);
				Module NewModule;
				if (bLastState)
					NewModule.bModulestate = false;
				else
					NewModule.bModulestate = true;
				NewModule.scModuleName = scModuleName;
				lModules.push_back(NewModule);
				return;				
			}
		}
	}



}