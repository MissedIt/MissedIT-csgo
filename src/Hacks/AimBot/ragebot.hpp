#pragma once

#include "../HeaderFiles.h"
#include "autowall.h"
#include "../lagcomp.h"

class RagebotPredictionSystem{

private:

	C_BasePlayer* enemy = nullptr;
	C_BasePlayer* localplayer = nullptr;

    #define RandomeFloat(x) (static_cast<double>( static_cast<double>(std::rand())/ static_cast<double>(RAND_MAX/x)))

	void GetDamageAndSpots(C_BasePlayer* player, Vector &Spot, int& Damage, int& playerHelth, int& i,const std::unordered_map<int, int>* modelType, const RageWeapon_t& currentSetting);

	void BestHeadPoint(C_BasePlayer* player, int &BoneIndex, int& Damage, Vector& Spot);

    void BestMultiPoint(C_BasePlayer* player, int &BoneIndex, int& Damage, Vector& Spot);

	void GetBestSpotAndDamage(C_BasePlayer* player,C_BasePlayer* localplayer, Vector& Spot, int& Damage,const RageWeapon_t& currSettings);

public :

    RagebotPredictionSystem(){}

	bool canShoot(CUserCmd* cmd, C_BasePlayer* localplayer, C_BaseCombatWeapon* activeWeapon,Vector &bestSpot, C_BasePlayer* enemy,const RageWeapon_t& currentSettings);

	void GetBestEnemyAndSpot(C_BasePlayer* localplayer,const RageWeapon_t& currSettings);

	C_BasePlayer* GetEnemy();
};


namespace Ragebot {
    struct enemy 
    {
        C_BasePlayer* player = nullptr;
        int LockedBone = -1;
        int bestDamage = 0;
        Vector lockedSpot = Vector(0);
        bool shooted = false;
        int playerhelth = 0;
    };

    inline enemy lockedEnemy;
    inline Vector localEye = Vector(0);
    inline Vector BestSpot = Vector(0);
    inline int BestDamage = 0;
    extern std::vector<int64_t> friends;

    inline int prevDamage = 0;
    inline RagebotPredictionSystem* ragebotPredictionSystem = new RagebotPredictionSystem();

    void CreateMove(CUserCmd*);
    void FireGameEvent(IGameEvent* event);
}
