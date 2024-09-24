
#include <game/localization.h>
#include <engine/shared/config.h>

#include "topranksviewer.h"

#define AMOUNT_RANK_DISPLAYED 5


CDemoViewer::CDemoViewer()
{
    OnReset();
}

void CDemoViewer::OnReset()
{
    m_Active = false;
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

void  CDemoViewer::renderTopRanksOnLeaderboard(CUIRect *pRect)
{
    for (int i = 0; i < AMOUNT_RANK_DISPLAYED; i++)
    {
        CUIRect Row;
        
        pRect->HSplitTop(20.0f, &Row, pRect);
        
        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), "%d. Player %d", i+1, i);
        Ui()->DoLabel(&Row, aBuf, 10.0f, TEXTALIGN_ML);
        
        str_format(aBuf, sizeof(aBuf), "%d.76", i);
        Ui()->DoLabel(&Row, aBuf, 10.0f, TEXTALIGN_MR);
    }
}

// DoButton_Demo(&s_DemoButton, Localize("DEMO!"), 0, &LeaderBoardBackground, 0, IGraphics::CORNER_ALL, 5.0f, 0.0f, ColorRGBA(0.0f, 0.0f, 0.0f, 0.25f));
// int CDemoViewer::DoButton_Demo(CButtonContainer *pButtonContainer, const char *pText, int Checked, const CUIRect *pRect, const char *pImageName, int Corners, float Rounding, float FontFactor, ColorRGBA Color)
// {
//     // CUIRect Text = *pRect;


// 	if(Checked)
// 		Color = ColorRGBA(0.6f, 0.6f, 0.6f, 0.5f);
// 	//Color.a *= Ui()->ButtonColorMul(pButtonContainer);

// 	pRect->Draw(Color, Corners, Rounding);

// 	// Text.HMargin(pRect->h >= 20.0f ? 2.0f : 1.0f, &Text);
// 	// Text.HMargin((Text.h * FontFactor) / 2.0f, &Text);
// 	// Ui()->DoLabel(&Text, pText, Text.h * CUi::ms_FontmodHeight, TEXTALIGN_MC);

// 	//return Ui()->DoButtonLogic(pButtonContainer, Checked, pRect);

//     return 0;
// }

// void CDemoViewer::thisIsAButton()
// {
//     //static CButtonContainer s_DemoButton;


// }

void CDemoViewer::OnRender()
{
    if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return ;
    if (!m_Active)
        return ;

    CUIRect LeaderBoardBackground;

    renderLeaderboardBackground(&LeaderBoardBackground);
    renderTopRanksOnLeaderboard(&LeaderBoardBackground);
}

void CDemoViewer::ConKeyLeaderboard(IConsole::IResult *pResult, void *pUserData)
{
    CDemoViewer *pSelf = (CDemoViewer *)pUserData;
    pSelf->m_Active = pResult->GetInteger(0) != 0;
}

void CDemoViewer::OnConsoleInit()
{
    printf("%s", "hell");
    Console()->Register("+leaderboard", "", CFGFLAG_CLIENT, ConKeyLeaderboard, this, "Open Leaderboard");
}
