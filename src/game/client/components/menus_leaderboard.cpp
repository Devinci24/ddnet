
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>

#include <game/localization.h>

#include "menus.h"

#define AMOUNT_RANK_DISPLAYED 5

void CMenus::renderLeaderboardBackground(CUIRect *pRect)
{
    ColorRGBA Color = ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f);
    int Corners = IGraphics::CORNER_ALL;
    float Rounding = 5.0f;

    CUIRect View = *Ui()->Screen();
    vec2 ScreenSize = View.Size();
    Ui()->MapScreen();

    pRect->x = 0;
    pRect->y = ScreenSize.y / 2 - ScreenSize.y / 8;
    pRect->w = ScreenSize.x / 6;
    pRect->h = ScreenSize.y / 3;

    pRect->Draw(Color, Corners, Rounding);
    pRect->HSplitTop(10.0f, nullptr, pRect);

    char aBuf[256];
    str_copy(aBuf, "Top Ranks", sizeof(aBuf));
    Ui()->DoLabel(pRect, aBuf, 10.0f, TEXTALIGN_TC);

    pRect->VMargin(10.0f, pRect);
    pRect->HSplitTop(20.0f, nullptr, pRect);
}

//TODO changes pixels to something deduced from screensize ?
void  CMenus::renderTopRanksOnLeaderboard(CUIRect *Leaderboard)
{
    vec2 leaderboardSize = Leaderboard->Size();
    static CButtonContainer DemoButtons[AMOUNT_RANK_DISPLAYED];
    static CButtonContainer GhostButtons[AMOUNT_RANK_DISPLAYED];

    if(g_Config.m_SvHideScore)
		return;

    for (int i = 0; i < AMOUNT_RANK_DISPLAYED; i++)
    {
        CUIRect Row, Time, Demo, Ghost;
        
        Leaderboard->HSplitTop(20.0f, &Row, Leaderboard);
        Row.VSplitRight(leaderboardSize.x / 8.0f, &Row, &Demo);
        Row.VSplitRight(leaderboardSize.x / 8.0f, &Row, &Ghost);
        Row.VSplitRight(leaderboardSize.x / 4.0f, &Row, &Time);
        Demo.Margin(1.0f, &Demo);
        Ghost.Margin(1.0f, &Ghost);

        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), "%d. %s", i+1+m_FirstRankToDisplay, m_PlayerNames[i], m_PlayerTimes[i]);
        Ui()->DoLabel(&Row, aBuf, 8.0f, TEXTALIGN_ML);
        str_format(aBuf, sizeof(aBuf), "%.2f", m_PlayerTimes[i]);
        Ui()->DoLabel(&Time, aBuf, 8.0f, TEXTALIGN_ML);

        if(DoButton_Menu(&DemoButtons[i], Localize("D"), 0, &Demo))
            dbg_msg("log", "%s", "DEMO!");
        if (DoButton_Menu(&GhostButtons[i], Localize("G"), 0, &Ghost))
            dbg_msg("log", "%s", "GHOST!");
    }
}

void CMenus::renderLeaderboardFooter(CUIRect *Leaderboard, bool &doRequest)
{
    if(g_Config.m_SvHideScore)
		return;

    vec2 leaderboardSize = Leaderboard->Size();

    CUIRect Footer, Prev, Next;
    static CButtonContainer PrevButton, NextButton;
    Leaderboard->HSplitBottom(20.0f, &Footer, Leaderboard);
    Footer.VSplitRight(leaderboardSize.x / 5, nullptr, &Next);
    Footer.VSplitLeft(leaderboardSize.x / 5, &Prev, nullptr);
    if (DoButton_Menu(&PrevButton, Localize("<"), 0, &Prev) && m_FirstRankToDisplay - AMOUNT_RANK_DISPLAYED >= 0)
    {
        m_FirstRankToDisplay = m_FirstRankToDisplay - AMOUNT_RANK_DISPLAYED;
        doRequest = true;
    }
    if (DoButton_Menu(&NextButton, Localize(">"), 0, &Next))
    {
        m_FirstRankToDisplay += AMOUNT_RANK_DISPLAYED; //TODO maybe check is this is bigger than the total amount of ranks?
        doRequest = true;
    }
}

void CMenus::renderLeaderboard()
{
    CUIRect Leaderboard;

    static bool doRequest = true;
    if (doRequest) // if we have all the info we require
    {
        getLeaderboardInfo();
        doRequest = false;
    }

    // if (m_PlayerNames.at(0))
    // {
    //     dbg_msg("log", "player names are : %s", m_PlayerNames.at(0));
    //     dbg_msg("log", "player times are : %f", m_PlayerTimes.at(0)); 
    // }
    
    renderLeaderboardBackground(&Leaderboard);
	renderTopRanksOnLeaderboard(&Leaderboard);
    renderLeaderboardFooter(&Leaderboard, doRequest);

    if (doRequest)
        dbg_msg("log", "request TRUE. First rank is : %d ", m_FirstRankToDisplay);
}

void CMenus::getLeaderboardInfo() //MY TODO maybe call it request?
{
	CNetMsg_Cl_LeaderboardInfo Msg;
	Msg.m_FirstRankToDisplay = m_FirstRankToDisplay;
    Msg.m_AmountOfRanksToDisplay = AMOUNT_RANK_DISPLAYED;
	Client()->SendPackMsgActive(&Msg, MSGFLAG_VITAL);
}

void CMenus::ConKeyLeaderboard(IConsole::IResult *pResult, void *pUserData)
{
    CMenus *pSelf = (CMenus *)pUserData;
    if (pResult->GetInteger(0) != 0 && !pSelf->IsLeaderboardActive())
        pSelf->m_LeaderboardActive = true;
    else if (pResult->GetInteger(0) != 0 && pSelf->IsLeaderboardActive())
        pSelf->m_LeaderboardActive = false;
}
