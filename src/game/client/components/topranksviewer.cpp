
#include <game/localization.h>
#include <game/client/gameclient.h>
#include <engine/shared/config.h>

#include "topranksviewer.h"

#define AMOUNT_RANK_DISPLAYED 1


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
    CUIRect Row, Demo, Ghost;

    for (int i = 0; i < AMOUNT_RANK_DISPLAYED; i++)
    {
        static CButtonContainer DemoButton;
        //CButtonContainer GhostButton;
        
        pRect->HSplitTop(20.0f, &Row, pRect);
        Row.VSplitRight(leaderboardSize.x / 8.0f, &Row, &Demo);
        Row.VSplitRight(leaderboardSize.x / 8.0f, &Row, &Ghost);
        Row.VSplitRight(leaderboardSize.x / 20.0f, &Row, nullptr);
        
        char aBuf[256];
        str_format(aBuf, sizeof(aBuf), "%d. Player %d", i+1, i);
        Ui()->DoLabel(&Row, aBuf, 10.0f, TEXTALIGN_ML);

        str_format(aBuf, sizeof(aBuf), "%d.76", i);
        Ui()->DoLabel(&Row, aBuf, 10.0f, TEXTALIGN_MR);

        if(m_Cmenu.DoButton_Menu(&DemoButton, Localize("D"), 0, &Demo))
            printf("%s", "DEMO!");
        //if (m_Cmenu.DoButton_Menu(&GhostButton, Localize("G"), 0, &Ghost))
        //    printf("%s", "GHOST!");

    }
}

// void CDemoViewer::DoButtonLogic(CButtonContainer *pButtonContainer, CUIRect *pRect)
// {
//     dbg_msg("log", "vec2 (%f.02, %f.02)", pRect->x, pRect->y);
// }

// int CDemoViewer::DoButton(CButtonContainer *pButtonContainer, const char *pText, CUIRect *pRect, const char *pImageName, int Corners, float Rounding, ColorRGBA Color)
// {
//     char aBuf[256];

// 	Color.a *= Ui()->ButtonColorMul(pButtonContainer);

//     //pRect->Margin(vec2(2.0f, 2.0f), pRect);
// 	pRect->Draw(Color, Corners, Rounding);

//     if (strcmp(pText, "DEMO!"))
//         str_copy(aBuf, "D", sizeof(aBuf));
//     else if (strcmp(pText, "GHOST!"))
//         str_copy(aBuf, "G", sizeof(aBuf));

//     Ui()->DoLabel(pRect, aBuf, 10.0f, TEXTALIGN_MC);

//     // TODO REPLACE TEXT BY IMAGES
// 	// if(pImageName)
// 	// {
// 	// 	CUIRect Image;
// 	// 	pRect->VSplitRight(pRect->h * 4.0f, &Text, &Image); // always correct ratio for image

// 	// 	// render image
// 	// 	const CMenuImage *pImage = FindMenuImage(pImageName);
// 	// 	if(pImage)
// 	// 	{
// 	// 		Graphics()->TextureSet(Ui()->HotItem() == pButtonContainer ? pImage->m_OrgTexture : pImage->m_GreyTexture);
// 	// 		Graphics()->WrapClamp();
// 	// 		Graphics()->QuadsBegin();
// 	// 		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
// 	// 		IGraphics::CQuadItem QuadItem(Image.x, Image.y, Image.w, Image.h);
// 	// 		Graphics()->QuadsDrawTL(&QuadItem, 1);
// 	// 		Graphics()->QuadsEnd();
// 	// 		Graphics()->WrapNormal();
// 	// 	}
// 	// }

//     DoButtonLogic(pButtonContainer, pRect);
// 	return 0;
// }

void CDemoViewer::OnRender()
{
    //dbg_msg("log", "vec2 (%f.02, %f.02)", GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy].x, GameClient()->m_Controls.m_aMousePos[g_Config.m_ClDummy].y);
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
    if (pResult->GetInteger(0) != 0 && !pSelf->m_Active)
        pSelf->m_Active = true;
    else if (pResult->GetInteger(0) != 0 && pSelf->m_Active)
        pSelf->m_Active = false;

    //TODO maybe should take dyncam into account && RESTORE default clMouseMaxDistance on quitting with leaderboard on
    if (pSelf->m_Active && pSelf->oldMaxDistance == -1)
    {
        pSelf->oldMaxDistance = g_Config.m_ClMouseMaxDistance;
        g_Config.m_ClMouseMaxDistance = 5000;
    }
    else if (!pSelf->m_Active && pSelf->oldMaxDistance != -1)
    {
        g_Config.m_ClMouseMaxDistance = pSelf->oldMaxDistance;
        pSelf->oldMaxDistance = -1;
    }
}

void CDemoViewer::OnConsoleInit()
{
    Console()->Register("+leaderboard", "", CFGFLAG_CLIENT, ConKeyLeaderboard, this, "Open Leaderboard");
}
