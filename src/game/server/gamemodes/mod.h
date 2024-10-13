#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H

#include <game/server/gamecontroller.h>
#include <vector>
#include <iterator>
#include <algorithm>

class CGameControllerCup : public IGameController
{
private:

	struct m_sPlayersInfo {
		std::string m_PlayerName;
		float m_CurrentTimeCP = -1;
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

	int m_CupState;

	std::vector<m_sPlayersInfo> m_PlayerLeaderboard;

	//start
	void m_fnSendtWarmupMsg();
	void m_PrepareRound();
	void m_fnPausePlayersTune();
	void StartRound() override; //do I really want to override?

	//end
	void EndRound() override; //do I really want to override?
	void m_RemoveEliminatedPlayers();

	//Rounds
	void m_SetSplits(CPlayer *pThisPlayer, int TimeCheckpoint);
	void m_CupOnPlayerFinish(int ClientId);
	void m_CleanUp();

	//utils
	std::vector<m_sPlayersInfo>::iterator m_GetPlayerByName(const char* PlayerName);
	static bool m_SplitsComparator(const m_sPlayersInfo& player1, m_sPlayersInfo& player2);

	//overrides
	//bool CanJoinTeam(int Team, int NotThisId, char *pErrorReason, int ErrorReasonSize) override;

public:
	CGameControllerCup(class CGameContext *pGameServer);
	~CGameControllerCup();

	CScore *Score();

	void Tick() override;
	void HandleCharacterTiles(class CCharacter *pChr, int MapIndex) override;
	void Snap(int SnappingClient) override;
	void OnPlayerConnect(CPlayer *pPlayer) override;

	void StartCup(int WarmupTime);
	int GetState() const override;
};
#endif // GAME_SERVER_GAMEMODES_MOD_H
