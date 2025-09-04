#include "NpcWaves.h"

namespace RandomMissions
{
	std::unordered_map<byte, std::vector<float>> npcWaveSizeByDifficulty;

	void ReadNpcWaveData() {
		INI_Reader ini;
		if (!ini.open("..\\DATA\\RANDOMMISSIONS\\npcranktodiff.ini", false)) return;

		while (ini.read_header())
		{
			if (ini.is_header("RankAndFormationSizeToDifficulty"))
			{
				while (ini.read_value())
				{
					if (ini.is_value("NpcRank"))
					{
						const byte rank = ini.get_value_int(0);
						for (int i = 1, length = ini.get_num_parameters(); i < length; i++)
						{
							npcWaveSizeByDifficulty[rank].push_back(ini.get_value_float(i));
						}
					}
				}
			}
		}
		ini.close();
	}
}