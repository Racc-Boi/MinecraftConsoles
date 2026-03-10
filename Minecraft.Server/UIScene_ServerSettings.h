#pragma once
#ifdef _DEDICATED_SERVER

#include "Common/UI/UIScene.h"
#include "Common/UI/UIControl_CheckBox.h"

class UIScene_ServerSettings : public UIScene
{
private:
	enum EControls
	{
		eControl_FireSpreads,
		eControl_TNT,
		eControl_MobGriefing,
		eControl_KeepInventory,
		eControl_DoMobSpawning,
		eControl_DoMobLoot,
		eControl_DoTileDrops,
		eControl_NaturalRegeneration,
		eControl_DoDaylightCycle,
		eControl_PvP,
		eControl_TrustPlayers,
	};

	UIControl_CheckBox m_checkboxFireSpreads;
	UIControl_CheckBox m_checkboxTNT;
	UIControl_CheckBox m_checkboxMobGriefing;
	UIControl_CheckBox m_checkboxKeepInventory;
	UIControl_CheckBox m_checkboxDoMobSpawning;
	UIControl_CheckBox m_checkboxDoMobLoot;
	UIControl_CheckBox m_checkboxDoTileDrops;
	UIControl_CheckBox m_checkboxNaturalRegeneration;
	UIControl_CheckBox m_checkboxDoDaylightCycle;
	UIControl_CheckBox m_checkboxPvP;
	UIControl_CheckBox m_checkboxTrustPlayers;
	UI_BEGIN_MAP_ELEMENTS_AND_NAMES(UIScene)
		UI_MAP_ELEMENT(m_checkboxFireSpreads,        "CheckboxFireSpreads")
		UI_MAP_ELEMENT(m_checkboxTNT,                "CheckboxTNT")
		UI_MAP_ELEMENT(m_checkboxMobGriefing,        "CheckboxMobGriefing")
		UI_MAP_ELEMENT(m_checkboxKeepInventory,      "CheckboxKeepInventory")
		UI_MAP_ELEMENT(m_checkboxDoMobSpawning,      "CheckboxMobSpawning")
		UI_MAP_ELEMENT(m_checkboxDoMobLoot,          "CheckboxMobLoot")
		UI_MAP_ELEMENT(m_checkboxDoTileDrops,        "CheckboxTileDrops")
		UI_MAP_ELEMENT(m_checkboxNaturalRegeneration,"CheckboxNaturalRegeneration")
		UI_MAP_ELEMENT(m_checkboxDoDaylightCycle,    "CheckboxDayLightCycle")
		UI_MAP_ELEMENT(m_checkboxPvP,                "CheckboxBuildAndMine")
		UI_MAP_ELEMENT(m_checkboxTrustPlayers,       "CheckboxUseDoorsAndSwitches")
	UI_END_MAP_ELEMENTS_AND_NAMES()

public:
	UIScene_ServerSettings(int iPad, void *initData, UILayer *parentLayer);

	virtual EUIScene getSceneType() { return eUIScene_ServerSettings; }
	virtual void updateTooltips();
	virtual void handleReload();

protected:
	virtual wstring getMoviePath();

public:
	virtual void handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled);
	virtual void handleCheckboxToggled(F64 controlId, bool selected);

private:
	void applySettings();
};

#endif // _DEDICATED_SERVER
