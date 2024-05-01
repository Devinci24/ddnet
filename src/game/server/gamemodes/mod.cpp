
#include "mod.h"

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
#define WU_TIMER 60
#define START_ROUND_TIMER 3
#define END_ROUND_TIMER 15

#include <unistd.h>

CGameControllerCup::CGameControllerCup(class CGameContext *pGameServer) :
	IGameController(pGameServer)
{
	m_pGameType = g_Config.m_SvTestingCommands ? TEST_TYPE_NAME : GAME_TYPE_NAME;

	m_RoundStarted = false;
	m_SomeoneHasFinished = false;
	m_IsFirstRound = true;
	m_FirstFinisherTick = -1;
	m_NumberOfPlayerLeft = -1;
	m_StopAll = false;
	m_TunesOn = false;
}

CGameControllerCup::~CGameControllerCup() = default;

void CGameControllerCup::m_fnRestartCup()
{
	m_RoundStarted = false;
	m_SomeoneHasFinished = false;
	m_IsFirstRound = true;
	m_FirstFinisherTick = -1;
	m_NumberOfPlayerLeft = -1;
	m_StopAll = false;
	m_TunesOn = false;
	ResetGame();
}

void CGameControllerCup::m_fnStartRoundTimer(int Seconds)
{
	ResetGame();
	DoWarmup(Seconds);
	m_RoundStarted = true;
}

bool CGameControllerCup::m_fnDoesElementExist(const int ClientId)
{
	int count = std::count_if(m_RoundScores.begin(), m_RoundScores.end(), [&](const auto& pair) {
		return pair.first == ClientId;
	});
	return count > 0;
}

int CGameControllerCup::m_fnGetIndexOfElement(const int ClientId)
{
    auto it = std::find_if(m_RoundScores.begin(), m_RoundScores.end(), [&](const auto& pair) {
        return pair.first == ClientId;
    });

    if (it != m_RoundScores.end())
        return std::distance(m_RoundScores.begin(), it);
    else
        return -1;
}

void CGameControllerCup::m_fnSendTimeLeftWarmupMsg()
{
	if (!(m_Warmup == 60 * SERVER_TICK_SPEED
		|| m_Warmup == 30 * SERVER_TICK_SPEED
		|| m_Warmup == 10 * SERVER_TICK_SPEED
		|| m_Warmup == 3 * SERVER_TICK_SPEED
		|| m_Warmup == 2 * SERVER_TICK_SPEED
		|| m_Warmup == 1* SERVER_TICK_SPEED))
		return ;

	char aBuf[64];
	int Seconds = m_Warmup / SERVER_TICK_SPEED;

	str_format(aBuf, sizeof(aBuf), "%d seconds of warmup left", Seconds);
	GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
}

void CGameControllerCup::m_fnPauseServer()
{
	GameServer()->Tuning()->Set("hook_drag_speed", 0);
	GameServer()->Tuning()->Set("ground_control_speed", 0);
	GameServer()->Tuning()->Set("air_control_speed", 0);
	GameServer()->Tuning()->Set("ground_jump_impulse", 0);
	GameServer()->Tuning()->Set("air_jump_impulse", 0);
	GameServer()->SendTuningParams(-1);
	m_TunesOn = true;
}

void CGameControllerCup::Tick()
{
	if (m_Warmup <= 0 && !m_RoundStarted && m_IsFirstRound) DoWarmup(WU_TIMER);
	if (m_Warmup && m_RoundStarted && m_TunesOn && m_FirstFinisherTick == -1)
		m_fnPauseServer();
	else if (!m_Warmup && m_TunesOn == true)
	{
		GameServer()->ResetTuning();
		m_TunesOn = false;
	}

	if (m_RoundStarted)
	{
		for(int ClientId = 0; ClientId < Server()->MaxClients(); ClientId++)
		{
			if(Server()->ClientIngame(ClientId))
			{
				CPlayer *pPlayer = Teams().GetPlayer(ClientId);
				if (pPlayer->m_Score.has_value() || !m_fnDoesElementExist(ClientId))
					pPlayer->SetTeam(TEAM_SPECTATORS);
			}
		}
	}


	if (m_FirstFinisherTick != -1 && m_FirstFinisherTick + END_ROUND_TIMER * SERVER_TICK_SPEED <= Server()->Tick())
		EndRound();

	// do warmup
	if(m_Warmup)
	{
		m_Warmup--;
		if(!m_RoundStarted)
			m_fnSendTimeLeftWarmupMsg();
		
		if(!m_Warmup && !m_RoundStarted) //only procs first round
			StartRound();
	}

	if(m_pLoadBestTimeResult != nullptr && m_pLoadBestTimeResult->m_Completed)
	{
		if(m_pLoadBestTimeResult->m_Success)
		{
			m_CurrentRecord = m_pLoadBestTimeResult->m_CurrentRecord;

			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(GameServer()->m_apPlayers[i] && GameServer()->m_apPlayers[i]->GetClientVersion() >= VERSION_DDRACE)
				{
					GameServer()->SendRecord(i);
				}
			}
		}
		m_pLoadBestTimeResult = nullptr;
	}

	DoActivityCheck();
}

CScore *CGameControllerCup::Score()
{
	return GameServer()->Score();
}

void CGameControllerCup::OnPlayerConnect(CPlayer *pPlayer)
{
	IGameController::OnPlayerConnect(pPlayer);
	int ClientId = pPlayer->GetCid();

	// init the player
	Score()->PlayerData(ClientId)->Reset();

	if(!Server()->ClientPrevIngame(ClientId))
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientId), GetTeamName(pPlayer->GetTeam()));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf, -1, CGameContext::CHAT_SIX);

		GameServer()->SendChatTarget(ClientId, "CupOfTheWeek. Version: " GAME_VERSION);
	}
}

void CGameControllerCup::DoWarmup(int Seconds)
{
	IGameController::DoWarmup(Seconds);
}

void CGameControllerCup::m_fnRemoveEliminatedPlayers()
{

	if (m_IsFirstRound)
	{
		m_IsFirstRound = false;
	// 	char aBuf[32];
	// 	str_format(aBuf, sizeof(aBuf), "No Eliminations this Round");
	// 	GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
	// 	return ;
	}

	int AmountOfPlayersToRemove;

	if (m_NumberOfPlayerLeft > 12)
		AmountOfPlayersToRemove = 4;
	else if (m_NumberOfPlayerLeft > 5)
		AmountOfPlayersToRemove = 2;
	else
		AmountOfPlayersToRemove = 1;

	std::sort(m_RoundScores.begin(), m_RoundScores.end(), [](auto &left, auto &right) {
		return left.second < right.second;
	});
	
	for (auto it = m_RoundScores.end() - 1; it != m_RoundScores.begin() - 1; --it)
	{
		char aBuf[64];

		if (it->second == 999999.0f)
		{
			it = m_RoundScores.erase(it);
			str_format(aBuf, sizeof(aBuf), "%s has been eliminated", Server()->ClientName(it->first));
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
			continue ;
		}

		if (m_NumberOfPlayerLeft - (int)m_RoundScores.size() >= AmountOfPlayersToRemove)
			break;
		else
		{
			it = m_RoundScores.erase(it);
			str_format(aBuf, sizeof(aBuf), "%s has been eliminated", Server()->ClientName(it->first));
			GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
		}
	}

	m_NumberOfPlayerLeft = m_RoundScores.size();
	if (m_NumberOfPlayerLeft == 1)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s is the winner!", Server()->ClientName(m_RoundScores[0].first));
		GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
		m_StopAll = true;
	}
}

void CGameControllerCup::EndRound()
{
	m_RoundStarted = false;
	m_FirstFinisherTick = -1;

	m_fnRemoveEliminatedPlayers();

	for(int ClientId = 0; ClientId < Server()->MaxClients(); ClientId++)
	{
		if(Server()->ClientIngame(ClientId))
		{
			CPlayer *pPlayer = Teams().GetPlayer(ClientId);
			if (pPlayer->GetTeam() == TEAM_SPECTATORS && m_fnDoesElementExist(ClientId))
				pPlayer->SetTeam(TEAM_FLOCK);
			else if (pPlayer->GetTeam() == TEAM_FLOCK && !m_fnDoesElementExist(ClientId))
				pPlayer->SetTeam(TEAM_SPECTATORS);
		}
	}
	if (!m_StopAll)
		StartRound();
}

void CGameControllerCup::StartRound()
{
	m_fnStartRoundTimer(START_ROUND_TIMER);
	m_SomeoneHasFinished = false;
	m_RoundStarted = true;
	m_RoundStartTick = Server()->Tick() + START_ROUND_TIMER * SERVER_TICK_SPEED;
	m_GameOverTick = -1;

	if (m_IsFirstRound)
		m_NumberOfPlayerLeft = m_RoundScores.size();

	for(int ClientId = 0; ClientId < Server()->MaxClients(); ClientId++)
	{
		if(Server()->ClientIngame(ClientId))
		{
			CPlayer *pPlayer = Teams().GetPlayer(ClientId);
			pPlayer->m_Score.reset();
		}
	}

	for (auto it = m_RoundScores.begin(); it != m_RoundScores.end(); it++)
		it->second = 999999.0f;

	Server()->DemoRecorder_HandleAutoStart();
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "start round type='%s' teamplay='%d'", m_pGameType, m_GameFlags & GAMEFLAG_TEAMS);
	GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
}


void CGameControllerCup::HandleCharacterTiles(CCharacter *pChr, int MapIndex)
{
	CPlayer *pPlayer = pChr->GetPlayer();
	const int ClientId = pPlayer->GetCid();

	int m_TileIndex = GameServer()->Collision()->GetTileIndex(MapIndex);
	int m_TileFIndex = GameServer()->Collision()->GetFTileIndex(MapIndex);

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
	bool IsOnStartTile = (m_TileIndex == TILE_START) || (m_TileFIndex == TILE_START) || FTile1 == TILE_START || FTile2 == TILE_START || FTile3 == TILE_START || FTile4 == TILE_START || Tile1 == TILE_START || Tile2 == TILE_START || Tile3 == TILE_START || Tile4 == TILE_START;
	// start
	if(IsOnStartTile && PlayerDDRaceState != DDRACE_CHEAT)
	{
		const int Team = GameServer()->GetDDRaceTeam(ClientId);
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
		pChr->m_LastTimeCp = -1;
		pChr->m_LastTimeCpBroadcasted = -1;
		for(float &CurrentTimeCp : pChr->m_aCurrentTimeCp)
		{
			CurrentTimeCp = 0.0f;
		}
	}

	// finish
	if(((m_TileIndex == TILE_FINISH) || (m_TileFIndex == TILE_FINISH) || FTile1 == TILE_FINISH || FTile2 == TILE_FINISH || FTile3 == TILE_FINISH || FTile4 == TILE_FINISH || Tile1 == TILE_FINISH || Tile2 == TILE_FINISH || Tile3 == TILE_FINISH || Tile4 == TILE_FINISH) && PlayerDDRaceState == DDRACE_STARTED)
	{
		Teams().OnCharacterFinish(ClientId);
		
		if (m_RoundStarted)
		{
			if (!m_SomeoneHasFinished)
			{
				m_FirstFinisherTick = Server()->Tick();
				DoWarmup(END_ROUND_TIMER);
				char aBuf[128];
		
				str_format(aBuf, sizeof(aBuf), "%s has finished first! You have %i seconds remaining until DNF", Server()->ClientName(ClientId), END_ROUND_TIMER);
				GameServer()->SendChat(-1, CGameContext::CHAT_ALL, aBuf);
			}
			
		}
		float Time = (float)(Server()->Tick() - Teams().GetStartTime(pPlayer)) / ((float)Server()->TickSpeed());
		if (!m_RoundStarted)
			m_RoundScores.emplace_back(ClientId, Time);
		else
		{
			int index = m_fnGetIndexOfElement(ClientId);
			if (index != -1)
			{
				m_RoundScores[index].first = ClientId;
				m_RoundScores[index].second = Time;
			}
		}
		for (auto it = m_RoundScores.end() - 1; it != m_RoundScores.begin() - 1; --it)
		{
			log_info("test vector", "first is %i second is %f", it->first, it->second);
		}
	}

	// solo part
	if(((m_TileIndex == TILE_SOLO_ENABLE) || (m_TileFIndex == TILE_SOLO_ENABLE)) && !Teams().m_Core.GetSolo(ClientId))
	{
		GameServer()->SendChatTarget(ClientId, "You are now in a solo part");
		pChr->SetSolo(true);
	}
	else if(((m_TileIndex == TILE_SOLO_DISABLE) || (m_TileFIndex == TILE_SOLO_DISABLE)) && Teams().m_Core.GetSolo(ClientId))
	{
		GameServer()->SendChatTarget(ClientId, "You are now out of the solo part");
		pChr->SetSolo(false);
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

	if (m_Warmup)
		pGameInfoObj->m_WarmupTimer = m_Warmup - Server()->Tick() + 50;
	else
		pGameInfoObj->m_WarmupTimer = -m_RoundStartTick;

	pGameInfoObj->m_RoundNum = 0;
	pGameInfoObj->m_RoundCurrent = m_RoundCount + 1;

	pGameInfoObj->m_GameStateFlags |= GAMESTATEFLAG_RACETIME;

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
