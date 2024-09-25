
#include <game/client/component.h>
#include <game/client/components/menus.h>
//#include <engine/console.h>

// button
#include <game/client/ui.h>


#ifndef GAME_CLIENT_COMPONENTS_TOPRANKSVIEWER_H
#define GAME_CLIENT_COMPONENTS_TOPRANKSVIEWER_H

class CDemoViewer : public CComponent
{
	bool m_Active;
	float oldMaxDistance;

	CMenus m_Cmenu;
	// int DoButton(CButtonContainer *pButtonContainer, const char *pText, CUIRect *pRect, const char *pImageName, int Corners, float Rounding, ColorRGBA Color);
	// void DoButtonLogic(CButtonContainer *pButtonContainer, CUIRect *pRect);

	void renderLeaderboardBackground(CUIRect *pRect);
	void renderTopRanksOnLeaderboard(CUIRect *pRect);

	static void ConKeyLeaderboard(IConsole::IResult *pResult, void *pUserData);

	public:
		CDemoViewer();
		//~CDemoViewer() {};
		virtual int Sizeof() const override { return sizeof(*this); }

		virtual void OnReset() override;
		virtual void OnConsoleInit() override;
		virtual void OnRender() override;
		// virtual void OnRelease() override;
		virtual bool OnCursorMove(float x, float y, IInput::ECursorType CursorType) override;
		// virtual bool OnInput(const IInput::CEvent &Event) override;

		bool IsActive() const { return m_Active; }
};



#endif