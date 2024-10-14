#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H

#include <game/server/gamecontroller.h>
#include <vector>

struct Sm_PlayersInfo {
	std::string m_PlayerName;
	float m_CurrentTimeCP[MAX_CHECKPOINTS];
	int m_AmountOfTimeCPs = 0;
	bool m_HasFinished = false;
};

class CGameControllerCup : public IGameController
{
private:

	enum
	{
		STATE_NONE,
		STATE_WARMUP,
		STATE_WARMUP_ROUND,
		STATE_ROUND,
	};

	int m_CupState;

	//start
	void SendtWarmupMsg();
	void PrepareRound();
	void PausePlayersTune();
	void StartRound() override; //do I really want to override?

	//end
	void EndRound() override; //do I really want to override?
	void RemoveEliminatedPlayers();

	//Rounds
	//void SetSplits(CPlayer *pThisPlayer, int TimeCheckpoint);
	void CupOnPlayerFinish(int ClientId);
	void CleanUp();

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

	//used outside
	void StartCup(int WarmupTime);
	int GetState() const;

	std::vector<Sm_PlayersInfo>::iterator GetPlayerByName(const char* PlayerName);
	static bool SplitsComparator(const Sm_PlayersInfo& player1, Sm_PlayersInfo& player2);
	void SetSplits(CPlayer *pThisPlayer, int TimeCheckpoint);

	std::vector<Sm_PlayersInfo> m_PlayerLeaderboard;

};
#endif // GAME_SERVER_GAMEMODES_MOD_H
