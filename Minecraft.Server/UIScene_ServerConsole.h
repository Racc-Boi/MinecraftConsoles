#pragma once
#ifdef _DEDICATED_SERVER

#include "Common/UI/UIScene.h"
#include "Common/UI/UIControl_Label.h"

#define SERVER_CONSOLE_LINES 10

class UIScene_ServerConsole : public UIScene
{
private:
	UIControl_Label m_labels[SERVER_CONSOLE_LINES];

	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT(m_labels[0], "consoleLine1")
		UI_MAP_ELEMENT(m_labels[1], "consoleLine2")
		UI_MAP_ELEMENT(m_labels[2], "consoleLine3")
		UI_MAP_ELEMENT(m_labels[3], "consoleLine4")
		UI_MAP_ELEMENT(m_labels[4], "consoleLine5")
		UI_MAP_ELEMENT(m_labels[5], "consoleLine6")
		UI_MAP_ELEMENT(m_labels[6], "consoleLine7")
		UI_MAP_ELEMENT(m_labels[7], "consoleLine8")
		UI_MAP_ELEMENT(m_labels[8], "consoleLine9")
		UI_MAP_ELEMENT(m_labels[9], "consoleLine10")
	UI_END_MAP_ELEMENTS_AND_NAMES()

	deque<wstring> m_logLines;
	wstring m_inputBuffer;
	int m_cursorPos;
	int m_cursorBlink;
	bool m_logDirty;

	static deque<wstring> s_globalLog;
	static CRITICAL_SECTION s_logCS;
	static bool s_csInit;

public:
	UIScene_ServerConsole(int iPad, void *initData, UILayer *parentLayer);

	virtual EUIScene getSceneType() { return eUIScene_ServerConsole; }
	virtual void updateTooltips();
	virtual void tick();

	virtual bool stealsFocus() { return true; }

protected:
	virtual wstring getMoviePath();

public:
	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);

	static void AddLogLine(const wstring &line);
	static void InitLog();

private:
	void refreshLabels();
	void submitCommand();
	void updateInputDisplay();
};

#endif // _DEDICATED_SERVER
