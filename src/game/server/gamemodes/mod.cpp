
#include "mod.h"
#include "base/system.h"
#include "engine/server.h"
#include "game/generated/protocol.h"
#include "game/generated/protocol7.h"
#include "game/server/teams.h"
#include "game/teamscore.h"

#include <engine/shared/protocol.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>
//#include <base/system.h>

#include <game/mapitems.h>
#include <game/server/entities/character.h>

//debug
#include <base/log.h>

#define GAME_TYPE_NAME "Cup"
#define TEST_TYPE_NAME "TestCup"
#define WARMUP_TIME 60
#define COUNTDOWN 3
#define END_ROUND_TIMER 20
#define BROADCAST_TIME 6
#define ELIMINATED_TEAM -1 //if this changes to actual team, will have to add SetForceCharacterTeam

#include <unistd.h>

CGameControllerCup::CGameControllerCup(class CGameContext *pGameServer) :
	IGameController(pGameServer)
{
	m_pGameType = g_Config.m_SvTestingCommands ? TEST_TYPE_NAME : GAME_TYPE_NAME;

	StartCup(WARMUP_TIME);
}

CGameControllerCup::~CGameControllerCup() = default;

int CGameControllerCup::GetState() const
{
	return m_CupState;
}

void CGameControllerCup::PausePlayersTune()
{
	GameServer()->Tuning()->Set("hook_drag_speed", 0);
	GameServer()->Tuning()->Set("ground_control_speed", 0);
	GameServer()->Tuning()->Set("air_control_speed", 0);
	GameServer()->Tuning()->Set("ground_jump_impulse", 0);
	GameServer()->Tuning()->Set("air_jump_impulse", 0);
	GameServer()->Tuning()->Set("gravity", 0);
	GameServer()->Tuning()->Set("player_collision", 0);
	GameServer()->SendTuningParams(-1);
}

void CGameControllerCup::PrepareRound()
{
	//if only 1 or no one has qualified.
	if (m_PlayerLeaderboard.size() <= 1)
		EndRound();
	else
	{
		m_CupState = STATE_WARMUP_ROUND;
		
		CleanUp();
		DoWarmup(COUNTDOWN);
		PausePlayersTune();

		//reset stats
		for (auto &playerInfo : m_PlayerLeaderboard)
		{
			for (int i = 0; i < MAX_CHECKPOINTS; i++)
				playerInfo.m_CurrentTimeCP[0] = 0.0f;
			playerInfo.m_AmountOfTimeCPs = 0;
			playerInfo.m_HasFinished = false;
		}

		//Do I really want this?
		Server()->DemoRecorder_HandleAutoStart();
	}
}

void CGameControllerCup::StartRound()
{	
	GameServer()->ResetTuning();
	m_CupState = STATE_ROUND;
	m_RoundStartTick = Server()->Tick();
}

void CGameControllerCup::StartCup(int WarmupTime)
{
	m_CupState = STATE_WARMUP;
	m_RoundStartTick = 0;

	CleanUp();
	m_PlayerLeaderboard.clear();
	GameServer()->ResetTuning();

	char aBuf[256];
	if (WarmupTime)
	{
		DoWarmup(WarmupTime);
		str_format(aBuf, sizeof(aBuf), "Qualifications have started. You have %d secondes left to qualify and make it to the elimination rounds!", WarmupTime);
	}
	else
	{
		for(int ClientId = 0; ClientId < Server()->MaxClients(); ClientId++)
		{
			if(Server()->ClientIngame(ClientId))
			{
				m_PlayerLeaderboard.emplace_back(Sm_PlayersInfo{
					Server()->ClientName(ClientId),
				});
			}
		}
		m_Warmup = 0;
		str_copy(aBuf, "No qualifications! Cup has started!!! GOGOGOGOOO");
	}

	GameServer()->SendBroadcast(aBuf, -1);
}

void CGameControllerCup::SendtWarmupMsg()
{
	if (!(m_Warmup == 60 * SERVER_TICK_SPEED
		|| m_Warmup == 30 * SERVER_TICK_SPEED
		|| m_Warmup == 10 * SERVER_TICK_SPEED
		|| m_Warmup == 3 * SERVER_TICK_SPEED
		|| m_Warmup == 2 * SERVER_TICK_SPEED
		|| m_Warmup == 1* SERVER_TICK_SPEED))
		return ;

	char aBuf[128];
	int Seconds = m_Warmup / SERVER_TICK_SPEED;

	str_format(aBuf, sizeof(aBuf), "%d seconds of qualifications left", Seconds);
	GameServer()->SendChat(-1, protocol7::CHAT_ALL, aBuf);
}

void CGameControllerCup::Tick()
{
	IGameController::Tick();
	Teams().ProcessSaveTeam();
	Teams().Tick();

	//warmup
	if (m_CupState == STATE_WARMUP && m_Warmup)
		SendtWarmupMsg();
	else if (m_CupState == STATE_WARMUP && !m_Warmup)
		PrepareRound();
	else if (m_CupState == STATE_WARMUP_ROUND && !m_Warmup)
		StartRound();
	else if (m_CupState == STATE_ROUND && !m_Warmup)
		EndRound();
	if (!m_Warmup)
		m_Warmup = -1;
}

CScore *CGameControllerCup::Score()
{
	return GameServer()->Score();
}

void CGameControllerCup::OnPlayerConnect(CPlayer *pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientId = pPlayer->GetCid();

	//init the player?
	Score()->PlayerData(ClientId)->Reset();

	if(!Server()->ClientPrevIngame(ClientId))
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientId), GetTeamName(pPlayer->GetTeam()));
		GameServer()->SendChat(-1, protocol7::CHAT_ALL, aBuf, -1, CGameContext::FLAG_SIX);

		GameServer()->SendChatTarget(ClientId, "CupOfTheWeek. Version: " GAME_VERSION);
	}

	//this doesn't work. Sends to console instead
	if (m_CupState == STATE_WARMUP && m_Warmup)
	{
		int Seconds = m_Warmup / Server()->TickSpeed();
		GameServer()->SendChatTarget(ClientId, "Qualifications have already started. But don't worry!! You still have %d seconds of left to qualify", Seconds);
	}

	if (m_CupState == STATE_NONE)
		Score()->LoadPlayerData(ClientId);

	if (m_CupState == STATE_ROUND)
	{
		auto PlayerInfo = GetPlayerByName(Server()->ClientName(ClientId));
		if (PlayerInfo == m_PlayerLeaderboard.end())
		{
			GameServer()->SendChatTarget(ClientId, "Cup has already started :c. You will be able to play once it ends");
			pPlayer->SetTeam(ELIMINATED_TEAM);
		}
	}

	//set to previous team idea
	// if (m_RoundStarted)
	// {
	// 	const char *clientName = Server()->ClientName(ClientId);
	// 	if (auto player = m_disconnectedPlayers.find(clientName); player != m_disconnectedPlayers.end())
	// 		Teams().SetForceCharacterTeam(ClientId, player->second);
	// }
}

//get previous team idea
// void CGameControllerCup::OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason)
// {
// 	int ClientId = pPlayer->GetCid();
// 	const int teamId = GameServer()->GetDDRaceTeam(ClientId);

// 	IGameController::OnPlayerDisconnect(pPlayer, pReason);

// 	if (m_RoundStarted)
// 	{
// 		const char *clientName = Server()->ClientName(ClientId);

// 		m_disconnectedPlayers.insert({clientName, teamId});
// 	}	
// }

void CGameControllerCup::RemoveEliminatedPlayers()
{
	if (m_PlayerLeaderboard.size() <= 1)
		return ;

	std::sort(m_PlayerLeaderboard.begin(), m_PlayerLeaderboard.end(), SplitsComparator);

	for (const auto& playera : m_PlayerLeaderboard)
		dbg_msg("LEADERBOARDPRINT", "\nname : %s\n amount of cps : %i\n timecps : %f\n has finishes : %i\n\n", playera.m_PlayerName.c_str(), playera.m_AmountOfTimeCPs, playera.m_HasFinished);
	dbg_msg("LEADERBOARD", "\n\n");

	int size = m_PlayerLeaderboard.size();

	char aBuf[256];
	for (int i = 0; i < (size/4) + 1 ; i++)
	{
		str_format(aBuf, sizeof(aBuf), "%s has been eliminated\n", m_PlayerLeaderboard.back().m_PlayerName.c_str());
		GameServer()->SendChat(-1, protocol7::CHAT_ALL, aBuf);
		m_PlayerLeaderboard.pop_back();
	}

	//remove player if they haven't finished
	while (!m_PlayerLeaderboard.back().m_HasFinished && !m_PlayerLeaderboard.empty())
		m_PlayerLeaderboard.pop_back();
}

//needs a better name
void CGameControllerCup::CleanUp()
{
	for(int ClientId = 0; ClientId < Server()->MaxClients(); ClientId++)
	{
		if(Server()->ClientIngame(ClientId))
		{
			auto playerInfo = GetPlayerByName(Server()->ClientName(ClientId));
			CPlayer *pPlayer = Teams().GetPlayer(ClientId);
			//this should never proc I think
			if (!pPlayer)
				continue ;

			//put player into spec or not when in rounds.
			if (m_CupState == STATE_WARMUP_ROUND) // (|| STATE_ROUND ) for convenience?
			{
				if (playerInfo == m_PlayerLeaderboard.end() && pPlayer->GetTeam() != ELIMINATED_TEAM) //CRASH HERE AT SECOND ROUND
					pPlayer->SetTeam(ELIMINATED_TEAM);
				else if (playerInfo != m_PlayerLeaderboard.end() && pPlayer->GetTeam() == ELIMINATED_TEAM)
				 	pPlayer->SetTeam(TEAM_FLOCK);
			}

			//Put Player into any playable team once cup ends.
			if (m_CupState == STATE_NONE || m_CupState == STATE_WARMUP)
			{
				if (pPlayer->GetTeam() == ELIMINATED_TEAM)
					pPlayer->SetTeam(TEAM_FLOCK);
			}

			//set score back if end Otherwise remove score
			if (m_CupState == STATE_NONE)
				Score()->LoadPlayerData(ClientId);
			else
				pPlayer->m_Score.reset();
		}
	}

	ResetGame();
}

void CGameControllerCup::EndRound()
{
	RemoveEliminatedPlayers();

	if (m_PlayerLeaderboard.size() <= 1)
	{
		char aBuf[256];
		m_CupState = STATE_NONE;

		if (m_PlayerLeaderboard.size() == 1)
		{
			str_format(aBuf, sizeof(aBuf), "%s IS THE WINNER!!", m_PlayerLeaderboard.front().m_PlayerName.c_str());
			GameServer()->SendBroadcast(aBuf, -1);
		}
		else {
			GameServer()->SendBroadcast("No one managed to qualify O.o", -1);
		}

		CleanUp();
	}
	else
		PrepareRound();
}

auto CGameControllerCup::GetPlayerByName(const char* PlayerName)
    -> decltype(m_PlayerLeaderboard.begin())
{
	return std::find_if(m_PlayerLeaderboard.begin(), m_PlayerLeaderboard.end(),
					[PlayerName](const Sm_PlayersInfo& PlayerLeaderboard){
						return !str_comp(PlayerName, PlayerLeaderboard.m_PlayerName.c_str());
					});
}

bool CGameControllerCup::SplitsComparator(const Sm_PlayersInfo& player1, Sm_PlayersInfo& player2) {

    if (player1.m_HasFinished != player2.m_HasFinished)
        return player1.m_HasFinished;

    if (player1.m_AmountOfTimeCPs == player2.m_AmountOfTimeCPs)
	{
		int TimeCheckpoint = 0;
		while (player1.m_CurrentTimeCP[TimeCheckpoint] && TimeCheckpoint < MAX_CHECKPOINTS)
			TimeCheckpoint++;
        return player1.m_CurrentTimeCP[TimeCheckpoint] < player2.m_CurrentTimeCP[TimeCheckpoint];
	}

    return player1.m_AmountOfTimeCPs > player2.m_AmountOfTimeCPs;
}

void CGameControllerCup::SetSplits(CPlayer *pThisPlayer, int TimeCheckpoint)
{
	const char *PlayerName = Server()->ClientName(pThisPlayer->GetCid());
	CCharacter *pChr = pThisPlayer->GetCharacter();
	if (!pChr) //probably useless check, but who knows
		return ;

	auto PlayerNameIt = GetPlayerByName(PlayerName);

	if(TimeCheckpoint > -1 && pChr->m_DDRaceState == DDRACE_STARTED && PlayerNameIt->m_CurrentTimeCP[TimeCheckpoint] == 0.0f && pChr->GetTime() != 0.0f)
	{
		PlayerNameIt->m_CurrentTimeCP[TimeCheckpoint] = pChr->GetTime();
		PlayerNameIt->m_AmountOfTimeCPs++;

		//dbg_msg("TIMECP", "PLAYER NAME IS %s\nTIMECP %i\nCURRENT CP TIME IS %f\n CURRENT TIME IS %f", PlayerNameIt->m_PlayerName.c_str(), TimeCheckpoint, PlayerNameIt->m_CurrentTimeCP[TimeCheckpoint], pChr->GetTime());

		std::sort(m_PlayerLeaderboard.begin(), m_PlayerLeaderboard.end(), SplitsComparator);
	}
}

void CGameControllerCup::CupOnPlayerFinish(int ClientId)
{
	//TODO add team support.

	const char* PlayerName = Server()->ClientName(ClientId);
	auto player = GetPlayerByName(PlayerName);

	if (m_CupState == STATE_WARMUP && player == m_PlayerLeaderboard.end())
		m_PlayerLeaderboard.emplace_back(Sm_PlayersInfo{
			Server()->ClientName(ClientId),
		});

	else if (m_CupState == STATE_ROUND)
	{
		//this should never proc?
		if (m_PlayerLeaderboard.empty())
		{
			dbg_msg("COTW OnPlayerFinish", "ALARM no players?");
			return ;
		}

		//first player finishing
		if (m_PlayerLeaderboard.front().m_HasFinished == false)
			DoWarmup(END_ROUND_TIMER);

		player->m_HasFinished = true;
		std::sort(m_PlayerLeaderboard.begin(), m_PlayerLeaderboard.end(), SplitsComparator);

		//last player finishing
		if (m_PlayerLeaderboard.back().m_HasFinished == true)
		{
			m_Warmup = -1;
			EndRound();
		}
	}
}

// bool CGameControllerCup::CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize)
// {
// 	//TODO DO SOMETHING WITH THIS SO IT MAKES SENSE

// 	// str_copy(
// 	// 	pErrorReason,
// 	// 	"COTW has started! You can't change yout team while the game is in process.",
// 	// 	ErrorReasonSize);
// 	// return false;
// 	return true;
// }

void CGameControllerCup::HandleCharacterTiles(CCharacter *pChr, int MapIndex)
{
	CPlayer *pPlayer = pChr->GetPlayer();
	const int ClientId = pPlayer->GetCid();

	int TileIndex = GameServer()->Collision()->GetTileIndex(MapIndex);
	int TileFIndex = GameServer()->Collision()->GetFTileIndex(MapIndex);

	//Sensitivity
	int S1 = GameServer()->Collision()->GetPureMapIndex(vec2(pChr->GetPos().x + pChr->GetProximityRadius() / 3.f, pChr->GetPos().y - pChr->GetProximityRadius() / 3.f));
	int S2 = GameServer()->Collision()->GetPureMapIndex(vec2(pChr->GetPos().x + pChr->GetProximityRadius() / 3.f, pChr->GetPos().y + pChr->GetProximityRadius() / 3.f));
	int S3 = GameServer()->Collision()->GetPureMapIndex(vec2(pChr->GetPos().x - pChr->GetProximityRadius() / 3.f, pChr->GetPos().y - pChr->GetProximityRadius() / 3.f));
	int S4 = GameServer()->Collision()->GetPureMapIndex(vec2(pChr->GetPos().x - pChr->GetProximityRadius() / 3.f, pChr->GetPos().y + pChr->GetProximityRadius() / 3.f));
	int Tile1 = GameServer()->Collision()->GetTileIndex(S1);
	int Tile2 = GameServer()->Collision()->GetTileIndex(S2);
	int Tile3 = GameServer()->Collision()->GetTileIndex(S3);
	int Tile4 = GameServer()->Collision()->GetTileIndex(S4);
	int FTile1 = GameServer()->Collision()->GetFTileIndex(S1);
	int FTile2 = GameServer()->Collision()->GetFTileIndex(S2);
	int FTile3 = GameServer()->Collision()->GetFTileIndex(S3);
	int FTile4 = GameServer()->Collision()->GetFTileIndex(S4);

	const int PlayerDDRaceState = pChr->m_DDRaceState;
	bool IsOnStartTile = (TileIndex == TILE_START) || (TileFIndex == TILE_START) || FTile1 == TILE_START || FTile2 == TILE_START || FTile3 == TILE_START || FTile4 == TILE_START || Tile1 == TILE_START || Tile2 == TILE_START || Tile3 == TILE_START || Tile4 == TILE_START;
	// start
	if(IsOnStartTile && PlayerDDRaceState != DDRACE_CHEAT)
	{
		const int Team = GameServer()->GetDDRaceTeam(ClientId);
		if(Teams().GetSaving(Team))
		{
			GameServer()->SendStartWarning(ClientId, "You can't start while loading/saving of team is in progress");
			pChr->Die(ClientId, WEAPON_WORLD);
			return;
		}
		if(g_Config.m_SvTeam == SV_TEAM_MANDATORY && (Team == TEAM_FLOCK || Teams().Count(Team) <= 1))
		{
			GameServer()->SendStartWarning(ClientId, "You have to be in a team with other tees to start");
			pChr->Die(ClientId, WEAPON_WORLD);
			return;
		}
		if(g_Config.m_SvTeam != SV_TEAM_FORCED_SOLO && Team > TEAM_FLOCK && Team < TEAM_SUPER && Teams().Count(Team) < g_Config.m_SvMinTeamSize && !Teams().TeamFlock(Team))
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "Your team has fewer than %d players, so your team rank won't count", g_Config.m_SvMinTeamSize);
			GameServer()->SendStartWarning(ClientId, aBuf);
		}
		if(g_Config.m_SvResetPickups)
		{
			pChr->ResetPickups();
		}

		Teams().OnCharacterStart(ClientId);

		//my changes
		if (m_CupState == STATE_ROUND)
			pChr->m_StartTime = m_RoundStartTick;

		pChr->m_LastTimeCp = -1;
		pChr->m_LastTimeCpBroadcasted = -1;
		for(float &CurrentTimeCp : pChr->m_aCurrentTimeCp)
		{
			CurrentTimeCp = 0.0f;
		}
	}

	// finish
	if(((TileIndex == TILE_FINISH) || (TileFIndex == TILE_FINISH) || FTile1 == TILE_FINISH || FTile2 == TILE_FINISH || FTile3 == TILE_FINISH || FTile4 == TILE_FINISH || Tile1 == TILE_FINISH || Tile2 == TILE_FINISH || Tile3 == TILE_FINISH || Tile4 == TILE_FINISH) && PlayerDDRaceState == DDRACE_STARTED)
	{
		Teams().OnCharacterFinish(ClientId);
		CupOnPlayerFinish(ClientId);
	}

	// unlock team
	else if(((TileIndex == TILE_UNLOCK_TEAM) || (TileFIndex == TILE_UNLOCK_TEAM)) && Teams().TeamLocked(GameServer()->GetDDRaceTeam(ClientId)))
	{
		Teams().SetTeamLock(GameServer()->GetDDRaceTeam(ClientId), false);
		GameServer()->SendChatTeam(GameServer()->GetDDRaceTeam(ClientId), "Your team was unlocked by an unlock team tile");
	}

	// solo part
	if(((TileIndex == TILE_SOLO_ENABLE) || (TileFIndex == TILE_SOLO_ENABLE)) && !Teams().m_Core.GetSolo(ClientId))
	{
		GameServer()->SendChatTarget(ClientId, "You are now in a solo part");
		pChr->SetSolo(true);
	}
	else if(((TileIndex == TILE_SOLO_DISABLE) || (TileFIndex == TILE_SOLO_DISABLE)) && Teams().m_Core.GetSolo(ClientId))
	{
		GameServer()->SendChatTarget(ClientId, "You are now out of the solo part");
		pChr->SetSolo(false);
	}

	//splits - should be now useless? Maybe only needed for dbg dummies
	if (m_CupState == STATE_ROUND)
	{
		SetSplits(pPlayer, GameServer()->Collision()->IsTimeCheckpoint(MapIndex));
		SetSplits(pPlayer, GameServer()->Collision()->IsFTimeCheckpoint(MapIndex)); //why do I need this?
	}
}

void CGameControllerCup::Snap(int SnappingClient)
{
	CNetObj_GameInfo *pGameInfoObj = Server()->SnapNewItem<CNetObj_GameInfo>(0);
	if(!pGameInfoObj)
		return;

	pGameInfoObj->m_GameFlags = m_GameFlags;
	pGameInfoObj->m_GameStateFlags = 0;
	if(m_GameOverTick != -1)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_GAMEOVER;
	if(m_SuddenDeath)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_SUDDENDEATH;
	if(GameServer()->m_World.m_Paused)
		pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_PAUSED;

	pGameInfoObj->m_RoundStartTick = m_RoundStartTick;

	//my changes
	if (m_Warmup > 0)
		pGameInfoObj->m_WarmupTimer = m_Warmup - Server()->Tick() + Server()->TickSpeed();
	else if (m_Warmup == -1)
		pGameInfoObj->m_WarmupTimer = -m_RoundStartTick;

	pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_RACETIME;

	pGameInfoObj->m_RoundNum = 0;
	pGameInfoObj->m_RoundCurrent = m_RoundCount + 1;

	CCharacter *pChr;
	CPlayer *pPlayer = SnappingClient != SERVER_DEMO_CLIENT ? GameServer()->m_apPlayers[SnappingClient] : 0;
	CPlayer *pPlayer2;

	if(pPlayer && (pPlayer->m_TimerType == CPlayer::TIMERTYPE_GAMETIMER || pPlayer->m_TimerType == CPlayer::TIMERTYPE_GAMETIMER_AND_BROADCAST) && pPlayer->GetClientVersion() >= VERSION_DDNET_GAMETICK)
	{
		if((pPlayer->GetTeam() == TEAM_SPECTATORS || pPlayer->IsPaused()) && pPlayer->m_SpectatorId != SPEC_FREEVIEW && (pPlayer2 = GameServer()->m_apPlayers[pPlayer->m_SpectatorId]))
		{
			if((pChr = pPlayer2->GetCharacter()) && pChr->m_DDRaceState == DDRACE_STARTED)
			{
				pGameInfoObj->m_WarmupTimer = -pChr->m_StartTime;
				pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_RACETIME;
			}
		}
		else if((pChr = pPlayer->GetCharacter()) && pChr->m_DDRaceState == DDRACE_STARTED)
		{
			pGameInfoObj->m_WarmupTimer = -pChr->m_StartTime;
			pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_RACETIME;
		}
	}

	CNetObj_GameInfoEx *pGameInfoEx = Server()->SnapNewItem<CNetObj_GameInfoEx>(0);
	if(!pGameInfoEx)
		return;

	pGameInfoEx->m_Flags =
		GAMEINFOFLAG_TIMESCORE |
		GAMEINFOFLAG_GAMETYPE_RACE |
		GAMEINFOFLAG_GAMETYPE_DDRACE |
		GAMEINFOFLAG_GAMETYPE_DDNET |
		GAMEINFOFLAG_UNLIMITED_AMMO |
		GAMEINFOFLAG_RACE_RECORD_MESSAGE |
		GAMEINFOFLAG_ALLOW_EYE_WHEEL |
		GAMEINFOFLAG_ALLOW_HOOK_COLL |
		GAMEINFOFLAG_ALLOW_ZOOM |
		GAMEINFOFLAG_BUG_DDRACE_GHOST |
		GAMEINFOFLAG_BUG_DDRACE_INPUT |
		GAMEINFOFLAG_PREDICT_DDRACE |
		GAMEINFOFLAG_PREDICT_DDRACE_TILES |
		GAMEINFOFLAG_ENTITIES_DDNET |
		GAMEINFOFLAG_ENTITIES_DDRACE |
		GAMEINFOFLAG_ENTITIES_RACE |
		GAMEINFOFLAG_RACE;
	pGameInfoEx->m_Flags2 = GAMEINFOFLAG2_HUD_DDRACE;
	if(g_Config.m_SvNoWeakHook)
		pGameInfoEx->m_Flags2 |= GAMEINFOFLAG2_NO_WEAK_HOOK;
	pGameInfoEx->m_Version = GAMEINFO_CURVERSION;

	if(Server()->IsSixup(SnappingClient))
	{
		protocol7::CNetObj_GameData *pGameData = Server()->SnapNewItem<protocol7::CNetObj_GameData>(0);
		if(!pGameData)
			return;

		pGameData->m_GameStartTick = m_RoundStartTick;
		pGameData->m_GameStateFlags = 0;
		if(m_GameOverTick != -1)
			pGameData->m_GameStateFlags |= protocol7::GAMESTATEFLAG_GAMEOVER;
		if(m_SuddenDeath)
			pGameData->m_GameStateFlags |= protocol7::GAMESTATEFLAG_SUDDENDEATH;
		if(GameServer()->m_World.m_Paused)
			pGameData->m_GameStateFlags |= protocol7::GAMESTATEFLAG_PAUSED;

		pGameData->m_GameStateEndTick = 0;

		protocol7::CNetObj_GameDataRace *pRaceData = Server()->SnapNewItem<protocol7::CNetObj_GameDataRace>(0);
		if(!pRaceData)
			return;

		pRaceData->m_BestTime = round_to_int(m_CurrentRecord * 1000);
		pRaceData->m_Precision = 2;
		pRaceData->m_RaceFlags = protocol7::RACEFLAG_KEEP_WANTED_WEAPON;
	}

	GameServer()->SnapSwitchers(SnappingClient);
}
