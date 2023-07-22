#include "main.h"

namespace SpawnProtection {

    // Definition der Map zur Speicherung der letzten Spawn-Zeitpunkte pro iClientID
    std::map<uint, mstime> g_lastSpawnTimes;
    uint g_spawnProtectionDuration;

    bool LoadSettings()
    {
        // Konfigpfad
        char szCurDir[MAX_PATH];
        GetCurrentDirectory(sizeof(szCurDir), szCurDir);
        std::string scPluginCfgFile = std::string(szCurDir) + PLUGIN_CONFIG_FILE;

        g_spawnProtectionDuration = IniGetI(scPluginCfgFile, "SpawnProtection", "ProtectionDuration", 0);
        if (g_spawnProtectionDuration == 0) {
            ConPrint(L"ERROR: No ProtectionDuration found in config file!\n");
            return false;
        }

        return true;
	}

    // Schutzzeitdauer in Sekunden

    // Funktion zum Speichern des letzten Spawn-Zeitpunkts für einen bestimmten iClientID
    void SetLastSpawnTime(uint iClientID, mstime spawnTimestamp) {
        g_lastSpawnTimes[iClientID] = spawnTimestamp;
    }

    // Funktion zum Abrufen des letzten Spawn-Zeitpunkts für einen bestimmten iClientID
    mstime GetLastSpawnTime(uint iClientID) {
        auto it = g_lastSpawnTimes.find(iClientID);
        if (it != g_lastSpawnTimes.end()) {
            return it->second;
        }
        return 0; // Falls kein Eintrag für die iClientID vorhanden ist
    }

    // Beispiel-Funktion, die den letzten Spawn-Zeitpunkt für einen iClientID aktualisiert
    void UpdateLastSpawnTime(uint iClientID) {
        mstime currentTimestamp = timeInMS(); // Hier musst du den tatsächlichen Timestamp einfügen
        SetLastSpawnTime(iClientID, currentTimestamp);
    }

    bool IsSpawnProtectionActive(uint iClientID)
    {
        mstime currentTime = timeInMS();
        mstime lastSpawnTime = g_lastSpawnTimes[iClientID];
        uint protectionDuration = g_spawnProtectionDuration * 1000;

        return (currentTime - lastSpawnTime) < protectionDuration;
    }



} // namespace SpawnProtection