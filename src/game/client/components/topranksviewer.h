
#include <game/client/component.h>
//#include <engine/console.h>

// button
#include <game/client/ui.h>


#ifndef GAME_CLIENT_COMPONENTS_TOPRANKSVIEWER_H
#define GAME_CLIENT_COMPONENTS_TOPRANKSVIEWER_H

class CDemoViewer : public CComponent
{
	bool m_Active;

	int DoButton_Demo(CButtonContainer *pButtonContainer, const char *pText, int Checked, const CUIRect *pRect, const char *pImageName, int Corners, float Rounding, float FontFactor, ColorRGBA Color);
	void thisIsAButton(void);

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
		// virtual bool OnCursorMove(float x, float y, IInput::ECursorType CursorType) override;
		// virtual bool OnInput(const IInput::CEvent &Event) override;

		bool IsActive() const { return m_Active; }
};



#endif