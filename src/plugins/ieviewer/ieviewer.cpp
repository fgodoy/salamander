// SPDX-FileCopyrightText: 2023 Open Salamander Authors
// SPDX-License-Identifier: GPL-2.0-or-later

#include "precomp.h"

#include <comutil.h>

#include "ieviewer.h"
#include "ieviewer.rh"
#include "ieviewer.rh2"
#include "lang\lang.rh"
#include "dbg.h"

#include "markdown.h"

#pragma comment(lib, "comsuppw.lib")

// plugin interface object, its methods are called from Salamander
CPluginInterface PluginInterface;
// CPluginInterface portion for the viewer
CPluginInterfaceForViewer InterfaceForViewer;

const char* WINDOW_CLASSNAME = "IE Viewer Class";
ATOM AtomObject = 0;                                         // window "property" with a pointer to the object
CIEMainWindowQueue CIEMainWindow::ViewerWindowQueue;         // list of all viewer windows
CThreadQueue CIEMainWindow::ThreadQueue("IEViewer Viewers"); // list of all window threads

HINSTANCE DLLInstance = NULL; // handle to the SPL module - language-independent resources
HINSTANCE HLanguage = NULL;   // handle to the SLG module - language-dependent resources

int ConfigVersion = 0;           // 0 - default, 1 - SS 1.6 beta 3, 2 - SS 1.6 beta 4, 3 - SS 2.5 beta 1, 4 - AS 3.1 beta 1
#define CURRENT_CONFIG_VERSION 4 // AS 3.1 beta 1
const char* CONFIG_VERSION = "Version";

// Salamander general interface - valid from startup until the plugin shuts down
CSalamanderGeneralAbstract* SalamanderGeneral = NULL;

// variable definition for "dbg.h"
CSalamanderDebugAbstract* SalamanderDebug = NULL;

// variable definition for "spl_com.h"
int SalamanderVersion = 0;

char GetStringFromIIDBuffer[MAX_PATH];
char* GetStringFromIID(REFIID riid)
{
    CALL_STACK_MESSAGE1("GetStringFromIID()");
    LPOLESTR lplpsz;
    if (StringFromIID(riid, &lplpsz) == E_OUTOFMEMORY)
    {
        TRACE_E("Low memory");
        *GetStringFromIIDBuffer = 0;
    }
    else
    {
        WideCharToMultiByte(CP_ACP,
                            0,
                            lplpsz,
                            -1,
                            GetStringFromIIDBuffer,
                            MAX_PATH,
                            NULL,
                            NULL);
        GetStringFromIIDBuffer[MAX_PATH - 1] = 0;

        //free the string
        LPMALLOC pMalloc;
        CoGetMalloc(1, &pMalloc);
        if (pMalloc != NULL)
        {
            pMalloc->Free(lplpsz);
            pMalloc->Release();
        }
    }
    return GetStringFromIIDBuffer;
}

//DeleteInterfaceImp calls 'delete' and NULLs the pointer
#define DeleteInterfaceImp(p) \
    { \
        if (p != NULL) \
        { \
            delete p; \
            p = NULL; \
        } \
    }

//ReleaseInterface calls 'Release' and NULLs the pointer
#define ReleaseInterface(p) \
    { \
        IUnknown* pt = (IUnknown*)p; \
        p = NULL; \
        if (pt != NULL) \
            pt->Release(); \
    }

//
// ****************************************************************************
// DllMain
//

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
        DLLInstance = hinstDLL;
    return TRUE; // DLL can be loaded
}

//
// ****************************************************************************
// LoadStr
//

char* LoadStr(int resID)
{
    return SalamanderGeneral->LoadStr(HLanguage, resID);
}

//
// ****************************************************************************
// SalamanderPluginGetReqVer
//

int WINAPI SalamanderPluginGetReqVer()
{
    return LAST_VERSION_OF_SALAMANDER;
}

//
// ****************************************************************************
// UpdateInternetFeatureControls
//

void WriteFeatureControl(const char* feature, const char* exe, DWORD value)
{
    char buff[1000];
    wsprintf(buff, "Software\\Microsoft\\Internet Explorer\\Main\\FeatureControl\\%s", feature);
    HKEY hKey;
    DWORD disp;
    RegCreateKeyEx(HKEY_CURRENT_USER, buff, 0, NULL,
                   REG_OPTION_NON_VOLATILE, KEY_CREATE_SUB_KEY | KEY_WRITE, NULL, &hKey, &disp);
    if (hKey != NULL)
    {
        RegSetValueEx(hKey, exe, 0, REG_DWORD, (LPBYTE)&value, sizeof(value));
        RegCloseKey(hKey);
    }
    else
    {
        TRACE_E("RegCreateKeyEx() failed");
    }
}

void UpdateInternetFeatureControl()
{
    //Internet Feature Control Keys
    //https://msdn.microsoft.com/en-us/library/ee330720%28v=vs.85%29.aspx

    // MSDN mentions that FEATURE_96DPI_PIXEL is deprecated and replaced by DOCHOSTUIFLAG_DPI_AWARE
    // https://msdn.microsoft.com/en-us/library/aa753277%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
    // but it still works under W10/IE11, so I am ignoring it

    char exePath[MAX_PATH];
    if (!GetModuleFileName(NULL, exePath, MAX_PATH))
        exePath[0] = 0;
    char* s = strrchr(exePath, '\\');
    if (s == NULL)
    {
        TRACE_E("GetModuleFileName() failed");
        return;
    }
    s++;

    WriteFeatureControl("FEATURE_96DPI_PIXEL", s, 1);                      // enable High-DPI support
    WriteFeatureControl("FEATURE_BROWSER_EMULATION", s, 11000);            // enable latest IE engine
    WriteFeatureControl("FEATURE_ENABLE_CLIPCHILDREN_OPTIMIZATION", s, 1); // enable Child Window Clipping
    WriteFeatureControl("FEATURE_GPU_RENDERING", s, 1);                    // enable GPU rendering
    WriteFeatureControl("FEATURE_AJAX_CONNECTIONEVENTS", s, 1);            // enable AJAX Connection Events
    WriteFeatureControl("FEATURE_DISABLE_NAVIGATION_SOUNDS", s, 1);        // disable navigation sounds
}

//
// ****************************************************************************
// SalamanderPluginEntry
//

CPluginInterfaceAbstract* WINAPI SalamanderPluginEntry(CSalamanderPluginEntryAbstract* salamander)
{
    // configure SalamanderDebug for "dbg.h"
    SalamanderDebug = salamander->GetSalamanderDebug();
    // configure SalamanderVersion for "spl_com.h"
    SalamanderVersion = salamander->GetVersion();

    CALL_STACK_MESSAGE1("SalamanderPluginEntry()");

    // this plugin is built for the current Salamander version and newer - perform a check
    if (SalamanderVersion < LAST_VERSION_OF_SALAMANDER)
    { // reject older versions
        MessageBox(salamander->GetParentWindow(),
                   REQUIRE_LAST_VERSION_OF_SALAMANDER,
                   "Internet Explorer Viewer" /* neprekladat! */, MB_OK | MB_ICONERROR);
        return NULL;
    }

    // let it load the language module (.slg)
    HLanguage = salamander->LoadLanguageModule(salamander->GetParentWindow(), "Internet Explorer Viewer" /* neprekladat! */);
    if (HLanguage == NULL)
        return NULL;

    // obtain the general Salamander interface
    SalamanderGeneral = salamander->GetSalamanderGeneral();

    // enable rendering in the current IE engine, High-DPI support, GPU acceleration, ...
    static BOOL ifcUpdated = FALSE;
    if (!ifcUpdated)
    {
        UpdateInternetFeatureControl();
        ifcUpdated = TRUE;
    }

    if (!InitViewer())
        return NULL; // error

    // configure the basic plugin information
    salamander->SetBasicPluginData(LoadStr(IDS_PLUGINNAME),
                                   FUNCTION_LOADSAVECONFIGURATION | FUNCTION_VIEWER,
                                   VERSINFO_VERSION_NO_PLATFORM,
                                   VERSINFO_COPYRIGHT,
                                   LoadStr(IDS_PLUGIN_DESCRIPTION),
                                   "IEVIEWER");

    salamander->SetPluginHomePageURL("www.altap.cz");

    return &PluginInterface;
}

//
// ****************************************************************************
// CPluginInterface
//

void CPluginInterface::About(HWND parent)
{
    char buf[1000];
    _snprintf_s(buf, _TRUNCATE,
                "%s " VERSINFO_VERSION "\n\n" VERSINFO_COPYRIGHT "\n\n"
                "%s",
                LoadStr(IDS_PLUGINNAME),
                LoadStr(IDS_PLUGIN_DESCRIPTION));
    SalamanderGeneral->SalMessageBox(parent, buf, LoadStr(IDS_ABOUT), MB_OK | MB_ICONINFORMATION);
}

BOOL CPluginInterface::Release(HWND parent, BOOL force)
{
    CALL_STACK_MESSAGE2("CPluginInterface::Release(, %d)", force);
    BOOL ret = CIEMainWindow::ViewerWindowQueue.Empty();
    if (!ret)
    {
        ret = CIEMainWindow::ViewerWindowQueue.CloseAllWindows(force) || force;
    }
    if (ret)
    {
        if (!CIEMainWindow::ThreadQueue.KillAll(force) && !force)
            ret = FALSE;
        else
            ReleaseViewer();
    }
    return ret;
}

void CPluginInterface::LoadConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    CALL_STACK_MESSAGE1("CPluginInterface::LoadConfiguration(, ,)");
    if (regKey != NULL) // load from the registry
    {
        if (!registry->GetValue(regKey, CONFIG_VERSION, REG_DWORD, &ConfigVersion, sizeof(DWORD)))
        {
            ConfigVersion = CURRENT_CONFIG_VERSION; // probably some troublemaker... ;-)
        }
    }
    else // default configuration
    {
        ConfigVersion = 0;
    }
}

void CPluginInterface::SaveConfiguration(HWND parent, HKEY regKey, CSalamanderRegistryAbstract* registry)
{
    DWORD v = CURRENT_CONFIG_VERSION;
    registry->SetValue(regKey, CONFIG_VERSION, REG_DWORD, &v, sizeof(DWORD));
}

const char* MARKDOWN_EXTENSIONS = "*.md;*.mdown;*.markdown";
const char* CODE_EXTENSIONS = "*.c;*.cpp;*.h;*.hpp;*.cc;*.cs;*.java;*.py;*.go;*.rs;*.rb;*.php;*.swift;*.kt;*.kts;*.js;*.mjs;*.cjs;*.ts;*.tsx;*.sql;*.json;*.xml;*.xsd;*.yaml;*.yml;*.sh;*.bat;*.cmd;*.ps1";

void CPluginInterface::Connect(HWND parent, CSalamanderConnectAbstract* salamander)
{
    CALL_STACK_MESSAGE1("CPluginInterface::Connect(,)");

    char buff[1000];
    sprintf_s(buff, "*.htm;*.html;*.xml;*.mht;%s", MARKDOWN_EXTENSIONS);

    salamander->AddViewer(buff, FALSE); // default (plugin installation), otherwise Salamander ignores it

    if (ConfigVersion < 2) // before SS 1.6 beta 4
    {
        salamander->AddViewer("*.xml", TRUE);   // add *.xml (up to 1.6 beta 3, otherwise already there, will not be added)
        salamander->ForceRemoveViewer("*.jpg"); // remove *.jpg (from beta 3, otherwise not present)
        salamander->ForceRemoveViewer("*.gif"); // remove *.gif (from beta 3, otherwise not present)
    }

    if (ConfigVersion < 3) // before SS 2.5 beta 1
    {
        salamander->AddViewer("*.mht", TRUE); // add *.mht (through 2.5 beta 1, otherwise already there, will not be added)
    }

    if (ConfigVersion < 4) // before AD 3.1 beta 1
    {
        salamander->AddViewer(MARKDOWN_EXTENSIONS, TRUE); // support for Markdown
    }
    
    salamander->AddViewer(CODE_EXTENSIONS, TRUE); // support for Syntax Highlighted Code
}

CPluginInterfaceForViewerAbstract*
CPluginInterface::GetInterfaceForViewer()
{
    return &InterfaceForViewer;
}

struct CTVData
{
    BOOL AlwaysOnTop;
    const char* Name;
    IStream* ContentStream;
    int Left, Top, Width, Height;
    UINT ShowCmd;
    BOOL ReturnLock;
    HANDLE* Lock;
    BOOL* LockOwner;
    BOOL Success;
    HANDLE Continue;
};

unsigned WINAPI ThreadIEMessageLoop(void* param)
{
    CALL_STACK_MESSAGE1("ThreadIEMessageLoop(Version 1.09)");
    SetThreadNameInVCAndTrace("IELoop");
    TRACE_I("Begin");

    CTVData* data = (CTVData*)param;

    CIEMainWindow* window = new CIEMainWindow;
    if (window != NULL)
    {
        if (data->ReturnLock)
        {
            *data->Lock = window->GetLock();
            *data->LockOwner = TRUE;
        }
        CALL_STACK_MESSAGE1("ThreadIEMessageLoop::CreateWindowEx");
        if ((!data->ReturnLock || *data->Lock != NULL) &&
            CreateWindowEx(data->AlwaysOnTop ? WS_EX_TOPMOST : 0,
                           WINDOW_CLASSNAME,
                           LoadStr(IDS_PLUGINNAME),
                           WS_OVERLAPPEDWINDOW,
                           data->Left,
                           data->Top,
                           data->Width,
                           data->Height,
                           NULL,
                           NULL,
                           DLLInstance,
                           window) != NULL)
        {
            SendMessage(window->HWindow, WM_SETICON, ICON_BIG,
                        (LPARAM)LoadIcon(DLLInstance, MAKEINTRESOURCE(IDI_IEVIEWER)));
            SendMessage(window->HWindow, WM_SETICON, ICON_SMALL,
                        (LPARAM)LoadIcon(DLLInstance, MAKEINTRESOURCE(IDI_IEVIEWER)));
            CALL_STACK_MESSAGE1("ThreadIEMessageLoop::ShowWindow");
            ShowWindow(window->HWindow, data->ShowCmd);
            SetForegroundWindow(window->HWindow);
            UpdateWindow(window->HWindow);
            data->Success = TRUE;
        }
        else
        {
            CALL_STACK_MESSAGE1("ThreadIEMessageLoop::delete-window");
            if (data->ReturnLock && *data->Lock != NULL)
                CloseHandle(*data->Lock);
            delete window;
            window = NULL;
        }
    }

    CALL_STACK_MESSAGE1("ThreadIEMessageLoop::SetEvent");
    char name[MAX_PATH];
    lstrcpyn(name, data->Name, MAX_PATH);
    IStream* contentStream = data->ContentStream;
    BOOL openFile = data->Success;
    SetEvent(data->Continue); // let the main thread continue; data are invalid from this point (=NULL)
    data = NULL;

    // if everything succeeded, open the requested file in the window
    if (window != NULL && openFile)
    {
        CALL_STACK_MESSAGE1("ThreadIEMessageLoop::Navigate");
        if (contentStream != NULL)
            window->m_IEViewer.Navigate(name, contentStream);
        else
            window->m_IEViewer.Navigate(name, NULL);

        CALL_STACK_MESSAGE1("ThreadIEMessageLoop::message-loop");
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0))
        {
            {
                CALL_STACK_MESSAGE5("MSG(0x%p, 0x%X, 0x%IX, 0x%IX)", msg.hwnd, msg.message, msg.wParam, msg.lParam);
                if (window->m_IEViewer.TranslateAccelerator(&msg) != S_OK)
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
    }

    // probably a common bug in IE - the object was originally destroyed in response to WM_DESTROY
    // but a message arrived before the message pump finished and hit the destroyed
    // object
    // therefore I moved the window destruction here - similar to ooStatic from WinLib
    CALL_STACK_MESSAGE1("ThreadIEMessageLoop::message_loop done");
    delete window;

    TRACE_I("End");
    return 0;
}

enum FileFormatEnum
{
    ffeHTML,
    ffeMarkdown,
    ffeCode
};

FileFormatEnum GetFileFormat(const char* name)
{
    // naive detection by extension; it probably deserves content-based heuristics
    FileFormatEnum ret = ffeHTML;
    CSalamanderMaskGroup* masks = SalamanderGeneral->AllocSalamanderMaskGroup();
    if (masks != NULL)
    {
        masks->SetMasksString(MARKDOWN_EXTENSIONS, FALSE);
        int err;
        if (masks->PrepareMasks(err) && masks->AgreeMasks(name, NULL))
        {
            ret = ffeMarkdown;
        }
        else
        {
            masks->SetMasksString(CODE_EXTENSIONS, FALSE);
            if (masks->PrepareMasks(err) && masks->AgreeMasks(name, NULL))
                ret = ffeCode;
        }
        SalamanderGeneral->FreeSalamanderMaskGroup(masks);
    }
    return ret;
}

BOOL CPluginInterfaceForViewer::ViewFile(const char* name, int left, int top, int width,
                                         int height, UINT showCmd, BOOL alwaysOnTop,
                                         BOOL returnLock, HANDLE* lock, BOOL* lockOwner,
                                         CSalamanderPluginViewerData* viewerData,
                                         int enumFilesSourceUID, int enumFilesCurrentIndex)
{
    CALL_STACK_MESSAGE11("CPluginInterfaceForViewer::ViewFile(%s, %d, %d, %d, %d, "
                         "0x%X, %d, %d, , , , %d, %d)",
                         name, left, top, width, height,
                         showCmd, alwaysOnTop, returnLock, enumFilesSourceUID, enumFilesCurrentIndex);

    FileFormatEnum fileFormat = GetFileFormat(name);

    CTVData data;
    data.AlwaysOnTop = alwaysOnTop;
    data.Name = name;
    data.ContentStream = NULL;
    if (fileFormat == ffeMarkdown)
        data.ContentStream = ConvertMarkdownToHTML(name); // if it returns NULL, display the file as HTML
    else if (fileFormat == ffeCode)
        data.ContentStream = ConvertCodeToHTML(name);
    data.Left = left;
    data.Top = top;
    data.Width = width;
    data.Height = height;
    data.ShowCmd = showCmd;
    data.ReturnLock = returnLock;
    data.Lock = lock;
    data.LockOwner = lockOwner;
    data.Success = FALSE;
    data.Continue = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (data.Continue == NULL)
    {
        TRACE_E("Failed to create the Continue event.");
        return FALSE;
    }

    if (CIEMainWindow::ThreadQueue.StartThread(ThreadIEMessageLoop, &data))
    {
        // wait until the thread processes the passed data and returns results
        WaitForSingleObject(data.Continue, INFINITE);
    }
    else
        data.Success = FALSE;
    CloseHandle(data.Continue);

    if (!data.Success)
    {
        SalamanderGeneral->SalMessageBox(NULL, LoadStr(IDS_UNABLETOOPENIE), LoadStr(IDS_ERRORTITLE),
                                         MB_ICONEXCLAMATION | MB_OK | MB_SETFOREGROUND);
    }

    return data.Success;
}

//
// ****************************************************************************
// InitViewer & ReleaseViewer
//

LRESULT CALLBACK WebView2HostWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

BOOL InitViewer()
{
    CALL_STACK_MESSAGE1("InitViewer()");
    AtomObject = GlobalAddAtom("object handle");
    if (AtomObject == 0)
    {
        TRACE_E("GlobalAddAtom has failed");
        return FALSE;
    }

    WNDCLASS wc;
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = CIEMainWindow::CIEMainWindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = DLLInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = WINDOW_CLASSNAME;
    if (RegisterClass(&wc) == 0)
    {
        TRACE_E("RegisterClass has failed");
        return FALSE;
    }

    WNDCLASS wcHost;
    wcHost.style = CS_DBLCLKS;
    wcHost.lpfnWndProc = WebView2HostWndProc;
    wcHost.cbClsExtra = 0;
    wcHost.cbWndExtra = 0;
    wcHost.hInstance = DLLInstance;
    wcHost.hIcon = NULL;
    wcHost.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcHost.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcHost.lpszMenuName = NULL;
    wcHost.lpszClassName = "SalamanderWebView2Host";
    if (RegisterClass(&wcHost) == 0)
    {
        TRACE_E("RegisterClass(SalamanderWebView2Host) has failed");
        UnregisterClass(WINDOW_CLASSNAME, DLLInstance);
        return FALSE;
    }

    return TRUE;
}

void ReleaseViewer()
{
    CALL_STACK_MESSAGE1("ReleaseViewer()");
    if (AtomObject != 0)
        GlobalDeleteAtom(AtomObject);
    if (!UnregisterClass(WINDOW_CLASSNAME, DLLInstance))
        TRACE_E("UnregisterClass(WINDOW_CLASSNAME) has failed");
    if (!UnregisterClass("SalamanderWebView2Host", DLLInstance))
        TRACE_E("UnregisterClass(SalamanderWebView2Host) has failed");
}

//***********************************************************************************
//
// CImpIOleClientSite
//

CImpIOleClientSite::CImpIOleClientSite(CSite* pSite, IUnknown* pUnkOuter)
{
    TRACE_I("CImpIOleClientSite::CImpIOleClientSite");
    m_cRef = 0;
    m_pSite = pSite;
    m_pUnkOuter = pUnkOuter;
}

CImpIOleClientSite::~CImpIOleClientSite()
{
    TRACE_I("CImpIOleClientSite::~CImpIOleClientSite");
    if (m_cRef != 0)
        TRACE_E("CImpIOleClientSite::~CImpIOleClientSite m_cRef == " << m_cRef);
}

//
// IUnknown methods
//

STDMETHODIMP CImpIOleClientSite::QueryInterface(REFIID riid, void** ppvObj)
{
    SLOW_CALL_STACK_MESSAGE1("CImpIOleClientSite::QueryInterface(, )");
    //  TRACE_I("CImpIOleClientSite::QueryInterface Interface "<<GetStringFromIID(riid));
    return m_pUnkOuter->QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG)
CImpIOleClientSite::AddRef()
{
    CALL_STACK_MESSAGE1("CImpIOleClientSite::AddRef()");
    //  TRACE_I("CImpIOleClientSite::AddRef");
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG)
CImpIOleClientSite::Release()
{
    CALL_STACK_MESSAGE1("CImpIOleClientSite::Release()");
    //  TRACE_I("CImpIOleClientSite::Release");
    if (m_cRef == 0)
        TRACE_E("CImpIOleClientSite::Release m_cRef == 0");
    --m_cRef;
    return m_pUnkOuter->Release();
}

//
// IOleClientSite methods
//

STDMETHODIMP CImpIOleClientSite::SaveObject()
{
    TRACE_I("CImpIOleClientSite::SaveObject");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleClientSite::GetMoniker(DWORD dwAssign,
                                            DWORD dwWhichMoniker,
                                            LPMONIKER FAR* ppmk)
{
    TRACE_I("CImpIOleClientSite::GetMoniker");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleClientSite::GetContainer(LPOLECONTAINER* ppContainer)
{
    TRACE_I("CImpIOleClientSite::GetContainer");
    *ppContainer = NULL;
    return E_NOINTERFACE;
}

STDMETHODIMP CImpIOleClientSite::ShowObject()
{
    TRACE_I("CImpIOleClientSite::ShowObject");
    return S_OK;
}

STDMETHODIMP CImpIOleClientSite::OnShowWindow(BOOL fShow)
{
    TRACE_I("CImpIOleClientSite::OnShowWindow");
    return NOERROR;
}

STDMETHODIMP CImpIOleClientSite::RequestNewObjectLayout()
{
    TRACE_I("CImpIOleClientSite::RequestNewObjectLayout");
    return E_NOTIMPL;
}

//***********************************************************************************
//
// CImpIOleInPlaceSite
//

CImpIOleInPlaceSite::CImpIOleInPlaceSite(CSite* pSite, IUnknown* pUnkOuter)
{
    TRACE_I("CImpIOleInPlaceSite::CImpIOleInPlaceSite");
    m_cRef = 0;
    m_pSite = pSite;
    m_pUnkOuter = pUnkOuter;
}

CImpIOleInPlaceSite::~CImpIOleInPlaceSite()
{
    TRACE_I("CImpIOleInPlaceSite::~CImpIOleInPlaceSite");
    if (m_cRef != 0)
        TRACE_E("CImpIOleInPlaceSite::~CImpIOleInPlaceSite m_cRef == " << m_cRef);
}

//
// IUnknown methods
//

STDMETHODIMP CImpIOleInPlaceSite::QueryInterface(REFIID riid, void** ppvObj)
{
    CALL_STACK_MESSAGE1("CImpIOleInPlaceSite::QueryInterface(, )");
    //  TRACE_I("CImpIOleInPlaceSite::QueryInterface Interface "<<GetStringFromIID(riid));
    return m_pUnkOuter->QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG)
CImpIOleInPlaceSite::AddRef()
{
    CALL_STACK_MESSAGE1("CImpIOleInPlaceSite::AddRef()");
    //  TRACE_I("CImpIOleInPlaceSite::AddRef");
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG)
CImpIOleInPlaceSite::Release()
{
    CALL_STACK_MESSAGE1("CImpIOleInPlaceSite::Release()");
    //  TRACE_I("CImpIOleInPlaceSite::Release");
    if (m_cRef == 0)
        TRACE_E("CImpIOleInPlaceSite::Release m_cRef == 0");
    --m_cRef;
    return m_pUnkOuter->Release();
}

//
// IOleWindow methods
//

STDMETHODIMP CImpIOleInPlaceSite::GetWindow(HWND* lphwnd)
{
    TRACE_I("CImpIOleInPlaceSite::GetWindow");
    *lphwnd = m_pSite->m_hParentWnd;
    return S_OK;
}

STDMETHODIMP CImpIOleInPlaceSite::ContextSensitiveHelp(BOOL fEnterMode)
{
    TRACE_I("CImpIOleInPlaceSite::ContextSensitiveHelp");
    return E_NOTIMPL;
}

//
// IOleInPlaceSite methods
//

STDMETHODIMP CImpIOleInPlaceSite::CanInPlaceActivate()
{
    TRACE_I("CImpIOleInPlaceSite::CanInPlaceActivate");
    return S_OK;
}

STDMETHODIMP CImpIOleInPlaceSite::OnInPlaceActivate()
{
    TRACE_I("CImpIOleInPlaceSite::OnInPlaceActivate");
    return S_OK;
}

STDMETHODIMP CImpIOleInPlaceSite::OnUIActivate()
{
    TRACE_I("CImpIOleInPlaceSite::OnUIActivate");
    return S_OK;
}

STDMETHODIMP CImpIOleInPlaceSite::GetWindowContext(LPOLEINPLACEFRAME* lplpFrame,
                                                   LPOLEINPLACEUIWINDOW* lplpDoc,
                                                   LPRECT lprcPosRect,
                                                   LPRECT lprcClipRect,
                                                   LPOLEINPLACEFRAMEINFO lpFrameInfo)
{
    CALL_STACK_MESSAGE1("CImpIOleInPlaceSite::GetWindowContext(, , , , )");
    TRACE_I("CImpIOleInPlaceSite::GetWindowContext");
    *lplpFrame = m_pSite->m_pImpIOleInPlaceFrame;
    (*lplpFrame)->AddRef();
    *lplpDoc = NULL;
    *lprcPosRect = m_pSite->m_rect;
    *lprcClipRect = m_pSite->m_rect;

    lpFrameInfo->cb = sizeof(OLEINPLACEFRAMEINFO);
    lpFrameInfo->fMDIApp = FALSE;
    lpFrameInfo->hwndFrame = m_pSite->m_hParentWnd;
    lpFrameInfo->haccel = NULL;
    lpFrameInfo->cAccelEntries = 0;
    return NOERROR;
}

STDMETHODIMP CImpIOleInPlaceSite::Scroll(SIZE scrollExtent)
{
    TRACE_I("CImpIOleInPlaceSite::Scroll");
    return S_OK;
}

STDMETHODIMP CImpIOleInPlaceSite::OnUIDeactivate(BOOL fUndoable)
{
    TRACE_I("CImpIOleInPlaceSite::OnUIDeactivate");
    return S_OK;
}

STDMETHODIMP CImpIOleInPlaceSite::OnInPlaceDeactivate()
{
    TRACE_I("CImpIOleInPlaceSite::OnInPlaceDeactivate");
    return S_OK;
}

STDMETHODIMP CImpIOleInPlaceSite::DiscardUndoState()
{
    TRACE_I("CImpIOleInPlaceSite::DiscardUndoState");
    return S_OK;
}

STDMETHODIMP CImpIOleInPlaceSite::DeactivateAndUndo()
{
    TRACE_I("CImpIOleInPlaceSite::DeactivateAndUndo");
    return S_OK;
}

STDMETHODIMP CImpIOleInPlaceSite::OnPosRectChange(LPCRECT lprcPosRect)
{
    TRACE_I("CImpIOleInPlaceSite::OnPosRectChange");
    return S_OK;
}

//***********************************************************************************
//
// CImpIOleInPlaceFrame
//

CImpIOleInPlaceFrame::CImpIOleInPlaceFrame(CSite* pSite, IUnknown* pUnkOuter)
{
    TRACE_I("CImpIOleInPlaceFrame::CImpIOleInPlaceFrame");
    m_cRef = 0;
    m_pSite = pSite;
    m_pUnkOuter = pUnkOuter;
}

CImpIOleInPlaceFrame::~CImpIOleInPlaceFrame()
{
    CALL_STACK_MESSAGE1("CImpIOleInPlaceFrame::~CImpIOleInPlaceFrame()");
    TRACE_I("CImpIOleInPlaceFrame::~CImpIOleInPlaceFrame");
    if (m_cRef != 0)
        TRACE_E("CImpIOleInPlaceFrame::~CImpIOleInPlaceFrame m_cRef == " << m_cRef);
}

//
// IUnknown methods
//

STDMETHODIMP CImpIOleInPlaceFrame::QueryInterface(REFIID riid, void** ppvObj)
{
    CALL_STACK_MESSAGE1("CImpIOleInPlaceFrame::QueryInterface(, )");
    //  TRACE_I("CImpIOleInPlaceFrame::QueryInterface Interface "<<GetStringFromIID(riid));
    return m_pUnkOuter->QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG)
CImpIOleInPlaceFrame::AddRef()
{
    CALL_STACK_MESSAGE1("CImpIOleInPlaceFrame::AddRef()");
    //  TRACE_I("CImpIOleInPlaceFrame::AddRef");
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG)
CImpIOleInPlaceFrame::Release()
{
    CALL_STACK_MESSAGE1("CImpIOleInPlaceFrame::Release()");
    //  TRACE_I("CImpIOleInPlaceFrame::Release");
    if (m_cRef == 0)
        TRACE_E("CImpIOleInPlaceFrame::Release m_cRef == 0");
    --m_cRef;
    return m_pUnkOuter->Release();
}

//
// IOleWindow methods
//

STDMETHODIMP CImpIOleInPlaceFrame::GetWindow(HWND* lphwnd)
{
    *lphwnd = m_pSite->m_hParentWnd;
    return *lphwnd != NULL ? S_OK : E_FAIL;
}

STDMETHODIMP CImpIOleInPlaceFrame::ContextSensitiveHelp(BOOL fEnterMode)
{
    TRACE_E("CImpIOleInPlaceFrame::ContextSensitiveHelp");
    return E_NOTIMPL;
}

//
// IOleInPlaceUIWindow methods
//

STDMETHODIMP CImpIOleInPlaceFrame::GetBorder(LPRECT lprectBorder)
{
    TRACE_I("CImpIOleInPlaceFrame::GetBorder");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleInPlaceFrame::RequestBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    TRACE_I("CImpIOleInPlaceFrame::RequestBorderSpace");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleInPlaceFrame::SetBorderSpace(LPCBORDERWIDTHS lpborderwidths)
{
    TRACE_I("CImpIOleInPlaceFrame::SetBorderSpace");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleInPlaceFrame::SetActiveObject(LPOLEINPLACEACTIVEOBJECT lpActiveObject,
                                                   LPCOLESTR lpszObjName)
{
    TRACE_I("CImpIOleInPlaceFrame::SetActiveObject");
    return E_NOTIMPL;
}

//
// IOleInPlaceFrame methods
//

STDMETHODIMP CImpIOleInPlaceFrame::InsertMenus(HMENU hmenuShared,
                                               LPOLEMENUGROUPWIDTHS lpMenuWidths)
{
    TRACE_I("CImpIOleInPlaceFrame::InsertMenus");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleInPlaceFrame::SetMenu(HMENU hmenuShared,
                                           HOLEMENU holemenu,
                                           HWND hwndActiveObject)
{
    TRACE_I("CImpIOleInPlaceFrame::SetMenu");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleInPlaceFrame::RemoveMenus(HMENU hmenuShared)
{
    TRACE_I("CImpIOleInPlaceFrame::RemoveMenus");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleInPlaceFrame::SetStatusText(LPCOLESTR lpszStatusText)
{
    CALL_STACK_MESSAGE1("CImpIOleInPlaceFrame::SetStatusText()");
    char buff[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, lpszStatusText, -1, buff,
                        MAX_PATH, NULL, NULL);
    buff[MAX_PATH - 1] = 0;
    TRACE_I("CImpIOleInPlaceFrame::SetStatusText: " << buff);
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleInPlaceFrame::EnableModeless(BOOL fEnable)
{
    TRACE_I("CImpIOleInPlaceSite::EnableModeless fEnable = " << fEnable);
    return S_OK;
}

STDMETHODIMP CImpIOleInPlaceFrame::TranslateAccelerator(LPMSG lpmsg,
                                                        WORD wID)
{
    TRACE_I("CImpIOleInPlaceFrame::TranslateAccelerator");
    return S_FALSE; // The keystroke was not used.
}

//***********************************************************************************
//
// CImpIOleControlSite
//

CImpIOleControlSite::CImpIOleControlSite(CSite* pSite, IUnknown* pUnkOuter)
{
    TRACE_I("CImpIOleControlSite::CImpIOleControlSite");
    m_cRef = 0;
    m_pSite = pSite;
    m_pUnkOuter = pUnkOuter;
}

CImpIOleControlSite::~CImpIOleControlSite()
{
    TRACE_I("CImpIOleControlSite::~CImpIOleControlSite");
    if (m_cRef != 0)
        TRACE_E("CImpIOleControlSite::~CImpIOleControlSite m_cRef == " << m_cRef);
}

// IUnknown methods
//
STDMETHODIMP CImpIOleControlSite::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    CALL_STACK_MESSAGE1("CImpIOleControlSite::QueryInterface(, )");
    //  TRACE_I("CImpIOleControlSite::QueryInterface Interface "<<GetStringFromIID(riid));
    return m_pUnkOuter->QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG)
CImpIOleControlSite::AddRef()
{
    CALL_STACK_MESSAGE1("CImpIOleControlSite::AddRef()");
    //  TRACE_I("CImpIOleControlSite::AddRef");
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG)
CImpIOleControlSite::Release()
{
    CALL_STACK_MESSAGE1("CImpIOleControlSite::Release()");
    //  TRACE_I("CImpIOleControlSite::Release");
    if (m_cRef == 0)
        TRACE_E("CImpIOleControlSite::Release m_cRef == 0");
    --m_cRef;
    return m_pUnkOuter->Release();
}

// CImpIOleControlSite methods
//
STDMETHODIMP CImpIOleControlSite::OnControlInfoChanged()
{
    TRACE_I("CImpIOleControlSite::OnControlInfoChanged");
    return NOERROR;
}

STDMETHODIMP CImpIOleControlSite::LockInPlaceActive(BOOL fLock)
{
    TRACE_I("CImpIOleControlSite::LockInPlaceActive");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleControlSite::GetExtendedControl(LPDISPATCH* ppDisp)
{
    TRACE_I("CImpIOleControlSite::GetExtendedControl");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleControlSite::TransformCoords(POINTL* lpptlHimetric,
                                                  POINTF* lpptfContainer,
                                                  DWORD flags)
{
    TRACE_I("CImpIOleControlSite::TransformCoords");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleControlSite::TranslateAccelerator(LPMSG lpMsg,
                                                       DWORD grfModifiers)
{
    return E_NOTIMPL;
}

STDMETHODIMP CImpIOleControlSite::OnFocus(BOOL fGotFocus)
{
    TRACE_I("CImpIOleControlSite::OnFocus fGotFocus = " << fGotFocus);
    return S_OK;
}

STDMETHODIMP CImpIOleControlSite::ShowPropertyFrame()
{
    TRACE_I("CImpIOleControlSite::ShowPropertyFrame");
    return E_NOTIMPL;
}

//***********************************************************************************
//
// CImpIAdviseSink
//
/*
CImpIAdviseSink::CImpIAdviseSink(CSite *pSite, IUnknown *pUnkOuter)
{
  TRACE_I("CImpIAdviseSink::CImpIAdviseSink");
  m_cRef = 0;
  m_pSite = pSite;
  m_pUnkOuter = pUnkOuter;
}

CImpIAdviseSink::~CImpIAdviseSink()
{
  TRACE_I("CImpIAdviseSink::~CImpIAdviseSink");
  if (m_cRef != 0) TRACE_E("CImpIAdviseSink::~CImpIAdviseSink m_cRef == "<<m_cRef);
}

// IUnknown methods
//
STDMETHODIMP CImpIAdviseSink::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
  CALL_STACK_MESSAGE1("CImpIAdviseSink::QueryInterface(, )");
//  TRACE_I("CImpIAdviseSink::QueryInterface Interface "<<GetStringFromIID(riid));
  return m_pUnkOuter->QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG) CImpIAdviseSink::AddRef()
{
  CALL_STACK_MESSAGE1("CImpIAdviseSink::AddRef()");
//  TRACE_I("CImpIAdviseSink::AddRef");
  ++m_cRef;
  return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CImpIAdviseSink::Release()
{
  CALL_STACK_MESSAGE1("CImpIAdviseSink::Release()");
//  TRACE_I("CImpIAdviseSink::Release");
  if (m_cRef == 0) TRACE_E("CImpIAdviseSink::Release m_cRef == 0");
  --m_cRef;
  return m_pUnkOuter->Release();
}

// IAdviseSink methods
//
STDMETHODIMP_(void) CImpIAdviseSink::OnDataChange(FORMATETC FAR* pFormatetc, STGMEDIUM FAR* pStgmed)
{
  TRACE_I("CImpIAdviseSink::OnDataChange");
}

STDMETHODIMP_(void) CImpIAdviseSink::OnViewChange(DWORD dwAspect, LONG lindex)
{
  CALL_STACK_MESSAGE3("CImpIAdviseSink::OnViewChange(0x%X, %d)", dwAspect,
                      lindex);
  TRACE_I("CImpIAdviseSink::OnViewChange");
  BSTR pbstrLocationURL;
  if (m_pSite->m_pIWebBrowser != NULL &&
      m_pSite->m_pIWebBrowser->get_LocationURL(&pbstrLocationURL) == S_OK)
  {
    char locationURL[1024];
    char locationName[1024];
    locationURL[0] = 0;
    locationName[0] = 0;

    WideCharToMultiByte(CP_ACP, 0, pbstrLocationURL, -1, locationURL, 1024, NULL, NULL);
    locationURL[1024 - 1] = 0;

    BSTR pbstrLocationName;
    if (m_pSite->m_pIWebBrowser2 != NULL &&
        m_pSite->m_pIWebBrowser2->get_LocationName(&pbstrLocationName) == S_OK)
    {
      WideCharToMultiByte(CP_ACP, 0, pbstrLocationName, -1, locationName, 1024, NULL, NULL);
      locationName[1024 - 1] = 0;
      SysFreeString(pbstrLocationName);
    }

    char location[1024];
    BOOL file = strncmp(locationURL, "file:", 5) == 0;
    if (file || locationName[0] == 0)
      lstrcpy(location, locationURL);
    else
      lstrcpy(location, locationName);

    char title[MAX_PATH + 200];
    title[0] = 0;
    if (location[0] != 0)
      sprintf(title, "%s - ", location);
    lstrcat(title, LoadStr(IDS_PLUGINNAME));
//    SetWindowText(m_pSite->m_hParentWnd, title);

    if (m_pSite->m_pIWebBrowser2 != NULL)
      m_pSite->m_pIWebBrowser2->put_StatusBar(TRUE);
    SysFreeString(pbstrLocationURL);
  }
  m_pSite->DoVerb(OLEIVERB_UIACTIVATE);
}

STDMETHODIMP_(void) CImpIAdviseSink::OnRename(LPMONIKER pmk)
{
  TRACE_I("CImpIAdviseSink::OnRename");
}

STDMETHODIMP_(void) CImpIAdviseSink::OnSave()
{
  TRACE_I("CImpIAdviseSink::OnSave");
}

STDMETHODIMP_(void) CImpIAdviseSink::OnClose()
{
  TRACE_I("CImpIAdviseSink::OnClose");
}
*/
//***********************************************************************************
//
// CImpIDispatch
//

CImpIDispatch::CImpIDispatch(CSite* pSite, IUnknown* pUnkOuter)
{
    TRACE_I("CImpIDispatch::CImpIDispatch");
    m_cRef = 0;
    m_pSite = pSite;
    m_pUnkOuter = pUnkOuter;
}

CImpIDispatch::~CImpIDispatch()
{
    TRACE_I("CImpIDispatch::~CImpIDispatch");
    if (m_cRef != 0)
        TRACE_E("CImpIDispatch::~CImpIDispatch m_cRef == " << m_cRef);
}

// IUnknown methods
//
STDMETHODIMP CImpIDispatch::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    CALL_STACK_MESSAGE1("CImpIDispatch::QueryInterface(, )");
    //  TRACE_I("CImpIDispatch::QueryInterface Interface "<<GetStringFromIID(riid));
    return m_pUnkOuter->QueryInterface(riid, ppvObj);
}

STDMETHODIMP_(ULONG)
CImpIDispatch::AddRef()
{
    CALL_STACK_MESSAGE1("CImpIDispatch::AddRef()");
    //  TRACE_I("CImpIDispatch::AddRef");
    ++m_cRef;
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG)
CImpIDispatch::Release()
{
    CALL_STACK_MESSAGE1("CImpIDispatch::Release()");
    //  TRACE_I("CImpIDispatch::Release");
    if (m_cRef == 0)
        TRACE_E("CImpIDispatch::Release m_cRef == 0");
    --m_cRef;
    return m_pUnkOuter->Release();
}

// CImpIDispatch methods
//
STDMETHODIMP CImpIDispatch::GetTypeInfoCount(unsigned int* pctinfo)
{
    TRACE_I("CImpIDispatch::GetTypeInfoCount");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIDispatch::GetTypeInfo(unsigned int itinfo, LCID lcid, ITypeInfo** pptinfo)
{
    TRACE_I("CImpIDispatch::GetTypeInfo");
    return E_NOTIMPL;
}

STDMETHODIMP CImpIDispatch::GetIDsOfNames(REFIID riid,
                                          OLECHAR** rgszNames,
                                          unsigned int cNames,
                                          LCID lcid,
                                          DISPID* rgdispid)
{
    TRACE_I("CImpIDispatch::GetIDsOfNames");
    return E_NOTIMPL;
}

BOOL CImpIDispatch::CanonizeURL(char* psz)
{
    LPTSTR pszSource = psz;
    LPTSTR pszDest = psz;

    BOOL file = _strnicmp(psz, "file://", 7) == 0;

    static const char szHex[] = ("0123456789ABCDEF");

    // unescape special characters

    while (*pszSource != '\0')
    {
        //    if (*pszSource == '+')   // j.r. it crashed when displaying a file in a directory with '+' in its name
        //      *pszDest++ = ' ';      // no idea why the '+' removal was originally there (inherited code)
        //    else
        if (*pszSource == '%')
        {
            TCHAR nValue = '?';
            LPCTSTR pszLow;
            LPCTSTR pszHigh;
            pszSource++;

            *pszSource = toupper(*pszSource);

            pszHigh = strchr(szHex, *pszSource);

            if (pszHigh != NULL)
            {
                pszSource++;
                *pszSource = toupper(*pszSource);
                pszLow = strchr(szHex, *pszSource);
                if (pszLow != NULL)
                {
                    nValue = (TCHAR)(((pszHigh - szHex) << 4) + (pszLow - szHex));
                }
            }

            *pszDest = nValue;
        }
        else
            *pszDest = *pszSource;
        if (file && *pszDest == '/')
            *pszDest = '\\';
        pszDest++;
        pszSource++;
    }
    *pszDest = '\0';
    if (file)
    {
        int offset = 7;
        if (m_pSite->m_pIWebBrowser2 != NULL)
        {
            // > IE 3.02 - newer IE versions write "file:\\\C:\" or "file:\\john\c"
            if (psz[7] == '\\' && __isascii(psz[8]))
                offset++;
            else
                offset -= 2;
        }
        memmove(psz, psz + offset, lstrlen(psz) - offset + 1);
    }
    return file;
}

STDMETHODIMP CImpIDispatch::Invoke(DISPID dispID,
                                   REFIID riid,
                                   LCID lcid,
                                   unsigned short wFlags,
                                   DISPPARAMS* pDispParams,
                                   VARIANT* pVarResult,
                                   EXCEPINFO* pExcepInfo,
                                   unsigned int* puArgErr)
{
    TRACE_I("CImpIDispatch::Invoke dispID=" << dispID);
    if (!pDispParams)
        return E_INVALIDARG;

    switch (dispID)
    {
    case DISPID_DOWNLOADCOMPLETE:
    {
        if (!m_pSite->m_fOpening)
            m_pSite->m_fCanClose = TRUE;
        break;
    }

    case DISPID_TITLECHANGE:
    {
        if (pDispParams->rgvarg[0].vt == VT_BSTR)
        {
            char title[1024];
            title[0] = 0;

            WideCharToMultiByte(CP_ACP, 0, pDispParams->rgvarg[0].bstrVal, -1, title, 1024, NULL, NULL);
            title[1024 - 1] = 0;

            char locationURL[1024];
            locationURL[0] = 0;
            BSTR pbstrLocationURL;
            if (m_pSite->m_pIWebBrowser != NULL &&
                m_pSite->m_pIWebBrowser->get_LocationURL(&pbstrLocationURL) == S_OK)
            {
                WideCharToMultiByte(CP_ACP, 0, pbstrLocationURL, -1, locationURL, 1024, NULL, NULL);
                locationURL[1024 - 1] = 0;

                BOOL file = CanonizeURL(locationURL);

                char buff[3000];
                if (m_pSite->MarkdownFilename[0] != 0)
                    lstrcpy(buff, m_pSite->MarkdownFilename);
                else
                {
                    lstrcpy(buff, locationURL);
                    if (!file || lstrcmp(locationURL, title) != 0)
                        sprintf(buff + lstrlen(buff), " (%s)", title);
                }

                sprintf(buff + lstrlen(buff), " - %s", LoadStr(IDS_PLUGINNAME));

                SetWindowText(m_pSite->m_hParentWnd, buff);
            }
            m_pSite->DoVerb(OLEIVERB_UIACTIVATE);
        }
    }
    }

    return S_OK;
}

//***********************************************************************************
//
// CSite
//

CSite::CSite()
{
    TRACE_I("CSite::CSite");
    m_cRef = 0;
    m_fInitialized = FALSE;
    m_fCreated = FALSE;
    m_fCanClose = TRUE;
    m_fOpening = FALSE;
    m_hParentWnd = NULL;

    //Object interfaces
    m_pIUnknown = NULL;
    m_pIWebBrowser = NULL;
    m_pIWebBrowser2 = NULL; // !!! warning - for IE3.02 it can be NULL
    m_pIOleObject = NULL;
    m_pIOleInPlaceObject = NULL;
    m_pIOleInPlaceActiveObject = NULL;
    m_pIViewObject = NULL;
    m_pConnectionPoint = NULL;

    //Our interfaces
    m_pImpIOleClientSite = NULL;
    m_pImpIOleInPlaceSite = NULL;
    m_pImpIOleInPlaceFrame = NULL;
    m_pImpIOleControlSite = NULL;
    //  m_pImpIAdviseSink = NULL;
    m_pImpIDispatch = NULL;

    MarkdownFilename[0] = 0;
}

CSite::~CSite()
{
    TRACE_I("CSite::~CSite");
    if (m_cRef != 0)
        TRACE_E("CSite::~CSite m_cRef == " << m_cRef);
}

//
// IUnknown methods for delegation
//

STDMETHODIMP CSite::QueryInterface(REFIID riid, void** ppvObj)
{
    CALL_STACK_MESSAGE1("CSite::QueryInterface(, )");
    TRACE_I("CSite::QueryInterface Interface " << GetStringFromIID(riid));
    *ppvObj = NULL;

    if (riid == IID_IUnknown)
        *ppvObj = this;

    if (riid == IID_IOleClientSite)
        *ppvObj = m_pImpIOleClientSite;

    if (riid == IID_IOleWindow || riid == IID_IOleInPlaceSite)
        *ppvObj = m_pImpIOleInPlaceSite;

    if (riid == IID_IOleInPlaceFrame)
        *ppvObj = m_pImpIOleInPlaceFrame;

    if (riid == IID_IOleControlSite)
        *ppvObj = m_pImpIOleControlSite;

    //  if (riid == IID_IAdviseSink)
    //    *ppvObj = m_pImpIAdviseSink;

    if (riid == DIID_DWebBrowserEvents || riid == IID_IDispatch)
        *ppvObj = m_pImpIDispatch;

    if (*ppvObj != NULL)
    {
        ((LPUNKNOWN)*ppvObj)->AddRef();
        return NOERROR;
    }

    //  TRACE_I("CSite::QueryInterface Unknown Interface "<<GetStringFromIID(riid));

    return ResultFromScode(E_NOINTERFACE);
}

STDMETHODIMP_(ULONG)
CSite::AddRef()
{
    //  TRACE_I("CSite::AddRef");
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CSite::Release()
{
    CALL_STACK_MESSAGE1("CSite::Release()");
    //  TRACE_I("CSite::Release");
    if (m_cRef == 0)
        TRACE_E("CSite::Release m_cRef == 0");
    return --m_cRef;
}

//
// Our methods
//

BOOL CSite::ObjectInitialize()
{
    CALL_STACK_MESSAGE1("CSite::ObjectInitialize()");
    if (m_hParentWnd == NULL)
    {
        TRACE_E("CSite::ObjectInitialize Parent window must be created");
        return FALSE;
    }

    if (m_fInitialized)
    {
        TRACE_E("CSite::ObjectInitialize Object is alredy intialized");
        return FALSE;
    }

    m_pImpIOleClientSite = new CImpIOleClientSite(this, this);
    m_pImpIOleInPlaceSite = new CImpIOleInPlaceSite(this, this);
    m_pImpIOleInPlaceFrame = new CImpIOleInPlaceFrame(this, this);
    m_pImpIOleControlSite = new CImpIOleControlSite(this, this);
    //  m_pImpIAdviseSink = new CImpIAdviseSink(this, this);
    m_pImpIDispatch = new CImpIDispatch(this, this);

    if (m_pImpIOleClientSite == NULL || m_pImpIOleInPlaceSite == NULL ||
        m_pImpIOleInPlaceFrame == NULL || m_pImpIOleControlSite == NULL ||
        /*m_pImpIAdviseSink == NULL ||*/ m_pImpIDispatch == NULL)
    {
        TRACE_E("Low memory");
        return FALSE;
    }

    m_fInitialized = TRUE;
    return TRUE;
}

// this nasty thing works even with IE 3.02
static CLSID const my_CLSID_WebBrowser =
    {0xeab22ac3, 0x30c1, 0x11cf, {0xa7, 0xeb, 0x0, 0x0, 0xc0, 0x5b, 0xae, 0x0b}};

BOOL CSite::Create(HWND hParentWnd)
{
    CALL_STACK_MESSAGE1("CSite::Create()");
    if (m_hParentWnd != NULL)
        TRACE_E("CSite::Create m_hParentWnd != NULL");
    m_hParentWnd = hParentWnd;

    if (FAILED(OleInitialize(NULL)))
    {
        TRACE_E("OleInitialize failed");
        return FALSE;
    }

    if (!ObjectInitialize())
        return FALSE;

    // Let it create an instance of CLSID_WebBrowser and fetch IID_IUnknown
    HRESULT hr = CoCreateInstance(my_CLSID_WebBrowser, NULL,
                                  CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                                  IID_IUnknown, (void**)&m_pIUnknown);

    if (hr == REGDB_E_CLASSNOTREG) // correct variant
    {
        TRACE_E("A InternetExplorer class is not registered in the registration database");
        return FALSE;
    }

    if (FAILED(hr))
    {
        TRACE_E("CoCreateInstance error");
        return FALSE;
    }

    // I often need IID_IWebBrowser, so I obtain one
    hr = m_pIUnknown->QueryInterface(IID_IWebBrowser, (void**)&m_pIWebBrowser);
    if (FAILED(hr))
    {
        TRACE_E("WebBrowser->QueryInterface failed on interface IID_IWebBrowser");
        return FALSE;
    }

    // try whether IID_IWebBrowser2 is also available
    hr = m_pIUnknown->QueryInterface(IID_IWebBrowser2, (void**)&m_pIWebBrowser2);
    if (FAILED(hr))
    {
        TRACE_I("IID_IWebBrowser2 interface is not available");
    }
    /*
  VARIANT_BOOL offline;
  m_pIWebBrowser2->get_Offline(&offline);

  m_pIWebBrowser2->put_Offline(-1);

  m_pIWebBrowser2->get_Offline(&offline);
*/
    // I often need IOleObject, so I obtain one
    hr = m_pIUnknown->QueryInterface(IID_IOleObject, (void**)&m_pIOleObject);
    if (FAILED(hr))
    {
        TRACE_E("WebBrowser->QueryInterface failed on interface IID_IOleObject");
        return FALSE;
    }

    // I often need IOleInPlaceObject, so I obtain one
    hr = m_pIUnknown->QueryInterface(IID_IOleInPlaceObject, (void**)&m_pIOleInPlaceObject);
    if (FAILED(hr))
    {
        TRACE_E("WebBrowser->QueryInterface failed on interface IID_IOleInPlaceObject");
        return FALSE;
    }

    // I often need IID_IOleInPlaceActiveObject, so I obtain one
    hr = m_pIUnknown->QueryInterface(IID_IOleInPlaceActiveObject, (void**)&m_pIOleInPlaceActiveObject);
    if (FAILED(hr))
    {
        TRACE_E("WebBrowser->QueryInterface failed on interface IID_IOleInPlaceActiveObject");
        return FALSE;
    }

    // I often need IID_IViewObject, so I obtain one
    hr = m_pIUnknown->QueryInterface(IID_IViewObject, (void**)&m_pIViewObject);
    if (FAILED(hr))
    {
        TRACE_E("WebBrowser->QueryInterface failed on interface IID_IViewObject");
        return FALSE;
    }
    /*
  if (FAILED(hr = m_pIViewObject->SetAdvise(DVASPECT_CONTENT, ADVF_PRIMEFIRST,
                                            m_pImpIAdviseSink)))
  {
    TRACE_E("SetAdvise on WebBrowser failed.");
    return FALSE;
  }
*/
    // Set the object's ClientSite
    if (FAILED(hr = m_pIOleObject->SetClientSite(m_pImpIOleClientSite)))
    {
        TRACE_E("SetClientSite on WebBrowser failed.");
        return FALSE;
    }

    GetClientRect(m_hParentWnd, &m_rect);

    SIZE size;
    size.cx = m_rect.right - m_rect.left;
    size.cy = m_rect.bottom - m_rect.top;

    hr = m_pIOleObject->SetExtent(DVASPECT_CONTENT, (SIZEL*)&size);
    if (FAILED(hr))
        TRACE_E("m_pIOleObject->SetExtent failed");

    hr = m_pIOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE,
                               NULL, m_pImpIOleClientSite, 0,
                               m_hParentWnd, &m_rect);
    if (FAILED(hr))
        TRACE_E("m_pIOleObject->DoVerb failed");

    hr = m_pIOleInPlaceObject->SetObjectRects(&m_rect, &m_rect);
    if (FAILED(hr))
        TRACE_E("m_pIOleInPlaceObject->SetObjectRects failed");

    ConnectEvents();

    m_fCreated = TRUE;
    m_fOpening = TRUE;
    return TRUE;
}

void CSiteCloseAux()
{
    __try
    {
        OleUninitialize();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void CSite::Close()
{
    CALL_STACK_MESSAGE1("CSite::Close()");
    TRACE_I("CSite::Close()");
    HRESULT hr;

    m_fCreated = FALSE;

    // deactivate the InPlaceObject
    if (m_pIOleInPlaceObject != NULL)
    {
        hr = m_pIOleInPlaceObject->InPlaceDeactivate();
        if (FAILED(hr))
            TRACE_E("m_pIOleInPlaceObject->InPlaceDeactivate failed");
        // release the interface
        ReleaseInterface(m_pIOleInPlaceObject);
    }

    DisconnectEvents();
    /*
  if (m_pIViewObject != NULL)
  {
    if (FAILED(hr = m_pIViewObject->SetAdvise(DVASPECT_CONTENT, ADVF_PRIMEFIRST, NULL)))
      TRACE_E("SetAdvise on WebBrowser failed.");
  }
*/
    // close the OleObject
    if (m_pIOleObject != NULL)
    {
        hr = m_pIOleObject->SetClientSite(NULL);
        if (FAILED(hr))
            TRACE_E("m_pIOleObject->SetClientSite failed");
        hr = m_pIOleObject->Close(OLECLOSE_NOSAVE);
        if (FAILED(hr))
            TRACE_E("m_pIOleObject->Close failed");
        // release the interface
        ReleaseInterface(m_pIOleObject);
    }

    // release helper interfaces
    ReleaseInterface(m_pIUnknown);
    ReleaseInterface(m_pIWebBrowser);
    if (m_pIWebBrowser2 != NULL)
        ReleaseInterface(m_pIWebBrowser2);
    ReleaseInterface(m_pIOleInPlaceActiveObject);
    ReleaseInterface(m_pIViewObject);

    // release my own interfaces
    DeleteInterfaceImp(m_pImpIOleClientSite);
    DeleteInterfaceImp(m_pImpIOleInPlaceSite);
    DeleteInterfaceImp(m_pImpIOleInPlaceFrame);
    DeleteInterfaceImp(m_pImpIOleControlSite);
    //  DeleteInterfaceImp(m_pImpIAdviseSink);
    DeleteInterfaceImp(m_pImpIDispatch);

    CSiteCloseAux();
}

HRESULT CSite::DoVerb(LONG iVerb)
{
    CALL_STACK_MESSAGE2("CSite::DoVerb(%d)", iVerb);
    if (m_fCreated)
        return m_pIOleObject->DoVerb(OLEIVERB_UIACTIVATE, NULL,
                                     m_pImpIOleClientSite, 0,
                                     m_hParentWnd, &m_rect);
    return OLEOBJ_S_CANNOT_DOVERB_NOW;
}

void CSite::ConnectEvents()
{
    CALL_STACK_MESSAGE1("CSite::ConnectEvents()");

    IConnectionPointContainer* pCPContainer;
    // Step 1: Get a pointer to the connection point container
    HRESULT hr = m_pIWebBrowser->QueryInterface(IID_IConnectionPointContainer,
                                                (void**)&pCPContainer);
    if (SUCCEEDED(hr))
    {
        // m_pConnectionPoint is defined like this:
        // IConnectionPoint* m_pConnectionPoint;

        // Step 2: Find the connection point
        hr = pCPContainer->FindConnectionPoint(DIID_DWebBrowserEvents, &m_pConnectionPoint);
        if (SUCCEEDED(hr))
        {
            // Step 3: Advise
            hr = m_pConnectionPoint->Advise(this, &m_dwCookie);
            //       if (FAILED(hr))
            //       {
            //         ::MessageBox(NULL, "Failed to Advise", "C++ Event Sink", MB_OK);
            //       }
        }

        pCPContainer->Release();
    }
}

void CSite::DisconnectEvents()
{
    CALL_STACK_MESSAGE1("CSite::DisconnectEvents()");
    // Step 5: Unadvise
    if (m_pConnectionPoint)
    {
        HRESULT hr = m_pConnectionPoint->Unadvise(m_dwCookie);
        //    if (FAILED(hr))
        //    {
        //      ::MessageBox(NULL, "Failed to Unadvise", "C++ Event Sink", MB_OK);
        //    }
    }
}

//***********************************************************************************
//
// CIEWindow
//

std::string ReadStreamToString(IStream* pStream)
{
    if (!pStream) return "";
    LARGE_INTEGER liZero = {0};
    pStream->Seek(liZero, STREAM_SEEK_SET, NULL);
    
    std::string result;
    char buffer[4096];
    ULONG bytesRead;
    while (SUCCEEDED(pStream->Read(buffer, sizeof(buffer), &bytesRead)) && bytesRead > 0)
    {
        result.append(buffer, bytesRead);
    }
    return result;
}

// Window Procedure for the child WebView2 Host window
LRESULT CALLBACK WebView2HostWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CIEWindow* pThis = (CIEWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    switch (uMsg)
    {
    case WM_NCCREATE:
    {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
        break;
    }
    case WM_CREATE:
    {
        CIEWindow* w = (CIEWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        if (w)
        {
            w->HWindow = hwnd;
            w->InitWebView2();
        }
        return 0;
    }
    case WM_SIZE:
        if (pThis && pThis->m_pController)
        {
            RECT rect;
            GetClientRect(hwnd, &rect);
            pThis->m_pController->put_Bounds(rect);
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// WebView2 Completed Handlers
class CControllerCompletedHandler;

class CEnvironmentCompletedHandler : public ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler
{
private:
    LONG m_refCount;
    HWND m_hWnd;
public:
    CEnvironmentCompletedHandler(HWND hWnd) : m_refCount(1), m_hWnd(hWnd) {}

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj)
    {
        if (riid == IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler || riid == IID_IUnknown)
        {
            *ppvObj = this;
            AddRef();
            return S_OK;
        }
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_refCount); }
    STDMETHODIMP_(ULONG) Release()
    {
        LONG count = InterlockedDecrement(&m_refCount);
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }

    STDMETHODIMP Invoke(HRESULT result, ICoreWebView2Environment* env);
};

class CControllerCompletedHandler : public ICoreWebView2CreateCoreWebView2ControllerCompletedHandler
{
private:
    LONG m_refCount;
    HWND m_hWnd;
public:
    CControllerCompletedHandler(HWND hWnd) : m_refCount(1), m_hWnd(hWnd) {}

    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj)
    {
        if (riid == IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler || riid == IID_IUnknown)
        {
            *ppvObj = this;
            AddRef();
            return S_OK;
        }
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_refCount); }
    STDMETHODIMP_(ULONG) Release()
    {
        LONG count = InterlockedDecrement(&m_refCount);
        if (count == 0)
        {
            delete this;
            return 0;
        }
        return count;
    }

    STDMETHODIMP Invoke(HRESULT result, ICoreWebView2Controller* controller)
    {
        if (FAILED(result) || !controller)
        {
            char msg[256];
            sprintf_s(msg, "WebView2 Controller creation failed. HRESULT: 0x%08X", result);
            MessageBox(m_hWnd, msg, "WebView2 Error", MB_OK | MB_ICONERROR);
            TRACE_E("WebView2 Controller creation failed: " << result);
            return result;
        }
        CIEWindow* host = (CIEWindow*)GetWindowLongPtr(m_hWnd, GWLP_USERDATA);
        if (host)
        {
            host->OnControllerCreated(controller);
        }
        else
        {
            controller->Release();
        }
        return S_OK;
    }
};

STDMETHODIMP CEnvironmentCompletedHandler::Invoke(HRESULT result, ICoreWebView2Environment* env)
{
    if (FAILED(result) || !env)
    {
        char msg[256];
        sprintf_s(msg, "WebView2 Environment creation failed. HRESULT: 0x%08X", result);
        MessageBox(m_hWnd, msg, "WebView2 Error", MB_OK | MB_ICONERROR);
        TRACE_E("WebView2 Environment creation failed: " << result);
        return result;
    }
    HRESULT hr = env->CreateCoreWebView2Controller(m_hWnd, new CControllerCompletedHandler(m_hWnd));
    if (FAILED(hr))
    {
        char msg[256];
        sprintf_s(msg, "CreateCoreWebView2Controller failed. HRESULT: 0x%08X", hr);
        MessageBox(m_hWnd, msg, "WebView2 Error", MB_OK | MB_ICONERROR);
        TRACE_E("CreateCoreWebView2Controller failed: " << hr);
    }
    return S_OK;
}

// CIEWindow implementations
BOOL CIEWindow::CreateSite(HWND hParent)
{
    CALL_STACK_MESSAGE1("CIEWindow::CreateSite()");
    TRACE_I("CIEWindow::CreateSite()");
    
    // Initialize COM/OLE for this thread since WebView2 requires STA COM apartment
    OleInitialize(NULL);

    m_hParentWnd = hParent;
    
    HWindow = CreateWindow("SalamanderWebView2Host", "", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 
                           0, 0, 100, 100, hParent, NULL, DLLInstance, this);
    if (HWindow == NULL)
    {
        TRACE_E("CreateWindow(SalamanderWebView2Host) failed");
        OleUninitialize();
        return FALSE;
    }
    return TRUE;
}

void CIEWindow::CloseSite()
{
    CALL_STACK_MESSAGE1("CIEWindow::CloseSite()");
    TRACE_I("CIEWindow::CloseSite()");
    
    if (m_pWebView)
    {
        m_pWebView->remove_WebMessageReceived(m_webMessageReceivedToken);
        m_pWebView->Release();
        m_pWebView = NULL;
    }
    if (m_pController)
    {
        m_pController->remove_AcceleratorKeyPressed(m_acceleratorKeyPressedToken);
        m_pController->Close();
        m_pController->Release();
        m_pController = NULL;
    }
    if (HWindow)
    {
        DestroyWindow(HWindow);
        HWindow = NULL;
    }
    m_isInitialized = false;

    OleUninitialize();
}

void CIEWindow::InitWebView2()
{
    wchar_t tempPath[MAX_PATH];
    if (GetTempPathW(MAX_PATH, tempPath) > 0)
    {
        wcscat_s(tempPath, L"SalamanderWebView2");
    }
    else
    {
        wcscpy_s(tempPath, L".");
    }

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(
        nullptr,
        tempPath,
        nullptr,
        new CEnvironmentCompletedHandler(HWindow)
    );
    if (FAILED(hr))
    {
        char msg[256];
        sprintf_s(msg, "CreateCoreWebView2EnvironmentWithOptions failed. HRESULT: 0x%08X", hr);
        MessageBox(HWindow, msg, "WebView2 Error", MB_OK | MB_ICONERROR);
        TRACE_E("CreateCoreWebView2EnvironmentWithOptions failed: " << hr);
    }
}

void CIEWindow::OnControllerCreated(ICoreWebView2Controller* controller)
{
    m_pController = controller;
    m_pController->AddRef();

    HRESULT hr = m_pController->get_CoreWebView2(&m_pWebView);
    if (SUCCEEDED(hr) && m_pWebView)
    {
        m_pWebView->AddRef();

        m_pController->add_AcceleratorKeyPressed(
            Microsoft::WRL::Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
                [this](ICoreWebView2Controller* sender, ICoreWebView2AcceleratorKeyPressedEventArgs* args) -> HRESULT
                {
                    COREWEBVIEW2_KEY_EVENT_KIND keyEventKind;
                    args->get_KeyEventKind(&keyEventKind);
                    
                    if (keyEventKind == COREWEBVIEW2_KEY_EVENT_KIND_KEY_DOWN)
                    {
                        UINT virtualKey;
                        args->get_VirtualKey(&virtualKey);
                        
                        if (virtualKey == VK_ESCAPE)
                        {
                            PostMessage(m_hParentWnd, WM_CLOSE, 0, 0);
                            args->put_Handled(TRUE);
                        }
                        else if (virtualKey == 'P' && (GetKeyState(VK_CONTROL) & 0x8000) != 0)
                        {
                            PostMessage(m_hParentWnd, WM_COMMAND, MAKEWPARAM(1001, 0), 0);
                            args->put_Handled(TRUE);
                        }
                    }
                    return S_OK;
                }).Get(),
            &m_acceleratorKeyPressedToken
        );

        // Register WebMessageReceived handler
        m_pWebView->add_WebMessageReceived(
            Microsoft::WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
                [this](ICoreWebView2* sender, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT
                {
                    LPWSTR messageRaw = NULL;
                    args->TryGetWebMessageAsString(&messageRaw);
                    if (messageRaw)
                    {
                        if (wcscmp(messageRaw, L"Escape") == 0)
                        {
                            PostMessage(m_hParentWnd, WM_CLOSE, 0, 0);
                        }
                        else if (wcscmp(messageRaw, L"CtrlP") == 0)
                        {
                            PostMessage(m_hParentWnd, WM_COMMAND, MAKEWPARAM(1001, 0), 0);
                        }
                        CoTaskMemFree(messageRaw);
                    }
                    return S_OK;
                }).Get(),
            &m_webMessageReceivedToken
        );

        // Inject script to catch Esc and Ctrl+P keys inside the loaded page
        m_pWebView->AddScriptToExecuteOnDocumentCreated(
            L"window.addEventListener('keydown', (e) => {\n"
            L"  if (e.key === 'Escape') {\n"
            L"    window.chrome.webview.postMessage('Escape');\n"
            L"  }\n"
            L"  if (e.ctrlKey && (e.key === 'p' || e.key === 'P')) {\n"
            L"    e.preventDefault();\n"
            L"    window.chrome.webview.postMessage('CtrlP');\n"
            L"  }\n"
            L"});",
            nullptr
        );

        // Get DLL directory
        char dllPath[MAX_PATH];
        if (GetModuleFileName(DLLInstance, dllPath, MAX_PATH) > 0)
        {
            char* lastSlash = strrchr(dllPath, '\\');
            if (lastSlash) *lastSlash = '\0';
            
            int len = MultiByteToWideChar(CP_ACP, 0, dllPath, -1, NULL, 0);
            wchar_t* wDllPath = new wchar_t[len];
            MultiByteToWideChar(CP_ACP, 0, dllPath, -1, wDllPath, len);

            // Map virtual host name "salamander.local" to the plugins/ieviewer folder
            ICoreWebView2_3* webView3 = NULL;
            if (SUCCEEDED(m_pWebView->QueryInterface(IID_ICoreWebView2_3, (void**)&webView3)) && webView3)
            {
                HRESULT hrMap = webView3->SetVirtualHostNameToFolderMapping(
                    L"salamander.local",
                    wDllPath,
                    COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND_ALLOW
                );
                if (FAILED(hrMap))
                {
                    char msg[256];
                    sprintf_s(msg, "SetVirtualHostNameToFolderMapping failed: 0x%08X", hrMap);
                    MessageBox(HWindow, msg, "WebView2 Map Error", MB_OK);
                }
                webView3->Release();
            }
            delete[] wDllPath;
        }

        ICoreWebView2Settings* settings = NULL;
        if (SUCCEEDED(m_pWebView->get_Settings(&settings)) && settings)
        {
            settings->put_IsScriptEnabled(TRUE);
            settings->put_AreDefaultContextMenusEnabled(TRUE);
            settings->put_IsStatusBarEnabled(FALSE);
            settings->Release();
        }

        RECT rect;
        GetClientRect(HWindow, &rect);
        m_pController->put_Bounds(rect);
        m_pController->put_IsVisible(TRUE);

        m_isInitialized = true;

        if (!m_pendingHtml.empty())
        {
            NavigateToString(m_pendingHtml);
            m_pendingHtml.clear();
        }
        else if (!m_pendingUrl.empty())
        {
            NavigateToUrl(m_pendingUrl);
            m_pendingUrl.clear();
        }
    }
}

void CIEWindow::NavigateToString(const std::string& html)
{
    if (m_pWebView)
    {
        int len = MultiByteToWideChar(CP_UTF8, 0, html.c_str(), -1, NULL, 0);
        wchar_t* wHtml = new wchar_t[len];
        MultiByteToWideChar(CP_UTF8, 0, html.c_str(), -1, wHtml, len);
        
        HRESULT hr = m_pWebView->NavigateToString(wHtml);
        if (FAILED(hr))
        {
            char msg[256];
            sprintf_s(msg, "NavigateToString failed. HRESULT: 0x%08X", hr);
            MessageBox(HWindow, msg, "WebView2 Error", MB_OK | MB_ICONERROR);
        }

        delete[] wHtml;
    }
}

void CIEWindow::NavigateToUrl(const std::wstring& url)
{
    if (m_pWebView)
    {
        m_pWebView->Navigate(url.c_str());
    }
}

void CIEWindow::Navigate(LPCTSTR lpszURL, IStream* contentStream)
{
    CALL_STACK_MESSAGE2("CIEWindow::Navigate(%s)", lpszURL);
    if (contentStream != NULL)
    {
        std::string html = ReadStreamToString(contentStream);
        contentStream->Release();
        
        if (m_isInitialized)
        {
            NavigateToString(html);
        }
        else
        {
            m_pendingHtml = html;
            m_pendingUrl.clear();
        }
    }
    else
    {
        int len = MultiByteToWideChar(CP_ACP, 0, lpszURL, -1, NULL, 0);
        wchar_t* wUrl = new wchar_t[len];
        MultiByteToWideChar(CP_ACP, 0, lpszURL, -1, wUrl, len);

        if (m_isInitialized)
        {
            NavigateToUrl(wUrl);
        }
        else
        {
            m_pendingUrl = wUrl;
            m_pendingHtml.clear();
        }
        delete[] wUrl;
    }
}

BOOL CIEWindow::CanClose()
{
    return TRUE;
}

HRESULT CIEWindow::TranslateAccelerator(LPMSG lpmsg)
{
    CALL_STACK_MESSAGE1("CIEWindow::TranslateAccelerator()");
    if (lpmsg->message == WM_KEYDOWN)
    {
        if (lpmsg->wParam == VK_ESCAPE)
        {
            TRACE_I("Posting WM_CLOSE");
            PostMessage(m_hParentWnd, WM_CLOSE, 0, 0);
            return S_OK;
        }
        else if (lpmsg->wParam == 'P' && (GetKeyState(VK_CONTROL) & 0x8000) != 0)
        {
            PostMessage(m_hParentWnd, WM_COMMAND, MAKEWPARAM(1001, 0), 0);
            return S_OK;
        }
    }
    return S_FALSE;
}

void CIEWindow::ExportToPdf()
{
    if (!m_isInitialized || !m_pWebView)
    {
        MessageBox(HWindow, "Viewer not fully initialized.", "Export to PDF", MB_OK | MB_ICONWARNING);
        return;
    }

    ICoreWebView2_7* webView7 = NULL;
    HRESULT hr = m_pWebView->QueryInterface(IID_ICoreWebView2_7, (void**)&webView7);
    if (FAILED(hr) || !webView7)
    {
        MessageBox(HWindow, "PDF export is not supported by your current WebView2 runtime version.", "Export to PDF", MB_OK | MB_ICONERROR);
        return;
    }

    OPENFILENAME ofn;
    char szFile[MAX_PATH] = "document.pdf";
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = HWindow;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "PDF Files (*.pdf)\0*.pdf\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn) == TRUE)
    {
        int len = MultiByteToWideChar(CP_ACP, 0, szFile, -1, NULL, 0);
        wchar_t* wPath = new wchar_t[len];
        MultiByteToWideChar(CP_ACP, 0, szFile, -1, wPath, len);

        class CPrintToPdfCompletedHandler : public ICoreWebView2PrintToPdfCompletedHandler
        {
        private:
            LONG m_refCount;
            HWND m_hWnd;
            wchar_t* m_path;
        public:
            CPrintToPdfCompletedHandler(HWND hWnd, const wchar_t* path) : m_refCount(1), m_hWnd(hWnd)
            {
                m_path = _wcsdup(path);
            }
            ~CPrintToPdfCompletedHandler()
            {
                free(m_path);
            }

            STDMETHODIMP QueryInterface(REFIID riid, void** ppvObj)
            {
                if (riid == IID_ICoreWebView2PrintToPdfCompletedHandler || riid == IID_IUnknown)
                {
                    *ppvObj = this;
                    AddRef();
                    return S_OK;
                }
                *ppvObj = NULL;
                return E_NOINTERFACE;
            }
            STDMETHODIMP_(ULONG) AddRef() { return InterlockedIncrement(&m_refCount); }
            STDMETHODIMP_(ULONG) Release()
            {
                LONG count = InterlockedDecrement(&m_refCount);
                if (count == 0)
                {
                    delete this;
                    return 0;
                }
                return count;
            }

            STDMETHODIMP Invoke(HRESULT errorCode, BOOL isSuccessful)
            {
                if (isSuccessful && SUCCEEDED(errorCode))
                {
                    MessageBoxW(m_hWnd, L"PDF exported successfully!", L"Export to PDF", MB_OK | MB_ICONINFORMATION);
                }
                else
                {
                    wchar_t msg[256];
                    swprintf_s(msg, L"Failed to export PDF. Error code: 0x%08X", errorCode);
                    MessageBoxW(m_hWnd, msg, L"Export to PDF", MB_OK | MB_ICONERROR);
                }
                return S_OK;
            }
        };

        hr = webView7->PrintToPdf(wPath, nullptr, new CPrintToPdfCompletedHandler(HWindow, wPath));
        if (FAILED(hr))
        {
            wchar_t msg[256];
            swprintf_s(msg, L"Failed to initiate PDF export. Error code: 0x%08X", hr);
            MessageBoxW(HWindow, msg, L"Export to PDF", MB_OK | MB_ICONERROR);
        }

        delete[] wPath;
    }
    webView7->Release();
}

//
// ****************************************************************************
// CIEMainWindowQueue
//

CIEMainWindowQueue::~CIEMainWindowQueue()
{
    if (!Empty())
        TRACE_E("A viewer window remained open!");
    // no multithreading risk here (the plugin is ending, threads are/were terminated)
    // free at least some memory
    CIEMainWindowQueueItem* last;
    CIEMainWindowQueueItem* item = Head;
    while (item != NULL)
    {
        last = item;
        item = item->Next;
        delete last;
    }
}

BOOL CIEMainWindowQueue::Add(CIEMainWindowQueueItem* item)
{
    CALL_STACK_MESSAGE1("CIEMainWindowQueue::Add()");
    CS.Enter();
    if (item != NULL)
    {
        item->Next = Head;
        Head = item;
        CS.Leave();
        return TRUE;
    }
    CS.Leave();
    return FALSE;
}

void CIEMainWindowQueue::Remove(HWND hWindow)
{
    CALL_STACK_MESSAGE1("CIEMainWindowQueue::Remove()");
    CS.Enter();
    CIEMainWindowQueueItem* last = NULL;
    CIEMainWindowQueueItem* item = Head;
    while (item != NULL)
    {
        if (item->HWindow == hWindow) // found, remove it
        {
            if (last != NULL)
                last->Next = item->Next;
            else
                Head = item->Next;
            delete item;
            CS.Leave();
            return;
        }
        last = item;
        item = item->Next;
    }
    CS.Leave();
}

BOOL CIEMainWindowQueue::Empty()
{
    BOOL e;
    CS.Enter();
    e = Head == NULL;
    CS.Leave();
    return e;
}

BOOL CIEMainWindowQueue::CloseAllWindows(BOOL force, int waitTime, int forceWaitTime)
{
    CALL_STACK_MESSAGE4("CIEMainWindowQueue::CloseAllWindows(%d, %d, %d)", force, waitTime, forceWaitTime);
    // send a request to close all windows
    CS.Enter();
    CIEMainWindowQueueItem* item = Head;
    while (item != NULL)
    {
        PostMessage(item->HWindow, WM_CLOSE, 0, 0);
        item = item->Next;
    }
    CS.Leave();

    // wait until/if they close
    DWORD ti = GetTickCount();
    DWORD w = force ? forceWaitTime : waitTime;
    while ((w == INFINITE || w > 0) && !Empty())
    {
        DWORD t = GetTickCount() - ti;
        if (w == INFINITE || t < w) // should we keep waiting
        {
            if (w == INFINITE || 50 < w - t)
                Sleep(50);
            else
            {
                Sleep(w - t);
                break;
            }
        }
        else
            break;
    }
    return force || Empty();
}

//
// ****************************************************************************
// CIEMainWindow
//

CIEMainWindow::CIEMainWindow()
{
    HWindow = NULL;
    Lock = NULL;
}

LRESULT
CIEMainWindow::WindowProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE4("CIEMainWindow::WindowProc(0x%X, 0x%IX, 0x%IX)", uMsg, wParam, lParam);
    switch (uMsg)
    {
    case WM_CREATE:
    {
        // Create top bar static window
        HWND hwndTopBar = CreateWindow("STATIC", "", WS_CHILD | WS_VISIBLE | SS_SUNKEN | WS_CLIPSIBLINGS,
                                       0, 0, 100, 40, HWindow, (HMENU)1002, DLLInstance, NULL);
                                       
        // Create the "Export to PDF" button inside the main window (parented to HWindow)
        HWND hwndButton = CreateWindow("BUTTON", "Export to PDF", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                                       10, 8, 120, 24, HWindow, (HMENU)1001, DLLInstance, NULL);

        if (!m_IEViewer.CreateSite(HWindow))
            return -1;
        ShowWindow(m_IEViewer.HWindow, SW_SHOW);
        return 0;
    }

    case WM_CLOSE:
    {
        if (!m_IEViewer.CanClose())
            return 0;
        break;
    }

    case WM_DESTROY:
    {
        TRACE_I("CIEMainWindow::WindowProc WM_DESTROY");
        if (Lock != NULL)
        {
            SetEvent(Lock);
            Lock = NULL;
        }
        TRACE_I("CIEMainWindow::WindowProc m_IEViewer.CloseSite()");
        m_IEViewer.CloseSite();
        TRACE_I("CIEMainWindow::WindowProc PostQuitMessage");
        PostQuitMessage(0);
        break;
    }

    case WM_SETFOCUS:
    {
        HWND hWnd = m_IEViewer.HWindow;
        HWND hIterator = hWnd;
        do
        {
            hIterator = GetWindow(hIterator, GW_CHILD);
            if (hIterator != NULL)
                hWnd = hIterator;
        } while (hIterator != NULL);
        SetFocus(hWnd);
        return 0;
    }

    case WM_ACTIVATE:
    {
        if (!LOWORD(wParam))
        {
            // the main window will not refresh when switching away from the viewer
            SalamanderGeneral->SkipOneActivateRefresh();
        }
        break;
    }

    case WM_SIZE:
    {
        int width = LOWORD(lParam);
        int height = HIWORD(lParam);

        HWND hwndTopBar = GetDlgItem(HWindow, 1002);
        if (hwndTopBar != NULL)
        {
            SetWindowPos(hwndTopBar, HWND_TOP, 0, 0, width, 40, SWP_NOZORDER);
        }

        if (m_IEViewer.HWindow != NULL)
        {
            SetWindowPos(m_IEViewer.HWindow, HWND_TOP, 0, 40, width, height - 40, SWP_NOZORDER);
        }
        break;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == 1001) // IDC_EXPORT_PDF
        {
            m_IEViewer.ExportToPdf();
            return 0;
        }
        break;
    }
    }
    return DefWindowProc(HWindow, uMsg, wParam, lParam);
}

HANDLE
CIEMainWindow::GetLock()
{
    if (Lock == NULL)
        Lock = CreateEvent(NULL, FALSE, FALSE, NULL);
    return Lock;
}

// ****************************************************************************
// static function CIEMainWindow::CIEMainWindowProc for all messages of all viewer
// windows, distributes messages to individual viewer windows via the
// CIEMainWindow::WindowProc method (a suitable place to process messages of
// individual windows)

LRESULT CALLBACK
CIEMainWindow::CIEMainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CALL_STACK_MESSAGE5("CIEMainWindow::CIEMainWindowProc(0x%p, 0x%X, 0x%IX, 0x%IX)", hwnd, uMsg, wParam, lParam);
    CIEMainWindow* wnd;
    switch (uMsg)
    {
    case WM_CREATE: // first message - attach the object to the window
    {
        wnd = (CIEMainWindow*)((CREATESTRUCT*)lParam)->lpCreateParams;
        if (wnd == NULL)
        {
            TRACE_E("Error while creating the window.");
            return FALSE;
        }
        else
        {
            wnd->HWindow = hwnd;
            SetProp(hwnd, (LPCTSTR)AtomObject, (HANDLE)wnd);
            ViewerWindowQueue.Add(new CIEMainWindowQueueItem(wnd->HWindow));
        }
        break;
    }

    case WM_DESTROY: // last message - detach the object from the window
    {
        wnd = (CIEMainWindow*)GetProp(hwnd, (LPCTSTR)AtomObject);
        if (wnd != NULL)
        {
            LRESULT res = wnd->WindowProc(uMsg, wParam, lParam);
            ViewerWindowQueue.Remove(hwnd);
            RemoveProp(hwnd, (LPCTSTR)AtomObject);
            //        delete wnd;
            if (res == 0)
                return 0; // the application handled it
            wnd = NULL;
        }
        break;
    }

    default:
    {
        wnd = (CIEMainWindow*)GetProp(hwnd, (LPCTSTR)AtomObject);
    }
    }
    // --- call the WindowProc(...) method of the corresponding window object
    if (wnd != NULL)
        return wnd->WindowProc(uMsg, wParam, lParam);
    else
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
