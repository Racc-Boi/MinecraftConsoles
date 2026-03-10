#pragma once
#ifdef _DEDICATED_SERVER

#include "Common/UI/UIScene.h"
#include "Common/UI/UIControl_Label.h"
#include "Common/UI/UIControl_Button.h"
#include "Common/UI/UIControl_PlayerList.h"

class UIScene_ServerDashboard : public UIScene
{
private:
	enum EControls
	{
		eControl_GameOptions,
		eControl_GamePlayers,
	};

	typedef struct _PlayerInfo
	{
		byte m_smallId;
		char m_voiceStatus;
		short m_colorState;
		wstring m_name;
	} PlayerInfo;

	vector<PlayerInfo *> m_players;

	UIControl_Button m_buttonGameOptions;
	UIControl_PlayerList m_playerList;
	UIControl_Label m_labelTitle;
	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT( m_buttonGameOptions, "GameOptions")
		UI_MAP_ELEMENT( m_playerList, "GamePlayers")
		UI_MAP_ELEMENT( m_labelTitle, "Title")
	UI_END_MAP_ELEMENTS_AND_NAMES()

public:
	UIScene_ServerDashboard(int iPad, void *initData, UILayer *parentLayer);
	virtual ~UIScene_ServerDashboard();

	virtual EUIScene getSceneType() { return eUIScene_ServerDashboard; }
	virtual void updateTooltips();

	virtual void handleReload();
	virtual void tick();

protected:
	virtual wstring getMoviePath();

public:
	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);

protected:
	virtual void handleGainFocus(bool navBack);
	void handlePress(F64 controlId, F64 childId);
	virtual void handleDestroy();
	virtual void handleFocusChange(F64 controlId, F64 childId);

public:
	static void OnPlayerChanged(void *callbackParam, INetworkPlayer *pPlayer, bool leaving);

private:
	PlayerInfo *BuildPlayerInfo(INetworkPlayer *player);
};

#endif // _DEDICATED_SERVER
