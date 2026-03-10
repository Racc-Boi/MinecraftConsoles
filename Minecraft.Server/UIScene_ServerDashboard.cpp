#include "stdafx.h"
#ifdef _DEDICATED_SERVER

#include "Common/UI/UI.h"
#include "UIScene_ServerDashboard.h"
#include "Minecraft.h"
#include "MultiPlayerLocalPlayer.h"
#include "ClientConnection.h"
#include "../Minecraft.World/net.minecraft.network.packet.h"
#include "MinecraftServer.h"

UIScene_ServerDashboard::UIScene_ServerDashboard(int iPad, void *initData, UILayer *parentLayer)
	: UIScene(iPad, parentLayer)
{
	initialiseMovie();

	parentLayer->addComponent(iPad, eUIComponent_Panorama);
	parentLayer->addComponent(iPad, eUIComponent_MenuBackground);

	m_buttonGameOptions.init(L"Host Options / Game Rules", eControl_GameOptions);
	m_labelTitle.init(L"Server Management");
	m_playerList.init(eControl_GamePlayers);

	m_players = vector<PlayerInfo *>();

	DWORD playerCount = g_NetworkManager.GetPlayerCount();
	for (DWORD i = 0; i < playerCount; ++i)
	{
		INetworkPlayer *player = g_NetworkManager.GetPlayerByIndex(i);
		if (player != nullptr)
		{
			PlayerInfo *info = BuildPlayerInfo(player);
			m_players.push_back(info);
			m_playerList.addItem(info->m_name, info->m_colorState, info->m_voiceStatus);
		}
	}

	g_NetworkManager.RegisterPlayerChangedCallback(m_iPad, &UIScene_ServerDashboard::OnPlayerChanged, this);
	updateTooltips();
}

UIScene_ServerDashboard::~UIScene_ServerDashboard()
{
	for (size_t i = 0; i < m_players.size(); ++i)
		delete m_players[i];
}

wstring UIScene_ServerDashboard::getMoviePath()
{
	return L"InGameInfoMenu";
}

void UIScene_ServerDashboard::updateTooltips()
{
	int keyA = -1;

	if (!m_players.empty() && m_playerList.hasFocus() &&
		m_playerList.getCurrentSelection() < static_cast<int>(m_players.size()))
	{
		keyA = IDS_TOOLTIPS_PRIVILEGES;
	}
	else if (m_buttonGameOptions.hasFocus())
	{
		keyA = IDS_TOOLTIPS_SELECT;
	}

	ui.SetTooltips(m_iPad, keyA, -1, -1, -1);
}

void UIScene_ServerDashboard::handleDestroy()
{
	g_NetworkManager.UnRegisterPlayerChangedCallback(m_iPad, &UIScene_ServerDashboard::OnPlayerChanged, this);

	m_parentLayer->removeComponent(eUIComponent_Panorama);
	m_parentLayer->removeComponent(eUIComponent_MenuBackground);
}

void UIScene_ServerDashboard::handleGainFocus(bool navBack)
{
	UIScene::handleGainFocus(navBack);
	if (navBack)
		g_NetworkManager.RegisterPlayerChangedCallback(m_iPad, &UIScene_ServerDashboard::OnPlayerChanged, this);
}

void UIScene_ServerDashboard::handleReload()
{
	DWORD playerCount = g_NetworkManager.GetPlayerCount();

	for (size_t i = 0; i < m_players.size(); ++i)
		delete m_players[i];
	m_players.clear();

	for (DWORD i = 0; i < playerCount; ++i)
	{
		INetworkPlayer *player = g_NetworkManager.GetPlayerByIndex(i);
		if (player != nullptr)
		{
			PlayerInfo *info = BuildPlayerInfo(player);
			m_players.push_back(info);
			m_playerList.addItem(info->m_name, info->m_colorState, info->m_voiceStatus);
		}
	}

	updateTooltips();

	if (controlHasFocus(eControl_GamePlayers))
		m_playerList.setCurrentSelection(getControlChildFocus());
}

void UIScene_ServerDashboard::tick()
{
	UIScene::tick();

	for (DWORD i = 0; i < m_players.size(); ++i)
	{
		INetworkPlayer *player = g_NetworkManager.GetPlayerByIndex(i);
		if (player != nullptr)
		{
			PlayerInfo *info = BuildPlayerInfo(player);
			m_players[i]->m_smallId = info->m_smallId;

			if (info->m_voiceStatus != m_players[i]->m_voiceStatus)
			{
				m_players[i]->m_voiceStatus = info->m_voiceStatus;
				m_playerList.setVOIPIcon(i, info->m_voiceStatus);
			}
			if (info->m_colorState != m_players[i]->m_colorState)
			{
				m_players[i]->m_colorState = info->m_colorState;
				m_playerList.setPlayerIcon(i, info->m_colorState);
			}
			if (info->m_name.compare(m_players[i]->m_name) != 0)
			{
				m_playerList.setButtonLabel(i, info->m_name);
				m_players[i]->m_name = info->m_name;
			}
			delete info;
		}
	}
}

void UIScene_ServerDashboard::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	ui.AnimateKeyPress(m_iPad, key, repeat, pressed, released);

	switch (key)
	{
	case ACTION_MENU_OK:
	case ACTION_MENU_UP:
	case ACTION_MENU_DOWN:
	case ACTION_MENU_PAGEUP:
	case ACTION_MENU_PAGEDOWN:
		sendInputToMovie(key, repeat, pressed, released);
		break;

	case ACTION_MENU_CANCEL:
		break;
	}
}

void UIScene_ServerDashboard::handlePress(F64 controlId, F64 childId)
{
	switch (static_cast<int>(controlId))
	{
	case eControl_GameOptions:
		ui.NavigateToScene(m_iPad, eUIScene_InGameHostOptionsMenu);
		break;

	case eControl_GamePlayers:
	{
		int sel = static_cast<int>(childId);
		if (sel < 0 || sel >= static_cast<int>(m_players.size()))
			break;

		INetworkPlayer *selectedPlayer = g_NetworkManager.GetPlayerBySmallId(m_players[sel]->m_smallId);
		if (selectedPlayer == nullptr)
			break;

		InGamePlayerOptionsInitData *pInitData = new InGamePlayerOptionsInitData();
		pInitData->iPad = m_iPad;
		pInitData->networkSmallId = m_players[sel]->m_smallId;
		pInitData->playerPrivileges = app.GetPlayerPrivileges(m_players[sel]->m_smallId);
		ui.NavigateToScene(m_iPad, eUIScene_InGamePlayerOptionsMenu, pInitData);
		break;
	}
	}
}

void UIScene_ServerDashboard::handleFocusChange(F64 controlId, F64 childId)
{
	switch (static_cast<int>(controlId))
	{
	case eControl_GamePlayers:
		m_playerList.updateChildFocus(static_cast<int>(childId));
		break;
	}
	updateTooltips();
}

void UIScene_ServerDashboard::OnPlayerChanged(void *callbackParam, INetworkPlayer *pPlayer, bool leaving)
{
	UIScene_ServerDashboard *scene = static_cast<UIScene_ServerDashboard *>(callbackParam);
	bool playerFound = false;
	int foundIndex = 0;

	for (int i = 0; i < static_cast<int>(scene->m_players.size()); ++i)
	{
		if (!playerFound && scene->m_players[i]->m_smallId == pPlayer->GetSmallId())
		{
			if (scene->m_playerList.getCurrentSelection() == scene->m_playerList.getItemCount() - 1)
				scene->m_playerList.setCurrentSelection(scene->m_playerList.getItemCount() - 2);

			playerFound = true;
			foundIndex = i;
		}
	}

	if (playerFound)
	{
		delete scene->m_players[foundIndex];
		scene->m_players.erase(scene->m_players.begin() + foundIndex);
		scene->m_playerList.removeItem(foundIndex);
	}

	if (!leaving)
	{
		PlayerInfo *info = scene->BuildPlayerInfo(pPlayer);
		scene->m_players.push_back(info);
		scene->m_playerList.addItem(info->m_name, info->m_colorState, info->m_voiceStatus);
	}
}

UIScene_ServerDashboard::PlayerInfo *UIScene_ServerDashboard::BuildPlayerInfo(INetworkPlayer *player)
{
	PlayerInfo *info = new PlayerInfo();
	info->m_smallId = player->GetSmallId();
	info->m_voiceStatus = 0;
	info->m_colorState = app.GetPlayerColour(player->GetSmallId());
	info->m_name = player->GetDisplayName();
	return info;
}

#endif // _DEDICATED_SERVER
