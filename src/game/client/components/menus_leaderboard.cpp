
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>

#include <game/localization.h>
#include <limits>

#include "base/system.h"
#include "engine/graphics.h"
#include "engine/shared/protocol.h"
#include "menus.h"

//MY TODO :

// 1. Somehow use LEADERBOARD_DISPLAY_RANKS in network.py
// 2. possibly limit the amount of ranks cachedLeaderboard can query?
// 3. Make sure cachedleaderboard doesn't bug somehow when new player finishes and size is not a multiple od LEADERBOARD_CACHED_RANK anymore (LOOKS FINE)
// 4. Fix leaderboard not reloading on map change.
// 5. Somehow make it that you can write in chat while leaderboard (but chat doesn't work after clicking button) open and best case scenario also aim

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
    pRect->Margin(10.0f, pRect);

    char aBuf[256];
    str_copy(aBuf, "Top Ranks", sizeof(aBuf));
    Ui()->DoLabel(pRect, aBuf, pRect->Size().x / 10, TEXTALIGN_TC);

    pRect->HSplitTop(20.0f, nullptr, pRect);
}

//TODO changes pixels to something deduced from screensize ?
void  CMenus::renderTopRanksOnLeaderboard(CUIRect *Leaderboard)
{
    vec2 LeaderboardSize = Leaderboard->Size();
    static CButtonContainer s_DemoButtons[LEADERBOARD_DISPLAY_RANKS];
    static CButtonContainer s_GhostButtons[LEADERBOARD_DISPLAY_RANKS];

    if(g_Config.m_SvHideScore)
		return;

    for (int i = 0; i < LEADERBOARD_DISPLAY_RANKS; i++)
    {
        CUIRect Row, Time, Demo, Ghost;
        
        Leaderboard->HSplitTop(LeaderboardSize.y / (LEADERBOARD_DISPLAY_RANKS + 1), &Row, Leaderboard);
        Row.VSplitRight(LeaderboardSize.x / 8.0f, &Row, &Demo);
        Row.VSplitRight(LeaderboardSize.x / 8.0f, &Row, &Ghost);
        Row.VSplitRight(LeaderboardSize.x / 4.0f, &Row, &Time);
        Demo.Margin(1.0f, &Demo);
        Ghost.Margin(1.0f, &Ghost);
        Demo.HMargin(2.0f, &Demo);
        Ghost.HMargin(2.0f, &Ghost);

        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), "%d. %s", i+1+m_FirstRankToDisplay, m_aPlayerLeaderboard[i].m_PlayerName, m_aPlayerLeaderboard[i].m_PlayerTime);
        Ui()->DoLabel(&Row, aBuf, 10.0f, TEXTALIGN_ML);
        str_format(aBuf, sizeof(aBuf), "%.2f", m_aPlayerLeaderboard[i].m_PlayerTime);
        Ui()->DoLabel(&Time, aBuf, 10.0f, TEXTALIGN_ML);

        if(DoButton_Menu(&s_DemoButtons[i], Localize("D"), 0, &Demo, nullptr, IGraphics::CORNER_ALL , 5.0f, 0.4f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
            dbg_msg("log", "%s", "DEMO!");
        if (DoButton_Menu(&s_GhostButtons[i], Localize("G"), 0, &Ghost, nullptr, IGraphics::CORNER_ALL , 5.0f, 0.4f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
            dbg_msg("log", "%s", "GHOST!");
    }
    Leaderboard->HSplitTop(10.0f, nullptr, Leaderboard);
}

//CMenus::DoButton_Menu(CButtonContainer *pButtonContainer, const char *pText, int Checked, const CUIRect *pRect, const char *pImageName, int Corners, float Rounding, float FontFactor, ColorRGBA Color)
void CMenus::renderLeaderboardFooter(CUIRect *Leaderboard, bool &DoRequest)
{
    if(g_Config.m_SvHideScore)
		return;

    vec2 LeaderboardSize = Leaderboard->Size();

    CUIRect Prev, Next;
    static CButtonContainer s_PrevButton, s_NextButton;

    Leaderboard->VSplitRight(LeaderboardSize.x / 3, nullptr, &Next);
    Leaderboard->VSplitLeft(LeaderboardSize.x / 3, &Prev, nullptr);
    if (DoButton_Menu(&s_PrevButton, Localize("<"), 0, &Prev, nullptr, IGraphics::CORNER_ALL , 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)) && m_FirstRankToDisplay - LEADERBOARD_DISPLAY_RANKS >= 0)
    {
        m_FirstRankToDisplay = m_FirstRankToDisplay - LEADERBOARD_DISPLAY_RANKS;
        DoRequest = true;
    }
    if (DoButton_Menu(&s_NextButton, Localize(">"), 0, &Next, nullptr, IGraphics::CORNER_ALL , 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
    {
        m_FirstRankToDisplay += LEADERBOARD_DISPLAY_RANKS; //TODO maybe check is this is bigger than the total amount of ranks?
        DoRequest = true;
    }
}

void CMenus::renderLeaderboard()
{
    CUIRect Leaderboard;

    static bool s_doRequest = true;
    if (s_doRequest) // if we have all the info we require
    {
        getLeaderboardInfo();
        s_doRequest = false;
    }

    // if (m_PlayerNames.at(0))
    // {
    //     dbg_msg("log", "player names are : %s", m_PlayerNames.at(0));
    //     dbg_msg("log", "player times are : %f", m_PlayerTimes.at(0)); 
    // }
    
    renderLeaderboardBackground(&Leaderboard);
	renderTopRanksOnLeaderboard(&Leaderboard);
    renderLeaderboardFooter(&Leaderboard, s_doRequest);
}

void CMenus::getLeaderboardInfo() //MY TODO maybe call it request?
{
	CNetMsg_Cl_LeaderboardInfo Msg;
	Msg.m_FirstRankToDisplay = m_FirstRankToDisplay;
	Client()->SendPackMsgActive(&Msg, MSGFLAG_VITAL);
}

void CMenus::ConKeyLeaderboard(IConsole::IResult *pResult, void *pUserData)
{
    CMenus *pSelf = (CMenus *)pUserData;
    if (pResult->GetInteger(0)) //why do I need this
        pSelf->m_LeaderboardActive = !pSelf->m_LeaderboardActive;
}

void CMenus::ConLeaderboardAim(IConsole::IResult *pResult, void *pUserData)
{
    CMenus *pSelf = (CMenus *)pUserData;

    bool State = pResult->NumArguments() ? pResult->GetInteger(0) : !pSelf->m_LeaderboardAimState;
    pSelf->m_LeaderboardAimState = State;
}