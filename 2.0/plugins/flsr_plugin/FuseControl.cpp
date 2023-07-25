#include "Main.h"

namespace FuseControl {

    IMPORT ClientFuse FuseControl[MAX_CLIENT_ID + 1];
    std::map<uint, Fuse> mFuseMap;
	
    static void ReadIniFuseConfigFile(const std::string& filePath, bool bFuseIni) {
        if (!bFuseIni)
        {
            FuseType eType = FuseType::FUSE_UNKNOWN;
            INI_Reader ini;
            if (ini.open(filePath.c_str(), false)) {
                while (ini.read_header()) {

                    if (ini.is_header("CollisionGroup"))
                    {
                        eType = FuseType::FUSE_COLLGRP;
                    }
                    else if (ini.is_header("Ship"))
                    {
                        eType = FuseType::FUSE_SHIP;
                    }
                    else if (ini.is_header("Solar"))
                    {
                        eType = FuseType::FUSE_SOLAR;
                    }

                    std::string scNickname = "";
                    uint iResID = 0;
                    while (ini.read_value())
                    {

                        if (ini.is_value("ids_name")) {
                            iResID = ini.get_value_int(0);
                        }

                        if (ini.is_value("nickname")) {
                            scNickname = ini.get_value_string();
                        }

                        if (ini.is_value("fuse")) {
                            Fuse NewFuse;
                            NewFuse.eType = eType;
                            NewFuse.scShipNickname = scNickname;
                            NewFuse.scNickname = ini.get_value_string(0);
                            NewFuse.fHitpoint = ini.get_value_float(2);

                            uint hash = CreateID(Trim(ToLower(NewFuse.scNickname)).c_str());
                            mFuseMap[hash] = NewFuse;

                        }

                    }
                }
                ini.close();
            }
        }else{

            INI_Reader ini;
            if (ini.open(filePath.c_str(), false)) {
                while (ini.read_header()) {
                    if (ini.is_header("fuse")) {
                        while (ini.read_value()) {
                            if (ini.is_value("name")) {

                            }
                            if (ini.is_value("name")) {

                            }
                        }
                    }

                }
                ini.close();
            }
        }
    }

    bool ReadIniFuseConfig() {

        std::string dataDirPath = Globals::DATADIR;
        bool bReturn = false;

        //Ship, Solar ini
        INI_Reader ini;
        mFuseMap.clear();
        if (ini.open("freelancer.ini", false)) {
            while (ini.read_header())
            {
                if (ini.is_header("Data")) {
                    while (ini.read_value()) {
                        if (ini.is_value("ships")) {
                            ReadIniFuseConfigFile(dataDirPath + std::string("\\") + ini.get_value_string(), false);
                            bReturn = true;
                        }
                        else if (ini.is_value("solar")) {
                            ReadIniFuseConfigFile(dataDirPath + std::string("\\") + ini.get_value_string(), false);
                            bReturn = true;
                        }
                    }
                }
            }
            ini.close();
        }

        //Fuse ini
        INI_Reader FuseIni;
        mFuseMap.clear();
        if (FuseIni.open("freelancer.ini", false)) {
            while (FuseIni.read_header())
            {
                if (FuseIni.is_header("Data")) {
                    while (FuseIni.read_value()) {
                        if (FuseIni.is_value("fuses")) {
                            ReadIniFuseConfigFile(dataDirPath + std::string("\\") + FuseIni.get_value_string(), true);
                            bReturn = true;
                        }
                    }
                }
            }
            FuseIni.close();
        }
        

        return bReturn;
    }



	
}