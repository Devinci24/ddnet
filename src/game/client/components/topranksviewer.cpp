#include <game/localization.h>
#include <game/client/gameclient.h>
#include <engine/shared/config.h>
#include <game/client/components/menus.h>

#include "topranksviewer.h"

#define AMOUNT_RANK_DISPLAYED 5

CDemoViewer::CDemoViewer()
{
    OnReset();
}

void CDemoViewer::OnReset()
{
    m_Active = false;
    oldMaxDistance = -1;
}

void CDemoViewer::renderLeaderboardBackground(CUIRect *pRect)
{
    ColorRGBA Color = ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f);
    int Corners = IGraphics::CORNER_ALL;
    float Rounding = 5.0f;

    CUIRect View = *Ui()->Screen();
    vec2 ScreenSize = View.Size();
    Ui()->MapScreen();

    pRect->x = 0;
    pRect->y = ScreenSize.y / 2 - ScreenSize.y / 8;
    pRect->w = ScreenSize.x / 8;
    pRect->h = ScreenSize.y / 4;

    pRect->Draw(Color, Corners, Rounding);
    pRect->HSplitTop(10.0f, nullptr, pRect);

    char aBuf[256];
    str_copy(aBuf, "Top Ranks", sizeof(aBuf));
    Ui()->DoLabel(pRect, aBuf, 10.0f, TEXTALIGN_TC);

    pRect->VMargin(10.0f, pRect);
    pRect->HSplitTop(20.0f, nullptr, pRect);
}

//TODO changes pixels to something deduced from screensize
void  CDemoViewer::renderTopRanksOnLeaderboard(CUIRect *pRect)
{
    vec2 leaderboardSize = pRect->Size();
    static CButtonContainer DemoButtons[AMOUNT_RANK_DISPLAYED];
    static CButtonContainer GhostButtons[AMOUNT_RANK_DISPLAYED];

    for (int i = 0; i < AMOUNT_RANK_DISPLAYED; i++)
    {
        CUIRect Row, Demo, Ghost;
        
        pRect->HSplitTop(20.0f, &Row, pRect);
        Row.VSplitRight(leaderboardSize.x / 8.0f, &Row, &Demo);
        Row.VSplitRight(leaderboardSize.x / 8.0f, &Row, &Ghost);
        Row.VSplitRight(leaderboardSize.x / 20.0f, &Row, nullptr);
        Demo.Margin(1.0f, &Demo);
        Ghost.Margin(1.0f, &Ghost);

        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), "%d. Player %d", i+1, i);
        Ui()->DoLabel(&Row, aBuf, 10.0f, TEXTALIGN_ML);

        str_format(aBuf, sizeof(aBuf), "%d.76", i);
        Ui()->DoLabel(&Row, aBuf, 10.0f, TEXTALIGN_MR);

        if(m_pClient->m_Menus.DoButton_Menu(&DemoButtons[i], Localize("D"), 0, &Demo))
            printf("%s", "DEMO!");
        if (m_pClient->m_Menus.DoButton_Menu(&GhostButtons[i], Localize("G"), 0, &Ghost))
            printf("%s", "GHOST!");
    }
}

void CDemoViewer::OnRender()
{
    //Ui()->StartCheck();
    //dbg_msg("log", "vec2 (%f.02, %f.02)", GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy].x, GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy].y);
    if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return ;
    if (!m_Active)
        return ;

    Ui()->MapScreen();
    Ui()->Update();
    RenderTools()->RenderCursor(Ui()->MousePos(), 24.0f);

    CUIRect LeaderBoardBackground;

    renderLeaderboardBackground(&LeaderBoardBackground);
    renderTopRanksOnLeaderboard(&LeaderBoardBackground);

	//Ui()->FinishCheck();
	//Ui()->ClearHotkeys();
}

void CDemoViewer::ConKeyLeaderboard(IConsole::IResult *pResult, void *pUserData)
{
    CDemoViewer *pSelf = (CDemoViewer *)pUserData;
    if (pResult->GetInteger(0) != 0 && !pSelf->m_Active)
        pSelf->m_Active = true;
    else if (pResult->GetInteger(0) != 0 && pSelf->m_Active)
        pSelf->m_Active = false;
}

void CDemoViewer::OnConsoleInit()
{
    Console()->Register("+leaderboard", "", CFGFLAG_CLIENT, ConKeyLeaderboard, this, "Open Leaderboard");
}

bool CDemoViewer::OnCursorMove(float x, float y, IInput::ECursorType CursorType)
{
    if (!m_Active) return false;

	Ui()->ConvertMouseMove(&x, &y, CursorType);
	Ui()->OnCursorMove(x, y);

    return true;
}

bool CDemoViewer::OnInput(const IInput::CEvent &Event)
{
    if(Event.m_Flags & IInput::FLAG_PRESS && Event.m_Key == KEY_ESCAPE && IsActive())
	{
		m_Active = false;
		return true;
	}
	return false;
}