#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H

#include "engine/shared/protocol.h"
#include <game/server/gamecontroller.h>
#include <vector>

class CGameControllerCup : public IGameController
{
private:
	int m_CupState;

	//start
	void SendWarmupMsg();
	void PrepareRound();
	void PausePlayersTune();
	void StartRound() override; //do I really want to override?

	//end
	void EndRound() override; //do I really want to override?
	void RemoveEliminatedPlayers();

	//Rounds
	void CupOnPlayerFinish(int ClientId);
	void CleanUp();

	//overrides
	//bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;

public:
	struct SPlayersInfo
	{
		char m_aPlayerName[MAX_NAME_LENGTH] = "\0";
		float m_aCurrentTimeCps[MAX_CHECKPOINTS] = {0.0f};
		int m_AmountOfTimeCPs = 0;
		bool m_HasFinished = false;
	};
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
	bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;

	//used outside
	void StartCup(int WarmupTime);
	int GetState() const;

	std::vector<SPlayersInfo>::iterator GetPlayerByName(const char *PlayerName);
	static bool SplitsComparator(const SPlayersInfo &Player1, SPlayersInfo &Player2);
	void SetSplits(CPlayer *pThisPlayer, int TimeCheckpoint);

	std::vector<SPlayersInfo> m_vPlayerLeaderboard;
};
#endif // GAME_SERVER_GAMEMODES_MOD_H
