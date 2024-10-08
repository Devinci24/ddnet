
#include <game/client/ui.h>
#include <game/client/ui_listbox.h>

#include <game/localization.h>

#include "base/system.h"
#include "engine/graphics.h"
#include "engine/shared/protocol.h"
#include "game/client/ui_rect.h"
#include "menus.h"

//MY TODO :

// 1. Somehow use LEADERBOARD_DISPLAY_RANKS in network.py
// 2. possibly limit the amount of ranks cachedLeaderboard can query?
// 3. Make sure cachedleaderboard doesn't bug somehow when new player finishes and size is not a multiple od LEADERBOARD_CACHED_RANK anymore (LOOKS FINE)
// 4. -
// 5. Make sure the chat hack doesn't break anything

void CMenus::RenderLeaderboardHeader(CUIRect *Header)
{
    char aBuf[256];
    str_copy(aBuf, "Top Ranks", sizeof(aBuf));
    Ui()->DoLabel(Header, aBuf, Header->Size().x / 10, TEXTALIGN_TC);
}

//TODO changes pixels to something deduced from screensize ?
void  CMenus::RenderTopRanksOnLeaderboard(CUIRect *Content)
{
    vec2 ContentSize = Content->Size();
    // static CButtonContainer s_DemoButtons[LEADERBOARD_DISPLAY_RANKS];
    // static CButtonContainer s_GhostButtons[LEADERBOARD_DISPLAY_RANKS];

    if(g_Config.m_SvHideScore)
		return;

    for (int i = 0; i < LEADERBOARD_DISPLAY_RANKS; i++)
    {
        CUIRect Row, Time;//, Demo, Ghost;
        
        Content->HSplitTop(ContentSize.y / (LEADERBOARD_DISPLAY_RANKS), &Row, Content);
        // Row.VSplitRight(ContentSize.x / 8.0f, &Row, &Demo);
        // Row.VSplitRight(ContentSize.x / 8.0f, &Row, &Ghost);
        Row.VSplitRight(ContentSize.x / 4.0f, &Row, &Time);
        // Demo.Margin(1.0f, &Demo);
        // Ghost.Margin(1.0f, &Ghost);
        // Demo.HMargin(2.0f, &Demo);
        // Ghost.HMargin(2.0f, &Ghost);

        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), "%d. %s", i+1+m_FirstRankToDisplay, m_aPlayerLeaderboard[i].m_PlayerName, m_aPlayerLeaderboard[i].m_PlayerTime);
        Ui()->DoLabel(&Row, aBuf, 10.0f, TEXTALIGN_ML);
        str_format(aBuf, sizeof(aBuf), "%.2f", m_aPlayerLeaderboard[i].m_PlayerTime);
        Ui()->DoLabel(&Time, aBuf, 10.0f, TEXTALIGN_ML);

        // if(DoButton_Menu(&s_DemoButtons[i], Localize("D"), 0, &Demo, nullptr, IGraphics::CORNER_ALL , 5.0f, 0.4f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
        //     dbg_msg("log", "%s", "DEMO!");
        // if (DoButton_Menu(&s_GhostButtons[i], Localize("G"), 0, &Ghost, nullptr, IGraphics::CORNER_ALL , 5.0f, 0.4f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
        //     dbg_msg("log", "%s", "GHOST!");
    }
}

//CMenus::DoButton_Menu(CButtonContainer *pButtonContainer, const char *pText, int Checked, const CUIRect *pRect, const char *pImageName, int Corners, float Rounding, float FontFactor, ColorRGBA Color)
void CMenus::RenderLeaderboardFooter(CUIRect *Footer)
{
    if(g_Config.m_SvHideScore)
		return;

    vec2 FooterSize = Footer->Size();

    CUIRect Prev, Next;
    static CButtonContainer s_PrevButton, s_NextButton;

    Footer->VSplitRight(FooterSize.x / 3, nullptr, &Next);
    Footer->VSplitLeft(FooterSize.x / 3, &Prev, nullptr);
    if (DoButton_Menu(&s_PrevButton, Localize("<"), 0, &Prev, nullptr, IGraphics::CORNER_ALL , 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
        getLeaderboardInfo(m_FirstRankToDisplay - LEADERBOARD_DISPLAY_RANKS);

    if (DoButton_Menu(&s_NextButton, Localize(">"), 0, &Next, nullptr, IGraphics::CORNER_ALL , 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f)))
        getLeaderboardInfo(m_FirstRankToDisplay + LEADERBOARD_DISPLAY_RANKS); //TODO maybe check is this is bigger than the total amount of ranks?
}

void CMenus::PrepareLeaderboard(CUIRect *Content, CUIRect *Header, CUIRect *Footer)
{
    ColorRGBA Color = ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f);
    int Corners = IGraphics::CORNER_ALL;
    float Rounding = 5.0f;

    CUIRect View = *Ui()->Screen();
    vec2 ScreenSize = View.Size();
    Ui()->MapScreen();

    Content->x = 0;
    Content->y = ScreenSize.y / 2 - ScreenSize.y / 8;
    Content->w = ScreenSize.x / 6.5f;
    Content->h = ScreenSize.y / 3;

    Content->Draw(Color, Corners, Rounding);
    Content->Margin(8.0f, Content);
    Content->HMargin(5.0f, Content);

    //create Header
    Content->HSplitTop(20.0f, Header, Content);

    //create Footer
    if (!m_LeaderboardUiOn)
    {
        Content->HSplitBottom(20.0f, Content, Footer);
        Footer->Margin(2.0f, Footer);
    }

    Content->Margin(2.0f, Content);
    Header->Margin(2.0f, Header);
}

void CMenus::renderLeaderboard()
{
    CUIRect Content, Header, Footer;
    
    PrepareLeaderboard(&Content, &Header, &Footer);
    RenderLeaderboardHeader(&Header);
	RenderTopRanksOnLeaderboard(&Content);
    if (!m_LeaderboardUiOn)
        RenderLeaderboardFooter(&Footer);
}

void CMenus::getLeaderboardInfo(int FirstRankToDisplay) //MY TODO maybe call it request?
{
    if (FirstRankToDisplay < 0)
        return ;
    
    m_FirstRankToDisplay = FirstRankToDisplay;

    CNetMsg_Cl_LeaderboardInfo Msg;
	Msg.m_FirstRankToDisplay = FirstRankToDisplay;
	Client()->SendPackMsgActive(&Msg, MSGFLAG_VITAL);
}

void CMenus::ConKeyLeaderboard(IConsole::IResult *pResult, void *pUserData)
{
    CMenus *pSelf = (CMenus *)pUserData;
    
    pSelf->m_LeaderboardActive = !pSelf->m_LeaderboardActive;
}

void CMenus::ConLeaderboardUiOnly(IConsole::IResult *pResult, void *pUserData)
{
    CMenus *pSelf = (CMenus *)pUserData;

    bool State = pResult->NumArguments() ? pResult->GetInteger(0) : !pSelf->m_LeaderboardUiOn;
    pSelf->m_LeaderboardUiOn = State;
}

void CMenus::ConLeaderboardNextRanks(IConsole::IResult *pResult, void *pUserData)
{
    CMenus *pSelf = (CMenus *)pUserData;

    pSelf->getLeaderboardInfo(pSelf->m_FirstRankToDisplay + LEADERBOARD_DISPLAY_RANKS);
}

void CMenus::ConLeaderboardPreviousRanks(IConsole::IResult *pResult, void *pUserData)
{
    CMenus *pSelf = (CMenus *)pUserData;

    pSelf->getLeaderboardInfo(pSelf->m_FirstRankToDisplay - LEADERBOARD_DISPLAY_RANKS);
}
