#include "stdafx.h"

#include <cstdio>
#include <iostream>
#include <string>
#include <shellapi.h>
#include <ShellScalingApi.h>

#include "MinecraftServer.h"
#include "Settings.h"
#include "Tesselator.h"

#include "Windows64\Network\WinsockNetLayer.h"
#include "Windows64\GameConfig\Minecraft.spa.h"

#include "UIScene_ServerConsole.h"

#include "..\Minecraft.World\AABB.h"
#include "..\Minecraft.World\Vec3.h"
#include "..\Minecraft.World\IntCache.h"
#include "..\Minecraft.World\compression.h"
#include "..\Minecraft.World\OldChunkStorage.h"
#include "..\Minecraft.World\TilePos.h"
#include "..\Minecraft.World\net.minecraft.world.level.tile.h"
#include "..\Minecraft.World\StringHelpers.h"

#include "Common\UI\UI.h"

BOOL  g_bWidescreen            = TRUE;

int   g_iScreenWidth           = 1920;
int   g_iScreenHeight          = 1080;
int   g_rScreenWidth           = 1280;
int   g_rScreenHeight          = 720;
float g_iAspectRatio           = 1920.0f / 1080.0f;

char    g_Win64Username[17]    = {};
wchar_t g_Win64UsernameW[17]   = {};

HINSTANCE g_hInst              = nullptr;
HWND      g_hWnd               = nullptr;
HINSTANCE hMyInst              = nullptr;

D3D_DRIVER_TYPE         g_driverType           = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel         = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice           = nullptr;
ID3D11DeviceContext*    g_pImmediateContext     = nullptr;
IDXGISwapChain*         g_pSwapChain           = nullptr;
ID3D11RenderTargetView* g_pRenderTargetView    = nullptr;
ID3D11DepthStencilView* g_pDepthStencilView    = nullptr;
ID3D11Texture2D*        g_pDepthStencilBuffer  = nullptr;

char     chGlobalText[256]     = {};
uint16_t ui16GlobalText[256]   = {};

class Renderer;
extern Renderer InternalRenderManager;

static constexpr int NUM_PROFILE_VALUES   = 5;
static constexpr int NUM_PROFILE_SETTINGS = 4;
static DWORD dwProfileSettingsA[NUM_PROFILE_VALUES] = { 0, 0, 0, 0, 0 };

static void CopyWideArgToAnsi(LPCWSTR source, char* dest, size_t destSize)
{
    if (destSize == 0) return;
    dest[0] = 0;
    if (source == nullptr) return;
    WideCharToMultiByte(CP_ACP, 0, source, -1, dest, static_cast<int>(destSize), nullptr, nullptr);
    dest[destSize - 1] = 0;
}

struct ServerLaunchOptions
{
    int  port = 0;
    char bindIP[256] = {};
};

static ServerLaunchOptions ParseServerLaunchOptions()
{
    ServerLaunchOptions opts{};

    g_Win64DedicatedServer     = true;
    g_Win64DedicatedServerPort = WIN64_NET_DEFAULT_PORT;
    g_Win64DedicatedServerBindIP[0] = 0;

    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr) return opts;

    for (int i = 1; i < argc; ++i)
    {
        if (_wcsicmp(argv[i], L"-name") == 0 && (i + 1) < argc)
        {
            CopyWideArgToAnsi(argv[++i], g_Win64Username, sizeof(g_Win64Username));
        }
        else if (_wcsicmp(argv[i], L"-ip") == 0 && (i + 1) < argc)
        {
            char ipBuf[256];
            CopyWideArgToAnsi(argv[++i], ipBuf, sizeof(ipBuf));
            strncpy_s(g_Win64DedicatedServerBindIP, sizeof(g_Win64DedicatedServerBindIP), ipBuf, _TRUNCATE);
            strncpy_s(opts.bindIP, sizeof(opts.bindIP), ipBuf, _TRUNCATE);
        }
        else if (_wcsicmp(argv[i], L"-port") == 0 && (i + 1) < argc)
        {
            wchar_t* endPtr = nullptr;
            const long p = wcstol(argv[++i], &endPtr, 10);
            if (endPtr != argv[i] && *endPtr == 0 && p > 0 && p <= 65535)
            {
                g_Win64DedicatedServerPort = static_cast<int>(p);
                opts.port = static_cast<int>(p);
            }
        }
    }

    LocalFree(argv);
    return opts;
}

static BOOL WINAPI ServerCtrlHandler(DWORD ctrlType)
{
    switch (ctrlType)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        app.m_bShutdown = true;
        MinecraftServer::HaltServer();
        return TRUE;
    default:
        return FALSE;
    }
}

static void AttachServerConsole()
{
    if (!AttachConsole(ATTACH_PARENT_PROCESS))
        AllocConsole();

    FILE* stream = nullptr;
    freopen_s(&stream, "CONIN$",  "r", stdin);
    freopen_s(&stream, "CONOUT$", "w", stdout);
    freopen_s(&stream, "CONOUT$", "w", stderr);

    SetConsoleTitleA("Minecraft Server");
    SetConsoleCtrlHandler(ServerCtrlHandler, TRUE);
}

static LRESULT CALLBACK ServerWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_CLOSE:
        app.m_bShutdown = true;
        MinecraftServer::HaltServer();
        DestroyWindow(hWnd);
        return 0;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        int vk = static_cast<int>(wParam);
        if ((lParam & 0x40000000) && vk != VK_LEFT && vk != VK_RIGHT && vk != VK_BACK)
            break;
        if (vk == VK_SHIFT)
            vk = (MapVirtualKey((lParam >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT) ? VK_RSHIFT : VK_LSHIFT;
        else if (vk == VK_CONTROL)
            vk = (lParam & (1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
        else if (vk == VK_MENU)
            vk = (lParam & (1 << 24)) ? VK_RMENU : VK_LMENU;
        g_KBMInput.OnKeyDown(vk);
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        int vk = static_cast<int>(wParam);
        if (vk == VK_SHIFT)
            vk = (MapVirtualKey((lParam >> 16) & 0xFF, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT) ? VK_RSHIFT : VK_LSHIFT;
        else if (vk == VK_CONTROL)
            vk = (lParam & (1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
        else if (vk == VK_MENU)
            vk = (lParam & (1 << 24)) ? VK_RMENU : VK_LMENU;
        g_KBMInput.OnKeyUp(vk);
        break;
    }
    case WM_CHAR:
        if (wParam >= 0x20 || wParam == 0x08 || wParam == 0x0D)
            g_KBMInput.OnChar(static_cast<wchar_t>(wParam));
        break;

    case WM_LBUTTONDOWN:
        g_KBMInput.OnMouseButtonDown(KeyboardMouseInput::MOUSE_LEFT);
        break;
    case WM_LBUTTONUP:
        g_KBMInput.OnMouseButtonUp(KeyboardMouseInput::MOUSE_LEFT);
        break;
    case WM_RBUTTONDOWN:
        g_KBMInput.OnMouseButtonDown(KeyboardMouseInput::MOUSE_RIGHT);
        break;
    case WM_RBUTTONUP:
        g_KBMInput.OnMouseButtonUp(KeyboardMouseInput::MOUSE_RIGHT);
        break;
    case WM_MBUTTONDOWN:
        g_KBMInput.OnMouseButtonDown(KeyboardMouseInput::MOUSE_MIDDLE);
        break;
    case WM_MBUTTONUP:
        g_KBMInput.OnMouseButtonUp(KeyboardMouseInput::MOUSE_MIDDLE);
        break;

    case WM_MOUSEMOVE:
        g_KBMInput.OnMouseMove(LOWORD(lParam), HIWORD(lParam));
        break;
    case WM_MOUSEWHEEL:
        g_KBMInput.OnMouseWheel(GET_WHEEL_DELTA_WPARAM(wParam));
        break;

    case WM_KILLFOCUS:
        g_KBMInput.ClearAllState();
        g_KBMInput.SetWindowFocused(false);
        break;
    case WM_SETFOCUS:
        g_KBMInput.SetWindowFocused(true);
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

static bool CreateServerWindow(HINSTANCE hInstance, int nCmdShow)
{
    WNDCLASSEXA wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = ServerWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = "MCServerClass";
    if (!RegisterClassExA(&wc)) return false;

    RECT wr = { 0, 0, g_rScreenWidth, g_rScreenHeight };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    g_hWnd = CreateWindowA(
        "MCServerClass", "Minecraft Server",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, hInstance, nullptr);
    if (!g_hWnd) return false;

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
    return true;
}

static bool InitD3D()
{
    RECT rc;
    GetClientRect(g_hWnd, &rc);
    const UINT width  = static_cast<UINT>(rc.right - rc.left);
    const UINT height = static_cast<UINT>(rc.bottom - rc.top);

    UINT createFlags = 0;
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] = {
        D3D_DRIVER_TYPE_HARDWARE, D3D_DRIVER_TYPE_WARP, D3D_DRIVER_TYPE_REFERENCE };
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0 };

    DXGI_SWAP_CHAIN_DESC sd{};
    sd.BufferCount                        = 1;
    sd.BufferDesc.Width                   = width;
    sd.BufferDesc.Height                  = height;
    sd.BufferDesc.Format                  = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator   = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage    = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow   = g_hWnd;
    sd.SampleDesc     = { 1, 0 };
    sd.Windowed       = TRUE;

    HRESULT hr = E_FAIL;
    for (auto dt : driverTypes)
    {
        g_driverType = dt;
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, dt, nullptr, createFlags,
            featureLevels, _countof(featureLevels), D3D11_SDK_VERSION,
            &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
        if (SUCCEEDED(hr)) break;
    }
    if (FAILED(hr)) return false;

    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
    if (FAILED(hr)) return false;

    hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
    pBackBuffer->Release();
    if (FAILED(hr)) return false;

    D3D11_TEXTURE2D_DESC dd{};
    dd.Width      = width;
    dd.Height     = height;
    dd.MipLevels  = 1;
    dd.ArraySize  = 1;
    dd.Format     = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dd.SampleDesc = { 1, 0 };
    dd.Usage      = D3D11_USAGE_DEFAULT;
    dd.BindFlags  = D3D11_BIND_DEPTH_STENCIL;

    hr = g_pd3dDevice->CreateTexture2D(&dd, nullptr, &g_pDepthStencilBuffer);
    if (FAILED(hr)) return false;

    D3D11_DEPTH_STENCIL_VIEW_DESC dsvd{};
    dsvd.Format        = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsvd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencilBuffer, &dsvd, &g_pDepthStencilView);
    if (FAILED(hr)) return false;

    g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

    D3D11_VIEWPORT vp{};
    vp.Width    = static_cast<FLOAT>(width);
    vp.Height   = static_cast<FLOAT>(height);
    vp.MaxDepth = 1.0f;
    g_pImmediateContext->RSSetViewports(1, &vp);

    return true;
}

static void CleanupD3D()
{
    if (g_pImmediateContext)   g_pImmediateContext->ClearState();
    if (g_pDepthStencilView)  { g_pDepthStencilView->Release();   g_pDepthStencilView  = nullptr; }
    if (g_pDepthStencilBuffer){ g_pDepthStencilBuffer->Release(); g_pDepthStencilBuffer = nullptr; }
    if (g_pRenderTargetView)  { g_pRenderTargetView->Release();   g_pRenderTargetView  = nullptr; }
    if (g_pSwapChain)         { g_pSwapChain->Release();          g_pSwapChain         = nullptr; }
    if (g_pImmediateContext)  { g_pImmediateContext->Release();   g_pImmediateContext  = nullptr; }
    if (g_pd3dDevice)         { g_pd3dDevice->Release();          g_pd3dDevice         = nullptr; }
}

static void DefineActions()
{
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_A,        _360_JOY_BUTTON_A);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_B,        _360_JOY_BUTTON_B);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_X,        _360_JOY_BUTTON_X);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_Y,        _360_JOY_BUTTON_Y);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_OK,       _360_JOY_BUTTON_A);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_CANCEL,   _360_JOY_BUTTON_B);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_UP,       _360_JOY_BUTTON_DPAD_UP   | _360_JOY_BUTTON_LSTICK_UP);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_DOWN,     _360_JOY_BUTTON_DPAD_DOWN | _360_JOY_BUTTON_LSTICK_DOWN);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_LEFT,     _360_JOY_BUTTON_DPAD_LEFT | _360_JOY_BUTTON_LSTICK_LEFT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_RIGHT,    _360_JOY_BUTTON_DPAD_RIGHT| _360_JOY_BUTTON_LSTICK_RIGHT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_PAGEUP,   _360_JOY_BUTTON_LT);
    InputManager.SetGameJoypadMaps(MAP_STYLE_0, ACTION_MENU_PAGEDOWN, _360_JOY_BUTTON_RT);
}

static Minecraft* InitialiseServerRuntime()
{
    app.loadMediaArchive();

    RenderManager.Initialise(g_pd3dDevice, g_pSwapChain);

    app.loadStringTable();
    ui.init(g_pd3dDevice, g_pImmediateContext, g_pRenderTargetView,
            g_pDepthStencilView, g_rScreenWidth, g_rScreenHeight);

    InputManager.Initialise(1, 3, MINECRAFT_ACTION_MAX, ACTION_MAX_MENU);
    g_KBMInput.Init();
    DefineActions();
    InputManager.SetJoypadMapVal(0, 0);
    InputManager.SetKeyRepeatRate(0.3f, 0.2f);

    ProfileManager.Initialise(
        TITLEID_MINECRAFT,
        app.m_dwOfferID,
        PROFILE_VERSION_10,
        NUM_PROFILE_VALUES,
        NUM_PROFILE_SETTINGS,
        dwProfileSettingsA,
        app.GAME_DEFINED_PROFILE_DATA_BYTES * XUSER_MAX_COUNT,
        &app.uiGameDefinedDataChangedBitmask);
    ProfileManager.SetDefaultOptionsCallback(
        &CConsoleMinecraftApp::DefaultOptionsCallback, static_cast<LPVOID>(&app));

    g_NetworkManager.Initialise();

    for (int i = 0; i < MINECRAFT_NET_MAX_PLAYERS; ++i)
    {
        IQNet::m_player[i].m_smallId      = static_cast<BYTE>(i);
        IQNet::m_player[i].m_isRemote     = false;
        IQNet::m_player[i].m_isHostPlayer  = (i == 0);
        swprintf_s(IQNet::m_player[i].m_gamertag, 32, L"Player%d", i);
    }
    wcscpy_s(IQNet::m_player[0].m_gamertag, 32, g_Win64UsernameW);

    WinsockNetLayer::Initialize();
    ProfileManager.SetDebugFullOverride(true);

    Tesselator::CreateNewThreadStorage(1024 * 1024);
    AABB::CreateNewThreadStorage();
    Vec3::CreateNewThreadStorage();
    IntCache::CreateNewThreadStorage();
    Compression::CreateNewThreadStorage();
    OldChunkStorage::CreateNewThreadStorage();
    Level::enableLightingCache();
    Tile::CreateNewThreadStorage();

    Minecraft::main();
    Minecraft* pMinecraft = Minecraft::GetInstance();
    if (pMinecraft == nullptr) return nullptr;

    app.InitGameSettings();
    app.InitialiseTips();

    return pMinecraft;
}

static int ConsoleInputThreadProc(void* /*unused*/)
{
    std::string line;
    while (!app.m_bShutdown)
    {
        if (!std::getline(std::cin, line))
        {
            if (std::cin.eof()) break;
            std::cin.clear();
            Sleep(50);
            continue;
        }

        wstring command = trimString(convStringToWstring(line));
        if (command.empty()) continue;

        MinecraftServer* server = MinecraftServer::getInstance();
        if (server != nullptr)
            server->handleConsoleInput(command, server);
    }
    return 0;
}

void MemSect(int /*sect*/) {}
void SeedEditBox() {}
void ClearGlobalText() { memset(chGlobalText, 0, 256); memset(ui16GlobalText, 0, 512); }
uint16_t* GetGlobalText()
{
    char* p = reinterpret_cast<char*>(ui16GlobalText);
    for (int i = 0; i < 256; ++i) p[i * 2] = chGlobalText[i];
    return ui16GlobalText;
}
void ToggleFullscreen() {}

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE /*hPrevInstance*/,
                     _In_ LPSTR  /*lpCmdLine*/,
                     _In_ int       nCmdShow)
{
    {
        char szExeDir[MAX_PATH]{};
        GetModuleFileNameA(nullptr, szExeDir, MAX_PATH);
        char* p = strrchr(szExeDir, '\\');
        if (p) { *(p + 1) = '\0'; SetCurrentDirectoryA(szExeDir); }
    }

    SetProcessDPIAware();
    g_rScreenWidth  = 1280;
    g_rScreenHeight = 720;

    g_hInst = hInstance;
    hMyInst = hInstance;

    AttachServerConsole();

    const auto launchOpts = ParseServerLaunchOptions();

    {
        char exePath[MAX_PATH]{};
        GetModuleFileNameA(nullptr, exePath, MAX_PATH);
        char* lastSlash = strrchr(exePath, '\\');
        if (lastSlash) *(lastSlash + 1) = '\0';

        char filePath[MAX_PATH]{};
        _snprintf_s(filePath, sizeof(filePath), _TRUNCATE, "%susername.txt", exePath);

        FILE* f = nullptr;
        if (fopen_s(&f, filePath, "r") == 0 && f)
        {
            char buf[128]{};
            if (fgets(buf, sizeof(buf), f))
            {
                auto len = static_cast<int>(strlen(buf));
                while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r' || buf[len - 1] == ' '))
                    buf[--len] = '\0';
                if (len > 0)
                    strncpy_s(g_Win64Username, sizeof(g_Win64Username), buf, _TRUNCATE);
            }
            fclose(f);
        }
    }
    if (g_Win64Username[0] == 0)
        strncpy_s(g_Win64Username, sizeof(g_Win64Username), "Server", _TRUNCATE);
    MultiByteToWideChar(CP_ACP, 0, g_Win64Username, -1, g_Win64UsernameW, 17);

    if (!CreateServerWindow(hInstance, nCmdShow))
    {
        fprintf(stderr, "Failed to create the server window.\n");
        return 1;
    }

    if (!InitD3D())
    {
        fprintf(stderr, "Failed to initialise D3D.\n");
        CleanupD3D();
        return 1;
    }

    Settings serverSettings(new File(L"server.properties"));
    const wstring configuredBindIp = serverSettings.getString(L"server-ip", L"");

    const char* bindIp = "*";
    if (g_Win64DedicatedServerBindIP[0] != 0)
        bindIp = g_Win64DedicatedServerBindIP;
    else if (!configuredBindIp.empty())
        bindIp = wstringtochararray(configuredBindIp);

    const int port = (launchOpts.port > 0)
        ? launchOpts.port
        : serverSettings.getInt(L"server-port", WIN64_NET_DEFAULT_PORT);

    printf("Minecraft Server starting on %s:%d\n", bindIp, port);
    fflush(stdout);

    Minecraft* pMinecraft = InitialiseServerRuntime();
    if (pMinecraft == nullptr)
    {
        fprintf(stderr, "Failed to initialise the Minecraft runtime.\n");
        CleanupD3D();
        return 1;
    }

    app.SetGameHostOption(eGameHostOption_Difficulty,   serverSettings.getInt(L"difficulty", 1));
    app.SetGameHostOption(eGameHostOption_Gamertags,    1);
    app.SetGameHostOption(eGameHostOption_GameType,     serverSettings.getInt(L"gamemode", 0));
    app.SetGameHostOption(eGameHostOption_LevelType,    0);
    app.SetGameHostOption(eGameHostOption_Structures,   serverSettings.getBoolean(L"generate-structures", true) ? 1 : 0);
    app.SetGameHostOption(eGameHostOption_BonusChest,   serverSettings.getBoolean(L"bonus-chest", false) ? 1 : 0);
    app.SetGameHostOption(eGameHostOption_PvP,          serverSettings.getBoolean(L"pvp", true) ? 1 : 0);
    app.SetGameHostOption(eGameHostOption_TrustPlayers, serverSettings.getBoolean(L"trust-players", true) ? 1 : 0);
    app.SetGameHostOption(eGameHostOption_FireSpreads,  serverSettings.getBoolean(L"fire-spreads", true) ? 1 : 0);
    app.SetGameHostOption(eGameHostOption_TNT,          serverSettings.getBoolean(L"tnt", true) ? 1 : 0);
    app.SetGameHostOption(eGameHostOption_HostCanFly,           1);
    app.SetGameHostOption(eGameHostOption_HostCanChangeHunger,  1);
    app.SetGameHostOption(eGameHostOption_HostCanBeInvisible,   1);
    app.SetGameHostOption(eGameHostOption_MobGriefing,          1);
    app.SetGameHostOption(eGameHostOption_KeepInventory,        0);
    app.SetGameHostOption(eGameHostOption_DoMobSpawning,        1);
    app.SetGameHostOption(eGameHostOption_DoMobLoot,            1);
    app.SetGameHostOption(eGameHostOption_DoTileDrops,          1);
    app.SetGameHostOption(eGameHostOption_NaturalRegeneration,  1);
    app.SetGameHostOption(eGameHostOption_DoDaylightCycle,      1);

    MinecraftServer::resetFlags();
    g_NetworkManager.HostGame(0, false, true, MINECRAFT_NET_MAX_PLAYERS, 0);

    if (!WinsockNetLayer::IsActive())
    {
        fprintf(stderr, "Failed to bind the server socket on %s:%d.\n", bindIp, port);
        CleanupD3D();
        return 1;
    }

    g_NetworkManager.FakeLocalPlayerJoined();

    auto* param     = new NetworkGameInitData();
    param->seed     = 0;
    param->settings = app.GetGameHostOption(eGameHostOption_All);

    g_NetworkManager.ServerStoppedCreate(true);
    g_NetworkManager.ServerReadyCreate(true);

    auto* serverThread = new C4JThread(
        &CGameNetworkManager::ServerThreadProc, param, "Server", 256 * 1024);
    serverThread->SetProcessor(CPU_CORE_SERVER);
    serverThread->Run();

    g_NetworkManager.ServerReadyWait();
    g_NetworkManager.ServerReadyDestroy();

    if (MinecraftServer::serverHalted())
    {
        fprintf(stderr, "The server halted during startup.\n");
        g_NetworkManager.LeaveGame(false);
        CleanupD3D();
        return 1;
    }

    app.SetGameStarted(true);
    g_NetworkManager.DoWork();

    printf("Server ready on %s:%d\n", bindIp, port);
    printf("Type 'help' for server commands.\n");
    fflush(stdout);

    UIScene_ServerConsole::InitLog();
    UIScene_ServerConsole::AddLogLine(L"Server ready on " + convStringToWstring(string(bindIp)) + L":" + to_wstring(port));
    UIScene_ServerConsole::AddLogLine(L"Type 'help' for server commands.");
    ui.NavigateToScene(0, eUIScene_ServerDashboard);

    auto* consoleThread = new C4JThread(
        &ConsoleInputThreadProc, nullptr, "Server console", 128 * 1024);
    consoleThread->Run();

    MSG msg{};
    while (WM_QUIT != msg.message && !app.m_bShutdown && !MinecraftServer::serverHalted())
    {
        g_KBMInput.Tick();

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) break;
        }
        if (msg.message == WM_QUIT) break;

        g_KBMInput.SetKBMActive(true);
        if (!g_KBMInput.IsMouseGrabbed())
            g_KBMInput.SetCursorHiddenForUI(false);

        RenderManager.StartFrame();

        app.UpdateTime();
        InputManager.Tick();
        ProfileManager.Tick();
        StorageManager.Tick();
        RenderManager.Tick();
        g_NetworkManager.DoWork();
        app.HandleXuiActions();

        float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, clearColor);
        g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        ui.tick();
        ui.render();

        g_pSwapChain->Present(1, 0);

        ui.CheckMenuDisplayed();

        Vec3::resetPool();
    }

    printf("Stopping server...\n");
    fflush(stdout);

    app.m_bShutdown = true;
    MinecraftServer::HaltServer();
    g_NetworkManager.LeaveGame(false);

    CleanupD3D();
    return 0;
}
