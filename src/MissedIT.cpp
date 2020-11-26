//#define FGUI_IMPLEMENTATION // make sure this is defined before
                            // you include the header file
//#include "FGUI/FGUI.hpp"
#include <thread>

#include "hooker.h"
#include "interfaces.h"
#include "Utils/util.h"
#include "fonts.h"
#include "Hooks/hooks.h"
#include "sdlhook.h"

#include "EventListener.h"
#include "Utils/xorstring.h"
#include "Utils/bonemaps.h"

#include "Hacks/nosmoke.h"
#include "Hacks/tracereffect.h"
#include "Hacks/skinchanger.h"
#include "Hacks/valvedscheck.h"
#include "settings.h"


static EventListener* eventListener = nullptr;

const char *Util::logFileName = "/tmp/MissedIT.log";
std::vector<VMT*> createdVMTs;

void MainThread()
{
	Interfaces::FindInterfaces();
    Interfaces::DumpInterfaces();

    cvar->ConsoleDPrintf(XORSTR("Loading...\n"));

	Hooker::FindSetNamedSkybox();
	Hooker::FindViewRender();
	Hooker::FindSDLInput();
	Hooker::FindIClientMode();
	Hooker::FindGlobalVars();
	Hooker::FindCInput();
	Hooker::FindGlowManager();
	Hooker::FindPlayerResource();
	Hooker::FindGameRules();
	Hooker::FindRankReveal();
	Hooker::FindSendClanTag();
	Hooker::FindPrediction();
	Hooker::FindSetLocalPlayerReady();
	Hooker::FindSurfaceDrawing();
	Hooker::FindGetLocalClient();
	Hooker::FindLineGoesThroughSmoke();
	Hooker::FindInitKeyValues();
	Hooker::FindLoadFromBuffer();
	Hooker::FindOverridePostProcessingDisable();
    Hooker::FindPanelArrayOffset();
    Hooker::FindPlayerAnimStateOffset();
    Hooker::FindPlayerAnimOverlayOffset();
	Hooker::FindSequenceActivity();
    Hooker::FindAbsFunctions();
    Hooker::FindItemSystem();
    Hooker::FindSendMove();
     Hooker::FindWriteUserCmd(); // write user cmd
    SDL2::HookSwapWindow();
    SDL2::HookPollEvent();

    Offsets::GetNetVarOffsets();
    Fonts::SetupFonts();

    
    engineVGuiVMT = new VMT(engineVGui);
    engineVGuiVMT->HookVM(Hooks::Paint, 15);
    engineVGuiVMT->ApplyVMT();

    clientModeVMT = new VMT(clientMode);
    clientModeVMT->HookVM(Hooks::OverrideView, 19);
    clientModeVMT->HookVM(Hooks::CreateMove, 25);
    // clientModeVMT->HookVM(Hooks::CreateMove2, 27);
    clientModeVMT->HookVM(Hooks::ShouldDrawCrosshair, 29);
    clientModeVMT->HookVM(Hooks::GetViewModelFOV, 36);
    clientModeVMT->ApplyVMT();

    clientVMT = new VMT(client);
    clientVMT->HookVM(Hooks::LevelInitPostEntity, 6);
    clientVMT->HookVM(Hooks::WriteUsercmdDeltaToBuffer, 24);
    clientVMT->HookVM(Hooks::FrameStageNotify, 37);
	clientVMT->ApplyVMT();

    materialVMT = new VMT(material);
    materialVMT->HookVM(Hooks::OverrideConfig, 21);
    materialVMT->HookVM(Hooks::BeginFrame, 42);
	materialVMT->ApplyVMT();

    gameEventsVMT = new VMT(gameEvents);
	gameEventsVMT->HookVM(Hooks::FireEventClientSide, 10);
	gameEventsVMT->ApplyVMT();

    inputInternalVMT = new VMT(inputInternal);
    inputInternalVMT->HookVM(Hooks::SetKeyCodeState, 92);
    inputInternalVMT->HookVM(Hooks::SetMouseCodeState, 93);
    inputInternalVMT->ApplyVMT();

    launcherMgrVMT = new VMT(launcherMgr);
    launcherMgrVMT->HookVM(Hooks::PumpWindowsMessageLoop, 19);
    launcherMgrVMT->ApplyVMT();

    soundVMT = new VMT(sound);
    soundVMT->HookVM( Hooks::EmitSound2, 6);
    soundVMT->ApplyVMT();

    modelRenderVMT = new VMT(modelRender);
    modelRenderVMT->HookVM(Hooks::DrawModelExecute, 21);
    modelRenderVMT->ApplyVMT();

    panelVMT = new VMT(panel);
    panelVMT->HookVM(Hooks::PaintTraverse, 42);
    panelVMT->ApplyVMT();

    viewRenderVMT = new VMT(viewRender);
    viewRenderVMT->HookVM(Hooks::RenderView, 6 );
    viewRenderVMT->HookVM(Hooks::RenderSmokePostViewmodel, 42);
    viewRenderVMT->ApplyVMT();
    
    surfaceVMT = new VMT(surface);
    surfaceVMT->HookVM(Hooks::OnScreenSizeChanged, 116);
	surfaceVMT->ApplyVMT();

    
    
	eventListener = new EventListener({ XORSTR("bullet_impact"),  XORSTR("cs_game_disconnected"), XORSTR("player_connect_full"), XORSTR("player_death"), XORSTR("item_purchase"), XORSTR("item_remove"), XORSTR("item_pickup"), XORSTR("player_hurt"), XORSTR("bomb_begindefuse"), XORSTR("enter_bombzone"), XORSTR("bomb_beginplant"), XORSTR("switch_team"), XORSTR("round_start"), XORSTR("vote_cast") });

	if (Hooker::HookRecvProp(XORSTR("CBaseViewModel"), XORSTR("m_nSequence"), SkinChanger::sequenceHook))
		SkinChanger::sequenceHook->SetProxyFunction((RecvVarProxyFn) SkinChanger::SetViewModelSequence);

	srand(time(nullptr)); // Seed random # Generator so we can call rand() later

    // Build bonemaps here if we are already in-game
    if( engine->IsInGame() ){
        BoneMaps::BuildAllBonemaps();
    }

    cvar->ConsoleColorPrintf(ColorRGBA(0, 225, 0), XORSTR("\nMissedIT Successfully loaded.\n"));
}
/* Entrypoint to the Library. Called when loading */
int __attribute__((constructor)) Startup()
{
	std::thread mainThread(MainThread);
	// The root of all suffering is attachment
	// Therefore our little buddy must detach from this realm.
	// Farewell my thread, may we join again some day..
	mainThread.detach();

	return 0;
}
/* Called when un-injecting the library */
void __attribute__((destructor)) Shutdown()
{
    if( Settings::SkyBox::enabled ){
        SetNamedSkyBox( cvar->FindVar("sv_skyname")->strValue );
    }
    cvar->FindVar(XORSTR("cl_mouseenable"))->SetValue(1);

	SDL2::UnhookWindow();
	SDL2::UnhookPollEvent();

	NoSmoke::Cleanup();
	TracerEffect::RestoreTracers();

    for( VMT* vmt : createdVMTs ){
        delete vmt;
    }

    input->m_fCameraInThirdPerson = false;
	input->m_vecCameraOffset.z = 150.f;
	GetLocalClient(-1)->m_nDeltaTick = -1;

	delete eventListener;

	*s_bOverridePostProcessingDisable = false;

	cvar->ConsoleColorPrintf(ColorRGBA(255, 0, 0), XORSTR("MissedIT Unloaded successfully.\n"));
}
/*
added option to enable / disable backtrack chams
enhanced resolver:
-More proper legit aa / legit detection
-basic lby, low delta and no desync detection
-attempted to make a more proper legit aa resolver
fixed legit aa on e still doing at targets
enhanced legitbot
attempted to make ragebot's "CanShoot" function more accurate 
added safety mode to slowwalk
added option to reset misses to player list
readded adaptive option to fakelag which should help with breaking lagcomp
fixed indicators and added lc indicator
updated multipoint code
added wait for onshot on key
added proper buybot
NOTE: NOT EVERYTHING IS DONE YET. TAKE THIS COMMIT AS A PART 1.
*/

