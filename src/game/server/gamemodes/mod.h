#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H

#include "engine/shared/protocol.h"
#include <game/server/gamecontroller.h>
#include <vector>

//MY TODOS:
//let people vote map change. And vote change modes
//make it so that at 6PM? 7? every day new map.

class CGameControllerCup : public IGameController
{
private:
	struct SCupInfo
	{
		//timers
		int m_WarmupTimer;
		int m_Coundown;
		int m_EndRoundTimer; //unused

		bool m_IsRealCup = true; //unused - should make it impossible to vote anything on real cup
		bool m_IsLoop = false;	//make the event repeatable
	};
	SCupInfo m_CupInfo;

	//players stuff
	struct SPlayersGameInfo
	{
		bool m_active = false;
		int m_Score = 0;	//unused
	};
	SPlayersGameInfo m_aPlayers[MAX_CLIENTS];

	int m_CupState;

	//start
	void SendWarmupMsg();
	void PrepareRound();
	void PausePlayersTune();
	void StartRound() override; //do I really want to override?

	//end
	void EndRound() override; //do I really want to override?
	void RemoveEliminatedPlayers();
	int AmountOfActivePlayers();

	//Rounds
	void CupOnPlayerFinish(int ClientId);
	void CleanUp();

public:
	struct SPlayersRaceInfo
	{
		char m_aPlayerName[MAX_NAME_LENGTH] = "\0";
		float m_aCurrentTimeCps[MAX_CHECKPOINTS] = {0.0f};
		int m_AmountOfTimeCPs = 0;
		bool m_HasFinished = false;
	};

	//NONE and WARMUP share A LOT of similarities. could remove one with very few changes. Keeping it like this because it makes it feels a bit more clear.
	enum
	{
		STATE_NONE,
		STATE_WARMUP,
		STATE_WARMUP_ROUND,
		STATE_ROUND,
	};
	

	CGameControllerCup(class CGameContext *pGameServer);
	~CGameControllerCup();

	CScore *Score();

	void Tick() override;
	void HandleCharacterTiles(class CCharacter *pChr, int MapIndex) override;
	void Snap(int SnappingClient) override;
	void OnPlayerConnect(CPlayer *pPlayer) override;
	void OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason) override;
	bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;

	//used outside
	void StartCup(int WarmupTime);
	void SetCupMode(int Mode);
	int GetState() const;

	std::vector<SPlayersRaceInfo>::iterator GetPlayerByName(const char *PlayerName);
	static bool SplitsComparator(const SPlayersRaceInfo &Player1, SPlayersRaceInfo &Player2);
	void SetSplits(CPlayer *pThisPlayer, int TimeCheckpoint);

	std::vector<SPlayersRaceInfo> m_vPlayerLeaderboard;
};
#endif // GAME_SERVER_GAMEMODES_MOD_H
