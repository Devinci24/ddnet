#ifndef GAME_SERVER_GAMEMODES_MOD_H
#define GAME_SERVER_GAMEMODES_MOD_H

#include <game/server/gamecontroller.h>
#include <vector>
#include <utility>
#include <algorithm>

class CGameControllerCup : public IGameController
{
private:

	bool m_TunesOn;
	bool m_StopAll;
	bool m_RoundStarted;
	bool m_SomeoneHasFinished;
	bool m_IsFirstRound;
	int m_FirstFinisherTick;
	int m_NumberOfPlayerLeft;
	int64_t m_lastScoreBroadcast;
	std::set<int> m_finishedPlayers;

	void m_fnSendTimeLeftWarmupMsg();
	void m_fnPauseServer();
	void m_fnStartRoundTimer(int Seconds);

	void m_fnRemoveEliminatedPlayers();
	bool m_fnDoesElementExist(const int ClientId);
	int m_fnGetIndexOfElement(const int ClientId);
	int m_fnGetId(int ClientId);

	std::set<int> GetPlayersIdOnTeam(int teamId);
public:

	std::vector<std::pair<int, float>> m_RoundScores;

	CGameControllerCup(class CGameContext *pGameServer);
	~CGameControllerCup();

	CScore *Score();

	void Tick() override;

	void m_fnRestartCup();
	void OnPlayerConnect(class CPlayer *pPlayer) override;
	void StartRound() override;
	void EndRound() override;
	void HandleCharacterTiles(class CCharacter *pChr, int MapIndex) override;
	void Snap(int SnappingClient) override;
	void DoWarmup(int Seconds) override;

	bool GetRoundStarted() const override;
};
#endif // GAME_SERVER_GAMEMODES_MOD_H
