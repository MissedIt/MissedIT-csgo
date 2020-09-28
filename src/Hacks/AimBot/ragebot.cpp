// #pragma GCC diagnostic ignored "-Wcomment"
// #pragma GCC diagnostic ignored "-Warray-bounds"

#include "ragebot.hpp"
#include "resolver.h"

#define absolute(x) ( x = x < 0 ? x * -1 : x)
#define RandomeFloat(x) (static_cast<double>( static_cast<double>(std::rand())/ static_cast<double>(RAND_MAX/x)))
#define NormalizeNo(x) (x = (x < 0) ? ( x * -1) : x )

std::vector<int64_t> Ragebot::friends = {};
std::vector<long> RagebotkillTimes = { 0 }; // the Epoch time from when we kill someone
QAngle RCSLastPunch;

/*
 * Class for damage and emeny prediction for ragebot
 * Decleard in ragebot.hpp
 */
void RagebotPredictionSystem::BestHeadPoint(C_BasePlayer* player, int &BoneIndex, int& Damage, Vector& Spot)
{
	if (!player || !player->GetAlive())
		return;

	model_t* pModel = player->GetModel();
    if (!pModel)
		return;
    studiohdr_t* hdr = modelInfo->GetStudioModel(pModel);
    if (!hdr)
		return;
	mstudiobbox_t* bbox = hdr->pHitbox((int)BoneIndex, 0);
	if (!bbox)
		return;

	// 0 - center, 1 - skullcap,
	// 2 - leftear, 3 - rightear, 4 - backofhead
	Vector center = Spot;
	Vector points[5] = {center,center,center,center,center};

	points[1].y -= bbox->radius * 0.65f;
	points[1].z += bbox->radius * 0.90f;
	points[2].x += bbox->radius * 0.80f;
	points[3].x -= bbox->radius * 0.80f;
	points[4].y += bbox->radius * 0.80f;

	for (int i = 0; i < 5; i++)
	{
		float bestDamage = AutoWall::GetDamage(points[i], true);
		if (bestDamage >= player->GetHealth())
		{
			Damage = bestDamage;
			Spot = points[i];
			return;
		}
		if (bestDamage > Damage)
		{
		 	Damage = bestDamage;
			Spot = points[i];
		}
	}
}

void RagebotPredictionSystem::BestMultiPoint(C_BasePlayer* player, int &BoneIndex, int& Damage, Vector& Spot)
    {
	    if (!player || !player->GetAlive())
		    return;
	    model_t* pModel = player->GetModel();
        if (!pModel)
		    return;
        studiohdr_t* hdr = modelInfo->GetStudioModel(pModel);
        if (!hdr)
		    return;
	    mstudiobbox_t* bbox = hdr->pHitbox((int)BoneIndex, 0);
	    if (!bbox)
		    return;
	    // 0 - center 1 - left, 2 - right, 3 - back
	    Vector center = Spot;
	    Vector points[4] = { center,center,center,center };

        points[1].x += bbox->radius * 0.95f; // morph each point.
	    points[2].x -= bbox->radius * 0.95f;
	    points[3].y -= bbox->radius * 0.95f;

	    for (int i = 0; i < 4; i++)
	    {
		    int bestDamage = AutoWall::GetDamage(points[i], true);
		    if (bestDamage >= player->GetHealth())
		    {
			    Damage = bestDamage;
			    Spot = points[i];
			    return;
		    }
		    else if (bestDamage > Damage)
		    {   
			    Damage = bestDamage;
			    Spot = points[i];
		    }
	    }
    }

bool RagebotPredictionSystem::canShoot(CUserCmd* cmd, C_BaseCombatWeapon* activeWeapon,Vector &bestSpot, C_BasePlayer* enemy,const RageWeapon_t& currentSettings)
{
	if (currentSettings.HitChance == 0)
		return true;
	if (!enemy || !enemy->GetAlive())
		return false;

    Vector src = localplayer->GetEyePosition();
    QAngle angle = Math::CalcAngle(src, bestSpot);
    Math::NormalizeAngles(angle);

	Vector forward, right, up;
    Math::AngleVectors(angle, &forward, &right, &up);

    int hitCount = 0;
    int NeededHits = static_cast<int>(256.f * (currentSettings.HitChance / 100.f));

    activeWeapon->UpdateAccuracyPenalty();
    float weap_spread = activeWeapon->GetSpread();
    float weap_inaccuracy = activeWeapon->GetInaccuracy();

    for (int i = 0; i < 256; i++) {
        static float val1 = (2.0 * M_PI);
	// float b = RandomFloat(0.f, 2.f * M_PI);
    // float spread = weap_spread * RandomFloat(0.f, 1.0f);
    // float d = RandomFloat(0.f, 2.f * M_PI);
    // float inaccuracy = weap_inaccuracy * RandomFloat(0.f, 1.0f);

		double b = RandomeFloat(val1);
        double spread = weap_spread * RandomeFloat(1.0f);
        double d = RandomeFloat(1.0f);
        double inaccuracy = weap_inaccuracy * RandomeFloat(1.0f);

        Vector spreadView((cos(b) * inaccuracy) + (cos(d) * spread), (sin(b) * inaccuracy) + (sin(d) * spread), 0), direction;

       	direction.x = forward.x + (spreadView.x * right.x) + (spreadView.y * up.x);
		direction.y = forward.y + (spreadView.x * right.y) + (spreadView.y * up.y);
		direction.z = forward.z + (spreadView.x * right.z) + (spreadView.y * up.z);
		direction.Normalize();
        	
		QAngle viewAnglesSpread;
        Math::VectorAngles(direction, up, viewAnglesSpread);
		Math::NormalizeAngles(viewAnglesSpread);

		Vector viewForward;
		Math::AngleVectors(viewAnglesSpread, viewForward);
		viewForward.NormalizeInPlace();

		viewForward = src + (viewForward * activeWeapon->GetCSWpnData()->GetRange());

        trace_t tr;
        Ray_t ray;

       	ray.Init(src, viewForward);
        trace->ClipRayToEntity(ray, MASK_SHOT | CONTENTS_GRATE, enemy, &tr);

        if (tr.m_pEntityHit == enemy)
           	hitCount++;

        if (static_cast<int>((hitCount/256.f) * 100.f) >= currentSettings.HitChance)
			return true;

		if ((255 - i + hitCount) < NeededHits)
			return false;
    }

    return false;
}

void RagebotPredictionSystem::GetDamageAndSpots(C_BasePlayer* player, Vector &Spot, int& Damage, int& playerHelth, int& i,const std::unordered_map<int, int>* modelType, const RageWeapon_t& currentSetting)
{
	if (!player || !player->GetAlive() || !currentSetting.desireBones[i])
		return;

	static auto HitboxHead([&](int BoneID){
		Spot = player->GetBonePosition(BoneID);
		BestHeadPoint(player, BoneID, Damage, Spot);
		// BestMultiPointHEADDamage(player, BoneID, Damage, Spot);
	});
	static auto UpperSpine([&](int BoneID){

		Spot = player->GetBonePosition(BoneID);
		BestMultiPoint(player, BoneID, Damage, Spot);
		// BestMultiPointDamage(player, BoneID, Damage, Spot);
		if (Damage >= 80 || Damage >= playerHelth)
			return;
			
		static const int BONE[] = {	BONE_LEFT_COLLARBONE, 
									BONE_RIGHT_COLLARBONE,
									BONE_LEFT_SHOULDER, 
									BONE_RIGHT_SHOULDER
								};
		
		for (auto &j : BONE)
		{
			BoneID = (*modelType).at(j);
			Vector bone3D = player->GetBonePosition(BoneID);
			
			int bestDamage = AutoWall::GetDamage(bone3D, true);
			if (bestDamage >= playerHelth)
			{
				Damage = bestDamage;
				Spot = bone3D;
				return;
			}
			if (bestDamage >= Damage)
			{
				Damage = bestDamage;
				Spot = bone3D;
			}
			if (bestDamage >= 70)
				return;
		}
	});
	static auto MiddleSpine([&](int BoneID){

		Spot = player->GetBonePosition(BoneID);
		BestMultiPoint(player, BoneID, Damage, Spot);
		// BestMultiPointDamage(player, BoneID, Damage, Spot);
		if (Damage >= 80 || Damage >= playerHelth)
			return;
			
		static const int BONE[] = {
									BONE_LEFT_ARMPIT,
    								BONE_RIGHT_ARMPIT,
									BONE_LEFT_BICEP,
    								BONE_RIGHT_BICEP,
									BONE_LEFT_ELBOW,
    								BONE_RIGHT_ELBOW
								};
		
		for (auto &j : BONE)
		{
			BoneID = (*modelType).at(j);
			Vector bone3D = player->GetBonePosition(BoneID);
			
			int bestDamage = AutoWall::GetDamage(bone3D, true);
			if (bestDamage >= playerHelth)
			{
				Damage = bestDamage;
				Spot = bone3D;
				return;
			}
			else if (bestDamage >= Damage)
			{
				Damage = bestDamage;
				Spot = bone3D;
			}
			else if (bestDamage >= 70)
				return;
		}
	});
	static auto LowerSpine([&](int BoneID){

		Spot = player->GetBonePosition(BoneID);
		BestMultiPoint(player, BoneID, Damage, Spot);
		// BestMultiPointDamage(player, BoneID, Damage, Spot);
		if (Damage >= 80 || Damage >= playerHelth)
			return;
			
		static const int BONE[] = {	
									BONE_LEFT_FOREARM,
    								BONE_LEFT_WRIST,
    								BONE_RIGHT_FOREARM,
    								BONE_RIGHT_WRIST,
								};
		
		for (auto &j : BONE)
		{
			BoneID = (*modelType).at(j);
			Vector bone3D = player->GetBonePosition(BoneID);
			
			int bestDamage = AutoWall::GetDamage(bone3D, true);
			if (bestDamage >= playerHelth)
			{
				Damage = bestDamage;
				Spot = bone3D;
				return;
			}
			if (bestDamage >= Damage)
			{
				Damage = bestDamage;
				Spot = bone3D;
			}
			if (bestDamage >= 70)
				return;
		}
	});
	static auto HipHitbox([&](int BoneID){

		Spot = player->GetBonePosition(BoneID);
		BestMultiPoint(player, BoneID, Damage, Spot);
		// BestMultiPointDamage(player, BoneID, Damage, Spot);
		if (Damage >= 40 || Damage >= playerHelth)
			return;
			
		static const int BONE[] = {BONE_LEFT_BUTTCHEEK,
    								BONE_LEFT_THIGH,
    								BONE_RIGHT_BUTTCHEEK,
    								BONE_RIGHT_THIGH,
								};
		
		for (auto &j : BONE)
		{
			BoneID = (*modelType).at(j);
			Vector bone3D = player->GetBonePosition(BoneID);
			
			int bestDamage = AutoWall::GetDamage(bone3D, true);
			if (bestDamage >= playerHelth)
			{
				Damage = bestDamage;
				Spot = bone3D;
				return;
			}
			if (bestDamage >= Damage)
			{
				Damage = bestDamage;
				Spot = bone3D;
			}
			if (bestDamage >= 70)
				return;
		}
	});
	static auto PelvisHitbox([&](int BoneID){

		Spot = player->GetBonePosition(BoneID);
		BestMultiPoint(player, BoneID, Damage, Spot);
		// BestMultiPointDamage(player, BoneID, Damage, Spot);
		if (Damage >= 40 || Damage >= playerHelth)
			return;
			
		static const int BONE[] = {BONE_LEFT_KNEE, 
									BONE_LEFT_ANKLE,
									BONE_LEFT_SOLE, 
									BONE_RIGHT_BUTTCHEEK,
									BONE_RIGHT_THIGH,
									BONE_RIGHT_KNEE,
									BONE_RIGHT_ANKLE,
									BONE_RIGHT_SOLE};
		
		for (auto &j : BONE)
		{
			BoneID = (*modelType).at(j);
			Vector bone3D = player->GetBonePosition(BoneID);
			
			int bestDamage = AutoWall::GetDamage(bone3D, true);
			if (bestDamage >= playerHelth)
			{
				Damage = bestDamage;
				Spot = bone3D;
				return;
			}
			if (bestDamage >= Damage)
			{
				Damage = bestDamage;
				Spot = bone3D;
			}
			if (bestDamage >= 70)
				return;
		}
	});
	static auto DefaultHitbox([&](int BoneID){
		Spot = player->GetBonePosition(BoneID);
		Damage = AutoWall::GetDamage(Spot, true);
	});

	int boneID = -1;

	switch (static_cast<DesireBones>(i))
	{
		case DesireBones::BONE_HEAD:
			boneID = (*modelType).at(BONE_HEAD);
			if ( playerHelth <= 85  ) 
				boneID = (*modelType).at(BONE_NECK);
			if (currentSetting.desiredMultiBones[i]) 
				HitboxHead(boneID); // lamda expression because again creating a new method is going to make the source code mess :p
			else 
				DefaultHitbox(boneID);
			break;
		
		case DesireBones::UPPER_CHEST:
			boneID = (*modelType).at(BONE_UPPER_SPINAL_COLUMN);
			if (currentSetting.desiredMultiBones[i]) 
				UpperSpine(boneID);
			else 
				DefaultHitbox(boneID);
			break;
			
		case DesireBones::MIDDLE_CHEST:
			boneID = (*modelType).at(BONE_MIDDLE_SPINAL_COLUMN);
			if (currentSetting.desiredMultiBones[i]) 
				MiddleSpine(boneID);
			else 
				DefaultHitbox(boneID);
			break;
		
		case DesireBones::LOWER_CHEST:
			boneID = (*modelType).at(BONE_LOWER_SPINAL_COLUMN);
			if (currentSetting.desiredMultiBones[i]) 
				LowerSpine(boneID);
			else 
				DefaultHitbox(boneID);
			break;
		
		case DesireBones::BONE_HIP:
			boneID = (*modelType).at(BONE_HIP);
			if (currentSetting.desiredMultiBones[i]) 
				HipHitbox(boneID);
			else 
				DefaultHitbox(boneID);
			break;
		
		case DesireBones::LOWER_BODY:
			boneID = BONE_PELVIS;
			if (currentSetting.desiredMultiBones[i]) 
				PelvisHitbox(boneID);
			else 
				DefaultHitbox(boneID);
			break;
	}
}

void RagebotPredictionSystem::GetBestSpotAndDamage(C_BasePlayer* player, Vector& Spot, int& Damage,const RageWeapon_t& currSettings)
{	
	if (!player || !localplayer || !player->GetAlive() || !localplayer->GetAlive())
		return;
		
	// Atleast Now Total Bones we are caring of
	Vector spot = Vector(0);
	int damage = 0.f;

	int playerHelth = player->GetHealth();
	const std::unordered_map<int, int>* modelType = BoneMaps::GetModelTypeBoneMap(player);

	// static matrix3x4_t boneMatrix[128];
	// 	player->SetupBones(boneMatrix, 128, 0x100, 0);
	
	static int i;
	for (i = 0; i < 6; i++)
	{
		GetDamageAndSpots(player, spot, damage, playerHelth, i, modelType, currSettings);
		if (damage >= playerHelth)
		{
			Damage = damage;
			Spot = spot;
			return;
		}
		else if (damage > Damage)
		{
			Damage = damage;
			Spot = spot;
		}
	}	
}

void RagebotPredictionSystem::GetBestEnemyAndSpot(const RageWeapon_t& currSettings)
{
	this->enemy = nullptr;

	if (!localplayer || !localplayer->GetAlive())
		return;
	
	Vector bestSpot = Vector(0);
	int bestDamage = 0;

	Ragebot::BestDamage = 0;
	Ragebot::BestSpot = Vector(0);

	if (Ragebot::lockedEnemy.player)
	{
		if (Ragebot::lockedEnemy.player->GetAlive() && !Ragebot::lockedEnemy.player->GetDormant() && !Ragebot::lockedEnemy.player->GetImmune())
		{
			GetBestSpotAndDamage(Ragebot::lockedEnemy.player, bestSpot, bestDamage, currSettings);
			if (bestDamage >= Ragebot::lockedEnemy.player->GetHealth() || bestDamage >= currSettings.MinDamage)
			{
				Ragebot::BestDamage = bestDamage;
				Ragebot::BestSpot = bestSpot;
				this->enemy = Ragebot::lockedEnemy.player;
				return;
			}
		}
		else 
		{
			Ragebot::lockedEnemy.player = nullptr;
			Ragebot::lockedEnemy.bestDamage = 0;
			Ragebot::lockedEnemy.lockedSpot = Vector(0);
		}
		
	}
	
	int maxClient = engine->GetMaxClients();
	C_BasePlayer* player = nullptr;
	for (int i = 1; i < maxClient; ++i)
	{
		player = (C_BasePlayer*) entityList->GetClientEntity(i);

		if (!player || 
			player == localplayer || 
			player->GetDormant() || 
			!player->GetAlive() || 
			player->GetImmune())
			continue;

		if (Entity::IsTeamMate(player, localplayer)) // Checking for Friend If any it will continue to next player
			continue;			

		GetBestSpotAndDamage(player, bestSpot, bestDamage, currSettings);
		
		if (bestDamage >= player->GetHealth() )
		{
			Ragebot::BestDamage = bestDamage;
			Ragebot::BestSpot = bestSpot;
			this->enemy = player;
			return;
		}	
		else if (bestDamage > Ragebot::BestDamage){
			Ragebot::BestDamage = bestDamage;
			Ragebot::BestSpot = bestSpot;
			this->enemy = player;
		}
	}	

	if (Ragebot::BestDamage < currSettings.MinDamage || Ragebot::BestDamage <= 0)
		this->enemy = nullptr;
}

void RagebotPredictionSystem::CheckHit(C_BaseCombatWeapon *activeWeapon){
	if (!localplayer || !localplayer->GetAlive())
		return;
	// cvar->ConsoleDPrintf("1 \n");
	if (!Ragebot::lockedEnemy.shooted || !Ragebot::lockedEnemy.player)
		return;
	// cvar->ConsoleDPrintf("2 \n");
    if (Ragebot::lockedEnemy.playerhelth != Ragebot::lockedEnemy.player->GetHealth() || !Ragebot::lockedEnemy.player->GetAlive())
		return;
	// cvar->ConsoleDPrintf("3 \n");
	// if ( activeWeapon->GetNextPrimaryAttack() > static_cast<float>(localplayer->GetTickBase()) * TICK_INTERVAL)
	// 	return;

    Ray_t ray;
    trace_t tr;
	/*
    // traceStart = localplayer->GetEyePosition();
	// traceEnd = this->bullePosition;
	*/\
	const Vector &traceStart = localplayer->GetEyePosition();
	const Vector &traceENd = *this->bulletPosition;
    ray.Init(traceStart, traceENd);
 	CTraceFilter* traceFilter = new CTraceFilter();
    traceFilter->pSkip = localplayer;

    trace->TraceRay(ray, MASK_SHOT, traceFilter, &tr);

    // cvar->ConsoleDPrintf("traced \n");
    if (tr.m_pEntityHit == Ragebot::lockedEnemy.player){
        Resolver::players[Ragebot::lockedEnemy.player->GetIndex()].MissedCount++;
		// Draw::ImText( ImVec2( 0.f, 0.f ), ImColor( 255, 0, 0, 255 ), XORSTR( "[ Missed shot due to bad resolve ]" ), nullptr, 0.0f, nullptr, ImFontFlags_Shadow );
        cvar->ConsoleDPrintf(XORSTR("say Missed shot on %s due to bad resolve \n"), XORSTR("Retard"));
	}
	else{
		// Draw::ImText( ImVec2( 0.f, 0.f ), ImColor( 255, 0, 0, 255 ), XORSTR( "[ Missed shot due to spread ]" ), nullptr, 0.0f, nullptr, ImFontFlags_Shadow );
		cvar->ConsoleDPrintf(XORSTR("Missed shot on %s due to spread\n"), XORSTR("Retard"));
	}

	if (Resolver::players[Ragebot::lockedEnemy.player->GetIndex()].MissedCount > 4){
		Resolver::players[Ragebot::lockedEnemy.player->GetIndex()].MissedCount = 0;
	}
		
	Ragebot::lockedEnemy.shooted = false;
	Ragebot::lockedEnemy.playerhelth = 0;
}

void RagebotPredictionSystem::SetBulletPosition(float& x, float& y, float& z){
	this->bulletPosition->Init(x, y, z);
} 

void RagebotPredictionSystem::init(C_BasePlayer* localplayer){
	this->localplayer = localplayer;
}
C_BasePlayer* RagebotPredictionSystem::GetEnemy(){ return this->enemy; }

/* END */

void RagebotNoRecoil(QAngle& angle, CUserCmd* cmd, C_BasePlayer* localplayer, C_BaseCombatWeapon* activeWeapon, const RageWeapon_t& currentSettings)
{
    if (!(cmd->buttons & IN_ATTACK) 
		|| !localplayer
		|| !localplayer->GetAlive())
		return;
	
	if (*activeWeapon->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_SSG08 || *activeWeapon->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_AWP)
		return;

	QAngle CurrentPunch = *localplayer->GetAimPunchAngle();

	if (currentSettings.silent)
	{
		angle.x -= CurrentPunch.x * 2.f;
		angle.y -= CurrentPunch.y * 2.f;
	}
	else if (localplayer->GetShotsFired() > 1 )
	{
		QAngle NewPunch = { CurrentPunch.x - RCSLastPunch.x, CurrentPunch.y - RCSLastPunch.y, 0 };

		angle.x -= NewPunch.x * 2.0;
		angle.y -= NewPunch.y * 2.0;
	}
	RCSLastPunch = CurrentPunch;
}

// void RagebotAutoCrouch(C_BasePlayer* localplayer, CUserCmd* cmd, C_BaseCombatWeapon* activeWeapon, const RageWeapon_t& currentSettings)
// {
//     if (!localplayer || !localplayer->GetAlive() || !Settings::Ragebot::AutoCrouch::enable)
// 		return;
	
// 	if (activeWeapon->GetNextPrimaryAttack() > globalVars->curtime)
// 		return;

//     cmd->buttons |= IN_DUCK;
// }

void RagebotAutoSlow(C_BasePlayer* localplayer, C_BaseCombatWeapon* activeWeapon, CUserCmd* cmd, float& forrwordMove, float& sideMove, QAngle& angle, const RageWeapon_t& currentSettings)
{
	if (!currentSettings.autoSlow)
		return;
	if (!localplayer || !localplayer->GetAlive())
		return;
	if ( !activeWeapon || activeWeapon->GetInReload())
		return;
	
	if (currentSettings.autoScopeEnabled && 
		Util::Items::IsScopeable(*activeWeapon->GetItemDefinitionIndex()) && 
		!localplayer->IsScoped() && 
		!(cmd->buttons & IN_ATTACK2) && 
		!(cmd->buttons&IN_ATTACK))
		{ 
			cmd->buttons |= IN_ATTACK2; 
		}
		

	if (activeWeapon->GetNextPrimaryAttack() > globalVars->curtime ) 
		return;

	// QAngle ViewAngle;
	// 	engine->GetViewAngles(ViewAngle);
	// static Vector oldOrigin = localplayer->GetAbsOrigin( );
	// Vector velocity = ( localplayer->GetVecOrigin( )-oldOrigin ) * (1.f/globalVars->interval_per_tick);
	// oldOrigin = localplayer->GetAbsOrigin( );
	// float speed  = velocity.Length( );
		
	// if(speed > 15.f)
	// {
	// 	QAngle dir;
	// 	Math::VectorAngles(velocity, dir);
	// 	dir.y = ViewAngle.y - dir.x;
		
	// 	Vector NewMove = Vector(0);
	// 	Math::AngleVectors(dir, NewMove);
	// 	auto max = std::max(forrwordMove, sideMove);
	// 	auto mult = 450.f/max;
	// 	NewMove *= -mult;
			
	// 	forrwordMove = NewMove.x;
	// 	sideMove = NewMove.y;
	// }
	// else 
	// {	
		forrwordMove = 0.f;
		sideMove = 0.f;
	// }
	
	cmd->buttons |= IN_WALK;
}

void RagebotAutoR8(C_BasePlayer* player, C_BasePlayer* localplayer, C_BaseCombatWeapon* activeWeapon, CUserCmd* cmd,Vector& bestspot, QAngle& angle, float& forrwordMove, float& sideMove, const RageWeapon_t& currentSettings)
{
    if (!currentSettings.autoShootEnabled)
		return;
	if (!localplayer || !localplayer->GetAlive())
		return;
	if (!activeWeapon || activeWeapon->GetInReload())
		return;

	if (*activeWeapon->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_REVOLVER)
	{		
		cmd->buttons |= IN_ATTACK;

    	float postponeFireReadyTime = activeWeapon->GetPostPoneReadyTime();
		if (player)
		{
			if ( !Ragebot::ragebotPredictionSystem->canShoot(cmd, activeWeapon, bestspot, player, currentSettings))
			{
				RagebotAutoSlow(localplayer, activeWeapon, cmd, forrwordMove, sideMove, angle, currentSettings);
				cmd->buttons &= ~IN_ATTACK;
			}
		}
		else if (postponeFireReadyTime < globalVars->curtime )
			cmd->buttons &= ~IN_ATTACK;

		return;
	}
}

void RagebotAutoShoot(C_BasePlayer* player, C_BasePlayer* localplayer, C_BaseCombatWeapon* activeWeapon, CUserCmd* cmd, Vector& bestspot, QAngle& angle, float& forrwordMove, float& sideMove, const RageWeapon_t& currentSettings)
{
    if (!currentSettings.autoShootEnabled)
		return;
	if (!activeWeapon || activeWeapon->GetInReload() || activeWeapon->GetAmmo() == 0)
		return;
    CSWeaponType weaponType = activeWeapon->GetCSWpnData()->GetWeaponType();
    if (weaponType == CSWeaponType::WEAPONTYPE_KNIFE || weaponType == CSWeaponType::WEAPONTYPE_C4 || weaponType == CSWeaponType::WEAPONTYPE_GRENADE)
		return;
	if (*activeWeapon->GetItemDefinitionIndex() == ItemDefinitionIndex::WEAPON_REVOLVER)
		return;
		
	if ( Ragebot::ragebotPredictionSystem->canShoot(cmd, activeWeapon, bestspot, player, currentSettings) )
	{
		if (activeWeapon->GetNextPrimaryAttack() > globalVars->curtime)
			cmd->buttons &= ~IN_ATTACK;
		else if ( !(cmd->buttons & IN_ATTACK) )
			cmd->buttons |= IN_ATTACK;
		return;
	}

	RagebotAutoSlow(localplayer, activeWeapon, cmd, forrwordMove, sideMove, angle, currentSettings);
}

inline void AutoPistol(C_BaseCombatWeapon* activeWeapon, CUserCmd* cmd, const RageWeapon_t& currentSettings)
{
	if (!activeWeapon || activeWeapon->GetInReload())
		return;
	if (!currentSettings.autoPistolEnabled)
		return;
	if (!activeWeapon || activeWeapon->GetCSWpnData()->GetWeaponType() != CSWeaponType::WEAPONTYPE_PISTOL)
		return;
	if (activeWeapon->GetNextPrimaryAttack() < globalVars->curtime)
		return;

    if (*activeWeapon->GetItemDefinitionIndex() != ItemDefinitionIndex::WEAPON_REVOLVER)
        cmd->buttons &= ~IN_ATTACK;
}

static void FixMouseDeltas(CUserCmd* cmd, C_BasePlayer* player, QAngle& angle, QAngle& oldAngle)
{
    if (!player || !player->GetAlive())
		return;

    QAngle delta = angle - oldAngle;
    const float &sens = cvar->FindVar(XORSTR("sensitivity"))->GetFloat();
    const float &m_pitch = cvar->FindVar(XORSTR("m_pitch"))->GetFloat();
    const float &m_yaw = cvar->FindVar(XORSTR("m_yaw"))->GetFloat();
    const float &zoomMultiplier = cvar->FindVar("zoom_sensitivity_ratio_mouse")->GetFloat();

    Math::NormalizeAngles(delta);

    cmd->mousedx = -delta.y / (m_yaw * sens * zoomMultiplier);
    cmd->mousedy = delta.x / (m_pitch * sens * zoomMultiplier);
}

void Ragebot::CreateMove(CUserCmd* cmd)
{
	if (!Settings::Ragebot::enabled)
		return;
	
	C_BasePlayer* localplayer = (C_BasePlayer*)entityList->GetClientEntity(engine->GetLocalPlayer());
    if (!localplayer || !localplayer->GetAlive())
		return;
    C_BaseCombatWeapon* activeWeapon = (C_BaseCombatWeapon*)entityList->GetClientEntityFromHandle(localplayer->GetActiveWeapon());
    if (!activeWeapon || activeWeapon->GetInReload())
		return;
    CSWeaponType weaponType = activeWeapon->GetCSWpnData()->GetWeaponType();
    if (weaponType == CSWeaponType::WEAPONTYPE_C4 || weaponType == CSWeaponType::WEAPONTYPE_GRENADE || weaponType == CSWeaponType::WEAPONTYPE_KNIFE)
		return;
	
	// Cheking if our aimbot miss any shot or not basically it is not the perfect way to do this
	// But for Now I only come up with this thing :(
	Ragebot::ragebotPredictionSystem->CheckHit(activeWeapon);

	QAngle oldAngle;
    engine->GetViewAngles(oldAngle);
    
	float oldForward = cmd->forwardmove;
    float oldSideMove = cmd->sidemove;
	QAngle angle = cmd->viewangles;

	ItemDefinitionIndex index = ItemDefinitionIndex::INVALID;
    if (Settings::Ragebot::weapons.find(*activeWeapon->GetItemDefinitionIndex()) != Settings::Ragebot::weapons.end())
		  index = *activeWeapon->GetItemDefinitionIndex();
    const RageWeapon_t &currentWeaponSetting = Settings::Ragebot::weapons.at(index);
		

	Ragebot::localEye = localplayer->GetEyePosition();
    Ragebot::BestSpot = Vector(0);
	Ragebot::BestDamage = 0;
	
	Ragebot::ragebotPredictionSystem->init(localplayer);
	Ragebot::ragebotPredictionSystem->GetBestEnemyAndSpot(currentWeaponSetting);
	C_BasePlayer* player = Ragebot::ragebotPredictionSystem->GetEnemy();
	
    if (player && Ragebot::BestDamage > 0)
    {
		Ragebot::lockedEnemy.player = player;
		Ragebot::lockedEnemy.lockedSpot = Ragebot::BestSpot;
		Ragebot::lockedEnemy.bestDamage = Ragebot::BestDamage;
		Settings::Debug::AutoAim::target = Ragebot::BestSpot;

		RagebotAutoShoot(player, localplayer, activeWeapon, cmd, Ragebot::BestSpot, angle, oldForward, oldSideMove, currentWeaponSetting);
    	RagebotAutoR8(player, localplayer, activeWeapon, cmd, Ragebot::BestSpot, angle, oldForward, oldSideMove, currentWeaponSetting);
		// RagebotAutoCrouch(player, cmd, activeWeapon, currentWeaponSetting);

		if (cmd->buttons & IN_ATTACK)
		{
			angle = Math::CalcAngle(Ragebot::localEye, Ragebot::BestSpot);
			lockedEnemy.shooted = true;
			lockedEnemy.playerhelth = lockedEnemy.player->GetHealth();
		}
    }
	else{
		Ragebot::lockedEnemy.player = nullptr;
		Ragebot::lockedEnemy.lockedSpot = Vector(0);
		Ragebot::lockedEnemy.bestDamage = 0;
		Ragebot::lockedEnemy.playerhelth = 0;
		lockedEnemy.shooted = false;
	}
	
	RagebotNoRecoil(angle, cmd, localplayer, activeWeapon, currentWeaponSetting);
	AutoPistol(activeWeapon, cmd, currentWeaponSetting);

    Math::NormalizeAngles(angle);
	Math::ClampAngles(angle);

    FixMouseDeltas(cmd, player, angle, oldAngle);
    cmd->viewangles = angle;

	if (!currentWeaponSetting.silent)
		engine->SetViewAngles(cmd->viewangles);

	Math::CorrectMovement(oldAngle, cmd, oldForward, oldSideMove);
}

void Ragebot::FireGameEvent(IGameEvent* event)
{
	if(!event)	
		return;

	// cvar->ConsoleDPrintf(XORSTR("fire %s\n"), event->GetName());
	if ( strcmp(event->GetName(), XORSTR("bullet_impact")) == 0 && engine->GetPlayerForUserID(event->GetInt(XORSTR("userid"))) == engine->GetLocalPlayer()){
		float x = event->GetFloat(XORSTR("x")), y = event->GetFloat(XORSTR("y")), z = event->GetFloat(XORSTR("z"));
		// cvar->ConsoleDPrintf(XORSTR("bullet impact x : %f,y : %f,z : %f\n"), x, y, z);
		// x = Ragebot::lockedEnemy.lockedSpot.x, y = Ragebot::lockedEnemy.lockedSpot.y, z = Ragebot::lockedEnemy.lockedSpot.z;
		// cvar->ConsoleDPrintf(XORSTR("bullet impact x : %f,y : %f,z : %f\n"), x, y, z);
		Ragebot::ragebotPredictionSystem->SetBulletPosition(x, y, z);
	}
    if (strcmp(event->GetName(), XORSTR("player_connect_full")) == 0 || strcmp(event->GetName(), XORSTR("cs_game_disconnected")) == 0)
    {
		if (event->GetInt(XORSTR("userid")) && engine->GetPlayerForUserID(event->GetInt(XORSTR("userid"))) != engine->GetLocalPlayer())
	    	return;
		Ragebot::friends.clear();
    }
    if (strcmp(event->GetName(), XORSTR("player_death")) == 0)
    {
		int attacker_id = engine->GetPlayerForUserID(event->GetInt(XORSTR("attacker")));
		int deadPlayer_id = engine->GetPlayerForUserID(event->GetInt(XORSTR("userid")));

		if (attacker_id == deadPlayer_id) // suicide
	    	return;

		if (attacker_id != engine->GetLocalPlayer())
	    	return;

		RagebotkillTimes.push_back(Util::GetEpochTime());
    }
}