#include "stdafx.h"
#ifdef _DEDICATED_SERVER

#include "Common/UI/UI.h"
#include "UIScene_ServerConsole.h"
#include "MinecraftServer.h"
#include "../Minecraft.World/StringHelpers.h"

deque<wstring> UIScene_ServerConsole::s_globalLog;
CRITICAL_SECTION UIScene_ServerConsole::s_logCS;
bool UIScene_ServerConsole::s_csInit = false;

void UIScene_ServerConsole::InitLog()
{
	if (!s_csInit)
	{
		InitializeCriticalSection(&s_logCS);
		s_csInit = true;
	}
}

void UIScene_ServerConsole::AddLogLine(const wstring &line)
{
	if (!s_csInit) return;
	EnterCriticalSection(&s_logCS);
	if (s_globalLog.size() >= 200) s_globalLog.pop_front();
	s_globalLog.push_back(line);
	LeaveCriticalSection(&s_logCS);
}

UIScene_ServerConsole::UIScene_ServerConsole(int iPad, void *initData, UILayer *parentLayer)
	: UIScene(iPad, parentLayer)
	, m_cursorPos(0)
	, m_cursorBlink(0)
	, m_logDirty(true)
{
	InitLog();
	initialiseMovie();

	EnterCriticalSection(&s_logCS);
	m_logLines = s_globalLog;
	LeaveCriticalSection(&s_logCS);

	refreshLabels();
	updateInputDisplay();
}

wstring UIScene_ServerConsole::getMoviePath()
{
	return L"DebugUIConsoleComponent";
}

void UIScene_ServerConsole::updateTooltips()
{
	ui.SetTooltips(m_iPad, -1, IDS_TOOLTIPS_BACK, -1, -1);
}

void UIScene_ServerConsole::tick()
{
	UIScene::tick();

	EnterCriticalSection(&s_logCS);
	size_t globalSize = s_globalLog.size();
	size_t localSize = m_logLines.size();
	if (globalSize > localSize)
	{
		for (size_t i = localSize; i < globalSize; ++i)
			m_logLines.push_back(s_globalLog[i]);
		m_logDirty = true;
	}
	LeaveCriticalSection(&s_logCS);

	++m_cursorBlink;
	if (m_cursorBlink >= 60) m_cursorBlink = 0;

	if (m_logDirty)
	{
		refreshLabels();
		m_logDirty = false;
	}

	updateInputDisplay();
}

void UIScene_ServerConsole::refreshLabels()
{
	int logCount = static_cast<int>(m_logLines.size());
	int displayLines = SERVER_CONSOLE_LINES - 1;

	for (int i = 0; i < displayLines; ++i)
	{
		int logIdx = logCount - displayLines + i;
		if (logIdx >= 0 && logIdx < logCount)
		{
			m_labels[i].setLabel(m_logLines[logIdx]);
		}
		else
		{
			m_labels[i].setLabel(wstring());
		}
	}
}

void UIScene_ServerConsole::updateInputDisplay()
{
	wstring display = L"> " + m_inputBuffer;
	bool showCursor = (m_cursorBlink < 30);
	if (showCursor)
	{
		if (m_cursorPos >= static_cast<int>(m_inputBuffer.size()))
			display += L"_";
		else
			display = L"> " + m_inputBuffer.substr(0, m_cursorPos) + L"_" + m_inputBuffer.substr(m_cursorPos);
	}
	m_labels[SERVER_CONSOLE_LINES - 1].setLabel(display);
}

void UIScene_ServerConsole::submitCommand()
{
	if (m_inputBuffer.empty()) return;

	wstring cmd = m_inputBuffer;
	m_inputBuffer.clear();
	m_cursorPos = 0;
	m_cursorBlink = 0;

	AddLogLine(L"> " + cmd);
	m_logDirty = true;

	MinecraftServer *server = MinecraftServer::getInstance();
	if (server != nullptr)
		server->handleConsoleInput(cmd, server);
}

void UIScene_ServerConsole::handleInput(int iPad, int key, bool repeat, bool pressed, bool released, bool &handled)
{
	if (!pressed) return;

	switch (key)
	{
	case ACTION_MENU_CANCEL:
		if (!repeat)
			navigateBack();
		handled = true;
		break;

	default:
		handled = true;
		break;
	}
}

#endif // _DEDICATED_SERVER
