#include "stdafx.h"
#ifdef _DEDICATED_SERVER

#include "Common/UI/UI.h"
#include "UIScene_ServerSettings.h"
#include "Minecraft.h"
#include "MultiPlayerLocalPlayer.h"
#include "ClientConnection.h"
#include "../Minecraft.World/net.minecraft.network.h"
#include "../Minecraft.World/net.minecraft.network.packet.h"

UIScene_ServerSettings::UIScene_ServerSettings(int iPad, void *initData, UILayer *parentLayer)
	: UIScene(iPad, parentLayer)
{
	initialiseMovie();

	m_checkboxFireSpreads.init(app.GetString(IDS_FIRE_SPREADS),
		eControl_FireSpreads, app.GetGameHostOption(eGameHostOption_FireSpreads) != 0);

	m_checkboxTNT.init(app.GetString(IDS_TNT_EXPLODES),
		eControl_TNT, app.GetGameHostOption(eGameHostOption_TNT) != 0);

	m_checkboxMobGriefing.init(app.GetString(IDS_MOB_GRIEFING),
		eControl_MobGriefing, app.GetGameHostOption(eGameHostOption_MobGriefing) != 0);

	m_checkboxKeepInventory.init(app.GetString(IDS_KEEP_INVENTORY),
		eControl_KeepInventory, app.GetGameHostOption(eGameHostOption_KeepInventory) != 0);

	m_checkboxDoMobSpawning.init(app.GetString(IDS_MOB_SPAWNING),
		eControl_DoMobSpawning, app.GetGameHostOption(eGameHostOption_DoMobSpawning) != 0);

	m_checkboxDoMobLoot.init(app.GetString(IDS_MOB_LOOT),
		eControl_DoMobLoot, app.GetGameHostOption(eGameHostOption_DoMobLoot) != 0);

	m_checkboxDoTileDrops.init(app.GetString(IDS_TILE_DROPS),
		eControl_DoTileDrops, app.GetGameHostOption(eGameHostOption_DoTileDrops) != 0);

	m_checkboxNaturalRegeneration.init(app.GetString(IDS_NATURAL_REGEN),
		eControl_NaturalRegeneration, app.GetGameHostOption(eGameHostOption_NaturalRegeneration) != 0);

	m_checkboxDoDaylightCycle.init(app.GetString(IDS_DAYLIGHT_CYCLE),
		eControl_DoDaylightCycle, app.GetGameHostOption(eGameHostOption_DoDaylightCycle) != 0);

	m_checkboxPvP.init(L"PvP",
		eControl_PvP, app.GetGameHostOption(eGameHostOption_PvP) != 0);

	m_checkboxTrustPlayers.init(L"Trust Players",
		eControl_TrustPlayers, app.GetGameHostOption(eGameHostOption_TrustPlayers) != 0);
}

wstring UIScene_ServerSettings::getMoviePath()
{
	return L"InGamePlayerOptions";
}

void UIScene_ServerSettings::updateTooltips()
{
	ui.SetTooltips(m_iPad, IDS_TOOLTIPS_SELECT, IDS_TOOLTIPS_BACK);
}

void UIScene_ServerSettings::handleReload()
{
	UIScene::handleReload();
}

void UIScene_ServerSettings::applySettings()
{
	unsigned int hostOptions = app.GetGameHostOption(eGameHostOption_All);
	app.SetGameHostOption(hostOptions, eGameHostOption_FireSpreads,        m_checkboxFireSpreads.IsChecked());
	app.SetGameHostOption(hostOptions, eGameHostOption_TNT,                m_checkboxTNT.IsChecked());
	app.SetGameHostOption(hostOptions, eGameHostOption_MobGriefing,        m_checkboxMobGriefing.IsChecked());
	app.SetGameHostOption(hostOptions, eGameHostOption_KeepInventory,      m_checkboxKeepInventory.IsChecked());
	app.SetGameHostOption(hostOptions, eGameHostOption_DoMobSpawning,      m_checkboxDoMobSpawning.IsChecked());
	app.SetGameHostOption(hostOptions, eGameHostOption_DoMobLoot,          m_checkboxDoMobLoot.IsChecked());
	app.SetGameHostOption(hostOptions, eGameHostOption_DoTileDrops,        m_checkboxDoTileDrops.IsChecked());
	app.SetGameHostOption(hostOptions, eGameHostOption_NaturalRegeneration,m_checkboxNaturalRegeneration.IsChecked());
	app.SetGameHostOption(hostOptions, eGameHostOption_DoDaylightCycle,    m_checkboxDoDaylightCycle.IsChecked());
	app.SetGameHostOption(hostOptions, eGameHostOption_PvP,                m_checkboxPvP.IsChecked());
	app.SetGameHostOption(hostOptions, eGameHostOption_TrustPlayers,       m_checkboxTrustPlayers.IsChecked());

	if (hostOptions != app.GetGameHostOption(eGameHostOption_All))
	{
		Minecraft *pMinecraft = Minecraft::GetInstance();
		if (pMinecraft != nullptr)
		{
			shared_ptr<MultiplayerLocalPlayer> player = pMinecraft->localplayers[m_iPad];
			if (player && player->connection)
				player->connection->send(std::make_shared<ServerSettingsChangedPacket>(ServerSettingsChangedPacket::HOST_IN_GAME_SETTINGS, hostOptions));
		}
	}
}

void UIScene_ServerSettings::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	ui.AnimateKeyPress(iPad, key, repeat, pressed, released);

	switch (key)
	{
	case ACTION_MENU_CANCEL:
		if (pressed)
		{
			applySettings();
			navigateBack();
			handled = true;
		}
		break;

	case ACTION_MENU_OK:
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
		sendInputToMovie(key, repeat, pressed, released);
		break;
	}
}

void UIScene_ServerSettings::handleCheckboxToggled(F64 controlId, bool selected)
{
}

#endif // _DEDICATED_SERVER
