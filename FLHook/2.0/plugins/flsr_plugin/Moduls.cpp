#include "main.h"



namespace Moduls {
	
    std::list<Moduls::Modul> lModules;

	void LoadModuls()
	{
        // Konfigpfad
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + PLUGIN_CONFIG_FILE;

        //Load Modul-Settings
        ConPrint(L"\n----------LOADING FLSR-MODULES----------\n");
        Moduls::lModules.clear();
        std::list<INISECTIONVALUE> lModulesSettings;
        IniGetSection(scPluginCfgFile, "Modules", lModulesSettings);

        for (std::list<INISECTIONVALUE>::iterator i = lModulesSettings.begin(); i != lModulesSettings.end(); i++) {
            Moduls::Modul NewModul;
            NewModul.scModuleName = i->scKey;
            NewModul.bModulestate = Tools::GetB(i->scValue);
            Moduls::lModules.push_back(NewModul);
        }
	}

	bool GetModulState(std::string scModulName) {
		for (std::list<Modul>::iterator i = lModules.begin(); i != lModules.end(); i++) {
			if (i->scModuleName == scModulName)
				return i->bModulestate;
		}
		return false;
	}

	void SwitchModulState(std::string scModulName) {
		for (std::list<Modul>::iterator i = lModules.begin(); i != lModules.end(); i++) {
			if (i->scModuleName == scModulName) {
				bool bLastState = i->bModulestate;
				lModules.erase(i);
				Modul NewModul;
				if (bLastState)
					NewModul.bModulestate = false;
				else
					NewModul.bModulestate = true;
				NewModul.scModuleName = scModulName;
				lModules.push_back(NewModul);
				return;				
			}
		}
	}



}