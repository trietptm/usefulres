/* Copyright 2006-2011 the SumatraPDF project authors (see AUTHORS file).
   License: GPLv3 */

#include "SumatraPDF.h"
#include <shlobj.h>
#include <wininet.h>
#include <locale.h>
#include <time.h>

#include "WinUtil.h"
#include "Http.h"
#include "FileUtil.h"
#include "Resource.h"
#include "translations.h"
#include "Version.h"

#include "WindowInfo.h"
#include "RenderCache.h"
#include "PdfSync.h"

#include "AppPrefs.h"
#include "SumatraDialogs.h"
#include "SumatraProperties.h"
#include "SumatraAbout.h"
#include "FileHistory.h"
#include "Favorites.h"
#include "FileWatch.h"
#include "AppTools.h"
#include "Notifications.h"
#include "TableOfContents.h"
#include "Toolbar.h"
#include "Print.h"
#include "Search.h"
#include "ExternalPdfViewer.h"
#include "Selection.h"
#include "Menu.h"
#include "Touch.h"
#include "HtmlWindow.h"

/*MyCode*/
#include "..\..\..\..\biggod.dev\Ctrl\Utility.hpp"
#include "..\..\..\..\biggod.app\PDFEditor\sumatrapdf_intf.h"
extern SumatraPdfIntf* g_pIntf;

unsigned char* ansii_to_cid(pdf_font_desc *fontdesc,unsigned char* buf,int& cid,int* o_cpt = NULL);
void my_pdf_show_char(my_pdf_gstate *gstate,int cid,fz_matrix& tm);

extern "C" {
__pragma(warning(push))
#include <mupdf.h>
__pragma(warning(pop))
}
//////////////////////////////////////////////////////////////////////////

#ifdef BUILD_RIBBON
#include "Ribbon.h"
// uncomment to actually use the (incomplete) ribbon
// #define USE_RIBBON
#endif

#include "CrashHandler.h"
#include "ParseCommandLine.h"
#include "StressTesting.h"

/* Define THREAD_BASED_FILEWATCH to use the thread-based implementation of file change detection. */
#define THREAD_BASED_FILEWATCH

#define ZOOM_IN_FACTOR      1.2f
#define ZOOM_OUT_FACTOR     1.0f / ZOOM_IN_FACTOR

/* if TRUE, we're in debug mode where we show links as blue rectangle on
   the screen. Makes debugging code related to links easier. */
#ifdef DEBUG
bool             gDebugShowLinks = true;
#else
bool             gDebugShowLinks = false;
#endif

/* if true, we're rendering everything with the GDI+ back-end,
   otherwise Fitz/MuPDF is used at least for screen rendering.
   In Debug builds, you can switch between the two through the Debug menu */
bool             gUseGdiRenderer = false;

// in plugin mode, the window's frame isn't drawn and closing and
// fullscreen are disabled, so that SumatraPDF can be displayed
// embedded (e.g. in a web browser)
TCHAR *          gPluginURL = NULL; // owned by CommandLineInfo in WinMain

#if defined(SVN_PRE_RELEASE_VER) && !defined(BLACK_ON_YELLOW)
#define ABOUT_BG_COLOR          RGB(0xFF, 0, 0)
#else
#define ABOUT_BG_COLOR          RGB(0xFF, 0xF2, 0)
#endif

// Background color comparison:
// Adobe Reader X   0x565656 without any frame border
// Foxit Reader 5   0x9C9C9C with a pronounced frame shadow
// PDF-XChange      0xACA899 with a 1px frame and a gradient shadow
// Google Chrome    0xCCCCCC with a symmetric gradient shadow
// Evince           0xD7D1CB with a pronounced frame shadow
#ifdef DRAW_PAGE_SHADOWS
// SumatraPDF (old) 0xCCCCCC with a pronounced frame shadow
#define COL_WINDOW_BG           RGB(0xCC, 0xCC, 0xCC)
#define COL_PAGE_FRAME          RGB(0x88, 0x88, 0x88)
#define COL_PAGE_SHADOW         RGB(0x40, 0x40, 0x40)
#else
// SumatraPDF       0x999999 without any frame border
#define COL_WINDOW_BG           RGB(0x99, 0x99, 0x99)
#endif

#define CANVAS_CLASS_NAME            _T("SUMATRA_PDF_CANVAS")
#define SIDEBAR_SPLITTER_CLASS_NAME  _T("SidebarSplitter")
#define FAV_SPLITTER_CLASS_NAME      _T("FavSplitter")
#define RESTRICTIONS_FILE_NAME       _T("sumatrapdfrestrict.ini")
#define CRASH_DUMP_FILE_NAME         _T("sumatrapdfcrash.dmp")

#define DEFAULT_LINK_PROTOCOLS       _T("http,https,mailto")

#define SPLITTER_DX         5
#define SIDEBAR_MIN_WIDTH   150

#define SPLITTER_DY         4
#define TOC_MIN_DY          100

#define REPAINT_TIMER_ID            1
#define REPAINT_MESSAGE_DELAY_IN_MS 1000

#define HIDE_CURSOR_TIMER_ID        3
#define HIDE_CURSOR_DELAY_IN_MS     3000

#define AUTO_RELOAD_TIMER_ID        5
#define AUTO_RELOAD_DELAY_IN_MS     100

#if !defined(THREAD_BASED_FILEWATCH)
#define FILEWATCH_DELAY_IN_MS       1000
#endif

       HINSTANCE                    ghinst = NULL;

       HCURSOR                      gCursorArrow;
       HCURSOR                      gCursorHand;
static HCURSOR                      gCursorDrag;
       HCURSOR                      gCursorIBeam;
static HCURSOR                      gCursorScroll;
static HCURSOR                      gCursorSizeWE;
static HCURSOR                      gCursorSizeNS;
static HCURSOR                      gCursorNo;
       HBRUSH                       gBrushNoDocBg;
       HBRUSH                       gBrushAboutBg;
static HBRUSH                       gBrushWhite;
static HBRUSH                       gBrushBlack;
       HFONT                        gDefaultGuiFont;
static HBITMAP                      gBitmapReloadingCue;

static RenderCache                  gRenderCache;
       Vec<WindowInfo*>             gWindows;
       FileHistory                  gFileHistory;
       Favorites *                  gFavorites;
static UIThreadWorkItemQueue        gUIThreadMarshaller;

static bool                         gIsStressTesting = false;
static bool                         gCrashOnOpen = false;

// in restricted mode, some features can be disabled (such as
// opening files, printing, following URLs), so that SumatraPDF
// can be used as a PDF reader on locked down systems
static int                          gPolicyRestrictions = Perm_All;
// only the listed protocols will be passed to the OS for
// opening in e.g. a browser or an email client (ignored,
// if gPolicyRestrictions doesn't contain Perm_DiskAccess)
static StrVec                       gAllowedLinkProtocols;

static void UpdateUITextForLanguage();
static void UpdateToolbarAndScrollbarState(WindowInfo& win);
static void EnterFullscreen(WindowInfo& win, bool presentation=false);
static void ExitFullscreen(WindowInfo& win);
static LRESULT CALLBACK WndProcCanvas(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

bool HasPermission(int permission)
{
    return (permission & gPolicyRestrictions) == permission;
}

bool CurrLangNameSet(const char *langName)
{
    const char *langCode = Trans::ConfirmLanguage(langName);
    if (!langCode)
        return false;

    gGlobalPrefs.currentLanguage = langCode;

    bool ok = Trans::SetCurrentLanguage(langCode);
    assert(ok);
    return ok;
}

#ifndef SUMATRA_UPDATE_INFO_URL
#ifdef SVN_PRE_RELEASE_VER
#define SUMATRA_UPDATE_INFO_URL _T("http://kjkpub.s3.amazonaws.com/sumatrapdf/sumpdf-prerelease-latest.txt")
#else
#define SUMATRA_UPDATE_INFO_URL _T("http://kjkpub.s3.amazonaws.com/sumatrapdf/sumpdf-latest.txt")
#endif
#endif

#ifndef SVN_UPDATE_LINK
#ifdef SVN_PRE_RELEASE_VER
#define SVN_UPDATE_LINK         _T("http://blog.kowalczyk.info/software/sumatrapdf/prerelease.html")
#else
#define SVN_UPDATE_LINK         _T("http://blog.kowalczyk.info/software/sumatrapdf")
#endif
#endif

#define SECS_IN_DAY 60*60*24

// lets the shell open a URI for any supported scheme in
// the appropriate application (web browser, mail client, etc.)
bool LaunchBrowser(const TCHAR *url)
{
    if (!HasPermission(Perm_DiskAccess))
        return false;

    // check if this URL's protocol is allowed
    const TCHAR *colon = str::FindChar(url, ':');
    if (!colon)
        return false;
    ScopedMem<TCHAR> protocol(str::DupN(url, colon - url));
    str::ToLower(protocol);
    if (gAllowedLinkProtocols.Find(protocol) == -1)
        return false;

    return LaunchFile(url, NULL, _T("open"));
}

#define DEFINE_GUID_STATIC(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
    static const GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }
DEFINE_GUID_STATIC(CLSID_SendMail, 0x9E56BE60, 0xC50F, 0x11CF, 0x9A, 0x2C, 0x00, 0xA0, 0xC9, 0x0A, 0x90, 0xCE); 

bool CanSendAsEmailAttachment(WindowInfo *win)
{
    // Requirements: a valid filename and access to SendMail's IDropTarget interface
    if (!CanViewExternally(win))
        return false;

    ScopedComPtr<IDropTarget> pDropTarget;
    HRESULT hr = CoCreateInstance(CLSID_SendMail, NULL, CLSCTX_ALL,
                                  IID_IDropTarget, (void **)&pDropTarget);
    return SUCCEEDED(hr);
}

static bool SendAsEmailAttachment(WindowInfo *win)
{
    if (!CanSendAsEmailAttachment(win))
        return false;

    // We use the SendTo drop target provided by SendMail.dll, which should ship with all
    // commonly used Windows versions, instead of MAPISendMail, which doesn't support
    // Unicode paths and might not be set up on systems not having Microsoft Outlook installed.
    ScopedComPtr<IDataObject> pDataObject(GetDataObjectForFile(win->dm->FileName(), win->hwndFrame));
    if (!pDataObject)
        return false;

    ScopedComPtr<IDropTarget> pDropTarget;
    HRESULT hr = CoCreateInstance(CLSID_SendMail, NULL, CLSCTX_ALL,
                                  IID_IDropTarget, (void **)&pDropTarget);
    if (FAILED(hr))
        return false;

    POINTL pt = { 0, 0 };
    DWORD dwEffect = 0;
    pDropTarget->DragEnter(pDataObject, MK_LBUTTON, pt, &dwEffect);
    hr = pDropTarget->Drop(pDataObject, MK_LBUTTON, pt, &dwEffect);
    return SUCCEEDED(hr);
}

inline void MoveWindow(HWND hwnd, RectI rect)
{
    MoveWindow(hwnd, rect.x, rect.y, rect.dx, rect.dy, TRUE);
}

void SwitchToDisplayMode(WindowInfo *win, DisplayMode displayMode, bool keepContinuous)
{
    if (!win->IsDocLoaded())
        return;

    if (keepContinuous && displayModeContinuous(win->dm->displayMode())) {
        switch (displayMode) {
            case DM_SINGLE_PAGE: displayMode = DM_CONTINUOUS; break;
            case DM_FACING: displayMode = DM_CONTINUOUS_FACING; break;
            case DM_BOOK_VIEW: displayMode = DM_CONTINUOUS_BOOK_VIEW; break;
        }
    }

    win->dm->ChangeDisplayMode(displayMode);
    UpdateToolbarState(win);
#ifdef BUILD_RIBBON
    if (win->ribbonSupport)
        win->ribbonSupport->UpdateState();
#endif
}

WindowInfo *FindWindowInfoByHwnd(HWND hwnd)
{
    HWND parent = GetParent(hwnd);
    for (size_t i = 0; i < gWindows.Count(); i++) {
        WindowInfo *win = gWindows.At(i);
        if (hwnd == win->hwndFrame      ||
            hwnd == win->hwndProperties ||
            // canvas, toolbar, rebar, tocbox, splitters
            parent == win->hwndFrame    ||
            // infotips, message windows
            parent == win->hwndCanvas   ||
            // page and find labels and boxes
            parent == win->hwndToolbar  ||
            // ToC tree, sidebar title and close button
            parent == win->hwndTocBox   ||
            // Favorites tree, title, and close button
            parent == win->hwndFavBox)
        {
            return win;
        }
    }
    return NULL;
}

bool WindowInfoStillValid(WindowInfo *win)
{
    return gWindows.Find(win) != -1;
}

// Find the first window showing a given PDF file 
WindowInfo* FindWindowInfoByFile(const TCHAR *file)
{
    ScopedMem<TCHAR> normFile(path::Normalize(file));

    for (size_t i = 0; i < gWindows.Count(); i++) {
        WindowInfo *win = gWindows.At(i);
        if (!win->IsAboutWindow() && path::IsSame(win->loadedFilePath, normFile))
            return win;
    }

    return NULL;
}

// Find the first window that has been produced from <file>
WindowInfo* FindWindowInfoBySyncFile(const TCHAR *file)
{
    for (size_t i = 0; i < gWindows.Count(); i++) {
        WindowInfo *win = gWindows.At(i);
        Vec<RectI> rects;
        UINT page;
        if (win->pdfsync && win->pdfsync->SourceToDoc(file, 0, 0, &page, rects) != PDFSYNCERR_UNKNOWN_SOURCEFILE)
            return win;
    }
    return NULL;
}

/* Get password for a given 'fileName', can be NULL if user cancelled the
   dialog box or if the encryption key has been filled in instead.
   Caller needs to free() the result. */
TCHAR *WindowInfo::GetPassword(const TCHAR *fileName, unsigned char *fileDigest,
                               unsigned char decryptionKeyOut[32], bool *saveKey)
{
    DisplayState *fileFromHistory = gFileHistory.Find(fileName);
    if (fileFromHistory && fileFromHistory->decryptionKey) {
        ScopedMem<char> fingerprint(str::MemToHex(fileDigest, 16));
        *saveKey = str::StartsWith(fileFromHistory->decryptionKey, fingerprint.Get());
        if (*saveKey && str::HexToMem(fileFromHistory->decryptionKey + 32, decryptionKeyOut, 32))
            return NULL;
    }

    *saveKey = false;
    if (suppressPwdUI)
        return NULL;

    fileName = path::GetBaseName(fileName);
    return Dialog_GetPassword(this->hwndFrame, fileName, gGlobalPrefs.rememberOpenedFiles ? saveKey : NULL);
}

static void RememberWindowPosition(WindowInfo& win)
{
    // update global windowState for next default launch when either
    // no pdf is opened or a document without window dimension information
    if (win.presentation)
        gGlobalPrefs.windowState = win.windowStateBeforePresentation;
    else if (win.fullScreen)
        gGlobalPrefs.windowState = WIN_STATE_FULLSCREEN;
    else if (IsZoomed(win.hwndFrame))
        gGlobalPrefs.windowState = WIN_STATE_MAXIMIZED;
    else if (!IsIconic(win.hwndFrame))
        gGlobalPrefs.windowState = WIN_STATE_NORMAL;

    gGlobalPrefs.sidebarDx = WindowRect(win.hwndTocBox).dx;

    /* don't update the window's dimensions if it is maximized, mimimized or fullscreened */
    if (WIN_STATE_NORMAL == gGlobalPrefs.windowState &&
        !IsIconic(win.hwndFrame) && !win.presentation) {
        // TODO: Use Get/SetWindowPlacement (otherwise we'd have to separately track
        //       the non-maximized dimensions for proper restoration)
        gGlobalPrefs.windowPos = WindowRect(win.hwndFrame);
    }
}

static void UpdateDisplayStateWindowRect(WindowInfo& win, DisplayState& ds, bool updateGlobal=true)
{
    if (updateGlobal)
        RememberWindowPosition(win);

    ds.windowState = gGlobalPrefs.windowState;
    ds.windowPos   = gGlobalPrefs.windowPos;
    ds.sidebarDx   = gGlobalPrefs.sidebarDx;
}

static void UpdateSidebarDisplayState(WindowInfo *win, DisplayState *ds)
{
    ds->tocVisible = win->tocVisible;

    if (win->tocLoaded) {
        win->tocState.Reset();
        HTREEITEM hRoot = TreeView_GetRoot(win->hwndTocTree);
        if (hRoot)
            UpdateTocExpansionState(win, hRoot);
    }

    delete ds->tocState;
    ds->tocState = NULL;
    if (win->tocState.Count() > 0)
        ds->tocState = new Vec<int>(win->tocState);
}

void UpdateCurrentFileDisplayStateForWin(WindowInfo* win)
{
    RememberWindowPosition(*win);
    if (!win->IsDocLoaded())
        return;

    const TCHAR *fileName = win->dm->FileName();
    assert(fileName);
    if (!fileName)
        return;

    DisplayState *state = gFileHistory.Find(fileName);
    assert(state || !gGlobalPrefs.rememberOpenedFiles);
    if (!state)
        return;

    if (!win->dm->DisplayStateFromModel(state))
        return;
    state->useGlobalValues = gGlobalPrefs.globalPrefsOnly;
    UpdateDisplayStateWindowRect(*win, *state, false);
    UpdateSidebarDisplayState(win, state);
}

bool IsUIRightToLeft()
{
    int langIx = Trans::GetLanguageIndex(gGlobalPrefs.currentLanguage);
    return Trans::IsLanguageRtL(langIx);
}

// updates the layout for a window to either left-to-right or right-to-left
// depending on the currently used language (cf. IsUIRightToLeft)
static void UpdateWindowRtlLayout(WindowInfo *win)
{
    bool isRTL = IsUIRightToLeft();
    bool wasRTL = (GetWindowLong(win->hwndFrame, GWL_EXSTYLE) & WS_EX_LAYOUTRTL) != 0;
    if (wasRTL == isRTL)
        return;

    bool tocVisible = win->tocVisible;
    if (tocVisible || gGlobalPrefs.favVisible)
        SetSidebarVisibility(win, false, false);

    // cf. http://www.microsoft.com/middleeast/msdn/mirror.aspx
    ToggleWindowStyle(win->hwndFrame, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, isRTL, GWL_EXSTYLE);

    ToggleWindowStyle(win->hwndTocBox, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, isRTL, GWL_EXSTYLE);
    HWND tocBoxTitle = GetDlgItem(win->hwndTocBox, IDC_TOC_TITLE);
    ToggleWindowStyle(tocBoxTitle, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, isRTL, GWL_EXSTYLE);

    ToggleWindowStyle(win->hwndFavBox, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, isRTL, GWL_EXSTYLE);
    HWND favBoxTitle = GetDlgItem(win->hwndFavBox, IDC_FAV_TITLE);
    ToggleWindowStyle(favBoxTitle, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, isRTL, GWL_EXSTYLE);
    ToggleWindowStyle(win->hwndFavTree, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, isRTL, GWL_EXSTYLE);

    ToggleWindowStyle(win->hwndReBar, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, isRTL, GWL_EXSTYLE);
    ToggleWindowStyle(win->hwndToolbar, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, isRTL, GWL_EXSTYLE);
    ToggleWindowStyle(win->hwndFindBox, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, isRTL, GWL_EXSTYLE);
    ToggleWindowStyle(win->hwndFindText, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, isRTL, GWL_EXSTYLE);
    ToggleWindowStyle(win->hwndPageText, WS_EX_LAYOUTRTL | WS_EX_NOINHERITLAYOUT, isRTL, GWL_EXSTYLE);

    win->notifications->Relayout();

    // TODO: also update the canvas scrollbars (?)

    // ensure that the ToC sidebar is on the correct side and that its
    // title and close button are also correctly laid out
    if (tocVisible || gGlobalPrefs.favVisible) {
        SetSidebarVisibility(win, tocVisible, gGlobalPrefs.favVisible);
        if (tocVisible)
            SendMessage(win->hwndTocBox, WM_SIZE, 0, 0);
        if (gGlobalPrefs.favVisible)
            SendMessage(win->hwndFavBox, WM_SIZE, 0, 0);
    }
}

void UpdateRtlLayoutForAllWindows()
{
    for (size_t i = 0; i < gWindows.Count(); i++) {
        UpdateWindowRtlLayout(gWindows.At(i));
    }
}

static int GetPolicies(bool isRestricted)
{
    static struct {
        const TCHAR *name;
        int perm;
    } policies[] = {
        { _T("InternetAccess"), Perm_InternetAccess },
        { _T("DiskAccess"),     Perm_DiskAccess },
        { _T("SavePreferences"),Perm_SavePreferences },
        { _T("RegistryAccess"), Perm_RegistryAccess },
        { _T("PrinterAccess"),  Perm_PrinterAccess },
        { _T("CopySelection"),  Perm_CopySelection },
    };

    // allow to restrict SumatraPDF's functionality from an INI file in the
    // same directory as SumatraPDF.exe (cf. ../docs/sumatrapdfrestrict.ini)
    ScopedMem<TCHAR> restrictPath(GetExePath());
    if (restrictPath) {
        restrictPath.Set(path::GetDir(restrictPath));
        restrictPath.Set(path::Join(restrictPath, RESTRICTIONS_FILE_NAME));
    }
    if (file::Exists(restrictPath)) {
        int policy = Perm_RestrictedUse;
        for (size_t i = 0; i < dimof(policies); i++) {
            int check = GetPrivateProfileInt(_T("Policies"), policies[i].name, 0, restrictPath);
            if (check)
                policy = policy | policies[i].perm;
        }

        // determine the list of allowed link protocols
        gAllowedLinkProtocols.Reset();
        if ((policy & Perm_DiskAccess)) {
            TCHAR protocols[200];
            ZeroMemory(protocols, sizeof(protocols));
            GetPrivateProfileString(_T("Policies"), _T("LinkProtocols"), DEFAULT_LINK_PROTOCOLS, protocols, dimof(protocols), restrictPath);
            str::ToLower(protocols);
            str::TransChars(protocols, _T(":; "), _T(",,,"));
            gAllowedLinkProtocols.Split(protocols, _T(","), true);
        }

        return policy;
    }

    gAllowedLinkProtocols.Reset();
    if (!isRestricted)
        gAllowedLinkProtocols.Split(DEFAULT_LINK_PROTOCOLS, _T(","));

    return isRestricted ? Perm_RestrictedUse : Perm_All;
}

void QueueWorkItem(UIThreadWorkItem *wi)
{
    gUIThreadMarshaller.Queue(wi);
}

static bool SaveThumbnailForFile(const TCHAR *filePath, RenderedBitmap *bmp)
{
    DisplayState *ds = gFileHistory.Find(filePath);
    if (!ds)
        return false;
    if (ds->thumbnail)
        delete ds->thumbnail;
    ds->thumbnail = bmp;
    SaveThumbnail(*ds);
    return true;
}

class ThumbnailRenderingWorkItem : public UIThreadWorkItem, public RenderingCallback
{
    const TCHAR *filePath;
    RenderedBitmap *bmp;

public:
    ThumbnailRenderingWorkItem(WindowInfo *win, const TCHAR *filePath) :
        UIThreadWorkItem(win), filePath(str::Dup(filePath)), bmp(NULL) {
    }

    ~ThumbnailRenderingWorkItem() {
        free((void *)filePath);
        delete bmp;
    }

    virtual void Callback(RenderedBitmap *bmp) {
        this->bmp = bmp;
        QueueWorkItem(this);
    }

    virtual void Execute() {
        if (SaveThumbnailForFile(filePath, bmp))
            bmp = NULL;
    }
};

// Create a thumbnail of chm document by loading it again and rendering
// its first page to a hwnd specially created for it. An alternative
// would be to reuse ChmEngine/HtmlWindow we already have but it has
// its own problem.
// Could be done in background but no need to do that unless it's
// too slow (I've measured it at ~1sec for a sample document)
static void CreateChmThumbnail(WindowInfo& win, DisplayState& ds)
{
    assert(win.IsChm());
    if (!win.IsChm()) return;

    MillisecondTimer t(true);

    ChmEngine *chmEngine = static_cast<ChmEngine *>(win.dm->AsChmEngine()->Clone());
    if (!chmEngine)
        return;

    SizeI thumbSize(THUMBNAIL_DX, THUMBNAIL_DY);
    RenderedBitmap *bmp = chmEngine->CreateThumbnail(thumbSize);
    if (bmp && SaveThumbnailForFile(win.loadedFilePath, bmp))
        bmp = NULL;
    delete bmp;
    delete chmEngine;

    t.Stop();
    double dur = t.GetTimeInMs();
    if (dur > 1000.0)
        DBG_OUT("Formatting %s took %.2f secs\n", win.loadedFilePath, dur / 1000.0);
}

void CreateThumbnailForFile(WindowInfo& win, DisplayState& ds)
{
    // don't even create thumbnails for files that won't need them anytime soon
    Vec<DisplayState *> *list = gFileHistory.GetFrequencyOrder();
    int ix = list->Find(&ds);
    delete list;
    if (ix < 0 || FILE_HISTORY_MAX_FREQUENT * 2 <= ix)
        return;

    if (HasThumbnail(ds))
        return;

    // don't unnecessarily accumulate thumbnails during a stress test
    if (gIsStressTesting)
        return;

    if (win.IsChm()) {
        CreateChmThumbnail(win, ds);
        return;
    }

    RectD pageRect = win.dm->engine->PageMediabox(1);
    if (pageRect.IsEmpty())
        return;

    pageRect = win.dm->engine->Transform(pageRect, 1, 1.0f, 0);
    float zoom = THUMBNAIL_DX / (float)pageRect.dx;
    if (pageRect.dy > (float)THUMBNAIL_DY / zoom)
        pageRect.dy = (float)THUMBNAIL_DY / zoom;
    pageRect = win.dm->engine->Transform(pageRect, 1, 1.0f, 0, true);

    RenderingCallback *callback = new ThumbnailRenderingWorkItem(&win, win.loadedFilePath);
    gRenderCache.Render(win.dm, 1, 0, zoom, pageRect, *callback);
}

static void RebuildMenuBarForWindow(WindowInfo *win)
{
#ifdef BUILD_RIBBON
    if (win->ribbonSupport)
        win->ribbonSupport->Reset();
    else
#endif
    {
        HMENU oldMenu = win->menu;
        win->menu = BuildMenu(win);
        DestroyMenu(oldMenu);
    }
}

static void RebuildMenuBarForAllWindows()
{
    for (size_t i = 0; i < gWindows.Count(); i++) {
        RebuildMenuBarForWindow(gWindows.At(i));
    }
}

// When displaying CHM document we subclass hwndCanvas. UnsubclassCanvas() reverts that.
static void UnsubclassCanvas(HWND hwnd)
{
    SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProcCanvas);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)0);
}

// isNewWindow : if true then 'win' refers to a newly created window that needs 
//   to be resized and placed
// allowFailure : if false then keep displaying the previously loaded document
//   if the new one is broken
// placeWindow : if true then the Window will be moved/sized according
//   to the 'state' information even if the window was already placed
//   before (isNewWindow=false)
static bool LoadDocIntoWindow(TCHAR *fileName, WindowInfo& win,
    DisplayState *state, bool isNewWindow, bool allowFailure, 
    bool showWin, bool placeWindow)
{
    ScopedMem<TCHAR> title;

    // Never load settings from a preexisting state if the user doesn't wish to
    // (unless we're just refreshing the document, i.e. only if placeWindow == true)
    if (placeWindow && (gGlobalPrefs.globalPrefsOnly || state && state->useGlobalValues)) {
        state = NULL;
    } else if (NULL == state) {
        state = gFileHistory.Find(fileName);
        if (state) {
            AdjustRemovableDriveLetter(fileName);
            if (state->windowPos.IsEmpty())
                state->windowPos = gGlobalPrefs.windowPos;
            EnsureAreaVisibility(state->windowPos);
        }
    }
    DisplayMode displayMode = gGlobalPrefs.defaultDisplayMode;
    int startPage = 1;
    ScrollState ss(1, -1, -1);
    bool showAsFullScreen = WIN_STATE_FULLSCREEN == gGlobalPrefs.windowState;
    int showType = SW_NORMAL;
    if (gGlobalPrefs.windowState == WIN_STATE_MAXIMIZED || showAsFullScreen)
        showType = SW_MAXIMIZE;

    bool tocVisible = gGlobalPrefs.tocVisible;
    if (state) {
        startPage = state->pageNo;
        displayMode = state->displayMode;
        showAsFullScreen = WIN_STATE_FULLSCREEN == state->windowState;
        if (state->windowState == WIN_STATE_NORMAL)
            showType = SW_NORMAL;
        else if (state->windowState == WIN_STATE_MAXIMIZED || showAsFullScreen)
            showType = SW_MAXIMIZE;
        else if (state->windowState == WIN_STATE_MINIMIZED)
            showType = SW_MINIMIZE;
        tocVisible = state->tocVisible;
    }

    DisplayModel *prevModel = win.dm;
    AbortFinding(&win);
    delete win.pdfsync;
    win.pdfsync = NULL;

    str::ReplacePtr(&win.loadedFilePath, fileName);
    win.dm = DisplayModel::CreateFromFileName(fileName, &win);

    // make sure that MSHTML can't be used as a potential exploit
    // vector through another browser and our plugin (which doesn't
    // advertise itself for Chm documents but could be tricked into
    // loading one nonetheless)
    if (gPluginMode && win.dm && win.dm->AsChmEngine() && IsUntrustedFile(fileName, gPluginURL)) {
        delete win.dm;
        win.dm = NULL;
    }

    bool needrefresh = !win.dm;

    // ToC items might hold a reference to an Engine, so make sure to
    // delete them before destroying the whole DisplayModel
    if (win.dm || allowFailure)
        ClearTocBox(&win);

    assert(!win.IsAboutWindow() && win.IsDocLoaded() == (win.dm != NULL));
    /* see http://code.google.com/p/sumatrapdf/issues/detail?id=1570
    if (!win.dm) {
        // TODO: this should be "Error opening %s". Change after 1.7 is released
        ScopedMem<TCHAR> msg(str::Format(_TR("Error loading %s"), win.loadedFilePath));
        ShowNotification(&win, msg, true, false, NG_RESPONSE_TO_ACTION);
        // TODO: CloseWindow() does slightly more than this
        //       (also, some code presumes that there is at least one window with
        //        IsAboutWindow() == true and that that window is gWindows.At(0))
        str::ReplacePtr(&win.loadedFilePath, NULL);
    }
    */
    if (win.dm) {
        win.dm->SetInitialViewSettings(displayMode, startPage, win.GetViewPortSize(), win.dpi);
        if (prevModel && str::Eq(win.dm->FileName(), prevModel->FileName())) {
            gRenderCache.KeepForDisplayModel(prevModel, win.dm);
            win.dm->CopyNavHistory(*prevModel);
        }
        delete prevModel;
        ChmEngine *chmEngine = win.dm->AsChmEngine();
        if (chmEngine)
            chmEngine->SetParentHwnd(win.hwndCanvas);
    } else if (allowFailure) {
        DBG_OUT("failed to load file %s\n", fileName);
        delete prevModel;
        ScopedMem<TCHAR> title(str::Format(_T("%s - %s"), path::GetBaseName(fileName), SUMATRA_WINDOW_TITLE));
        win::SetText(win.hwndFrame, title);
        goto Error;
    } else {
        // if there is an error while reading the document and a repair is not requested
        // then fallback to the previous state
        DBG_OUT("failed to load file %s, falling back to previous DisplayModel\n", fileName);
        win.dm = prevModel;
    }

    float zoomVirtual = gGlobalPrefs.defaultZoom;
    int rotation = 0;

    if (state) {
        if (win.dm->ValidPageNo(startPage)) {
            ss.page = startPage;
            if (ZOOM_FIT_CONTENT != state->zoomVirtual) {
                ss.x = state->scrollPos.x;
                ss.y = state->scrollPos.y;
            }
            // else let win.dm->Relayout() scroll to fit the page (again)
        } else if (startPage > win.dm->PageCount()) {
            ss.page = win.dm->PageCount();
        }
        zoomVirtual = state->zoomVirtual;
        rotation = state->rotation;

        win.tocState.Reset();
        if (state->tocState)
            win.tocState = *state->tocState;
    }

    win.dm->Relayout(zoomVirtual, rotation);

    if (!isNewWindow) {
        win.RedrawAll();
        OnMenuFindMatchCase(&win);
    }
    UpdateFindbox(&win);

    // menu for chm docs is different, so we have to re-create it
    RebuildMenuBarForWindow(&win);

    int pageCount = win.dm->PageCount();
    if (pageCount > 0) {
        UpdateToolbarPageText(&win, pageCount);
        UpdateToolbarFindText(&win);
    }

    const TCHAR *baseName = path::GetBaseName(win.dm->FileName());
    TCHAR *docTitle = win.dm->engine ? win.dm->engine->GetProperty("Title") : NULL;
    if (!str::IsEmpty(docTitle)) {
        ScopedMem<TCHAR> docTitleBit(str::Format(_T("- [%s] "), docTitle));
        free(docTitle);
        docTitle = docTitleBit.StealData();
    }
    title.Set(str::Format(_T("%s %s- %s"), baseName, docTitle ? docTitle : _T(""), SUMATRA_WINDOW_TITLE));
#ifdef UNICODE
    if (IsUIRightToLeft()) {
        // explicitly revert the title, so that filenames aren't garbled
        title.Set(str::Format(_T("%s %s- %s"), SUMATRA_WINDOW_TITLE, docTitle ? docTitle : _T(""), baseName));
    }
#endif
    free(docTitle);
    if (needrefresh)
        title.Set(str::Format(_TR("[Changes detected; refreshing] %s"), title));
    win::SetText(win.hwndFrame, title);

    if (HasPermission(Perm_DiskAccess) && Engine_PDF == win.dm->engineType) {
        int res = Synchronizer::Create(fileName, win.dm, &win.pdfsync);
        // expose SyncTeX in the UI
        if (PDFSYNCERR_SUCCESS == res)
            gGlobalPrefs.enableTeXEnhancements = true;
    }

Error:
    if (isNewWindow || placeWindow && state) {
        if (isNewWindow && state && !state->windowPos.IsEmpty()) {
            // Make sure it doesn't have a position like outside of the screen etc.
            RectI rect = ShiftRectToWorkArea(state->windowPos);
            // This shouldn't happen until !win.IsAboutWindow(), so that we don't
            // accidentally update gGlobalState with this window's dimensions
            MoveWindow(win.hwndFrame, rect);
        }

        if (showWin) {
            ShowWindow(win.hwndFrame, showType);
        }
        UpdateWindow(win.hwndFrame);
    }
    if (win.IsDocLoaded()) {
        ToggleWindowStyle(win.hwndPageBox, ES_NUMBER, !win.dm->engine || !win.dm->engine->HasPageLabels());
        // if the window isn't shown and win.canvasRc is still empty, zoom has not been determined yet
        assert(!showWin || !win.canvasRc.IsEmpty() || win.IsChm());
        if (showWin || ss.page != 1)
            win.dm->SetScrollState(ss);
        UpdateToolbarState(&win);
        // Note: this is a hack. Somewhere between r4593 and r4629
        // restoring zoom for chm files from history regressed and
        // I'm too lazy to figure out where and why. This forces
        // setting zoom level after a page has been displayed
        // (indirectly triggered via UpdateToolbarState()).
        if (win.IsChm()) {
            win.dm->Relayout(zoomVirtual, rotation);
        }
    }
#ifdef BUILD_RIBBON
    if (win.ribbonSupport)
        win.ribbonSupport->UpdateState();
#endif

    SetSidebarVisibility(&win, tocVisible, gGlobalPrefs.favVisible);
    win.RedrawAll(true);

    UpdateToolbarAndScrollbarState(win);
    if (!win.IsDocLoaded()) {
        win.RedrawAll();
        return false;
    }
    // This should only happen after everything else is ready
    if ((isNewWindow || placeWindow) && showWin && showAsFullScreen)
        EnterFullscreen(win);
    if (!isNewWindow && win.presentation && win.dm)
        win.dm->SetPresentationMode(true);

    return true;
}

void ReloadDocument(WindowInfo *win, bool autorefresh)
{
    if (win->IsChm() && InHtmlNestedMessagePump())
        return;

    DisplayState ds;
    ds.useGlobalValues = gGlobalPrefs.globalPrefsOnly;
    if (!win->IsDocLoaded() || !win->dm->DisplayStateFromModel(&ds)) {
        if (!autorefresh && !win->IsDocLoaded() && !win->IsAboutWindow())
            LoadDocument(win->loadedFilePath, win);
        return;
    }
    UpdateDisplayStateWindowRect(*win, ds);
    UpdateSidebarDisplayState(win, &ds);
    // Set the windows state based on the actual window's placement
    ds.windowState =  win->fullScreen ? WIN_STATE_FULLSCREEN
                    : IsZoomed(win->hwndFrame) ? WIN_STATE_MAXIMIZED 
                    : IsIconic(win->hwndFrame) ? WIN_STATE_MINIMIZED
                    : WIN_STATE_NORMAL ;

    // We don't allow PDF-repair if it is an autorefresh because
    // a refresh event can occur before the file is finished being written,
    // in which case the repair could fail. Instead, if the file is broken, 
    // we postpone the reload until the next autorefresh event
    bool allowFailure = !autorefresh;
    bool isNewWindow = false;
    bool showWin = true;
    bool placeWindow = false;
    ScopedMem<TCHAR> path(str::Dup(win->loadedFilePath));
    if (!LoadDocIntoWindow(path, *win, &ds, isNewWindow, allowFailure, showWin, placeWindow))
        return;

    if (gGlobalPrefs.showStartPage) {
        // refresh the thumbnail for this file
        DisplayState *state = gFileHistory.Find(ds.filePath);
        if (state)
            CreateThumbnailForFile(*win, *state);
    }

    // save a newly remembered password into file history so that
    // we don't ask again at the next refresh
    ScopedMem<char> decryptionKey(win->dm->engine->GetDecryptionKey());
    if (decryptionKey) {
        DisplayState *state = gFileHistory.Find(ds.filePath);
        if (state && !str::Eq(state->decryptionKey, decryptionKey))
            str::ReplacePtr(&state->decryptionKey, decryptionKey);
    }
}

static void UpdateToolbarAndScrollbarState(WindowInfo& win)
{
    ToolbarUpdateStateForWindow(&win, true);
#ifdef BUILD_RIBBON
    if (win.ribbonSupport)
        win.ribbonSupport->UpdateState();
#endif

    if (!win.IsDocLoaded()) {
        ShowScrollBar(win.hwndCanvas, SB_BOTH, FALSE);
        if (win.IsAboutWindow())
            win::SetText(win.hwndFrame, SUMATRA_WINDOW_TITLE);
    }
}

static void CreateSidebar(WindowInfo* win)
{
    win->hwndSidebarSplitter = CreateWindow(SIDEBAR_SPLITTER_CLASS_NAME, _T(""), 
        WS_CHILDWINDOW, 0, 0, 0, 0, win->hwndFrame, (HMENU)0, ghinst, NULL);

    CreateToc(win);
    win->hwndFavSplitter = CreateWindow(FAV_SPLITTER_CLASS_NAME, _T(""), 
        WS_CHILDWINDOW, 0, 0, 0, 0, win->hwndFrame, (HMENU)0, ghinst, NULL);
    CreateFavorites(win);

    if (win->tocVisible) {
        InvalidateRect(win->hwndTocBox, NULL, TRUE);
        UpdateWindow(win->hwndTocBox);
    }

    if (gGlobalPrefs.favVisible) {
        InvalidateRect(win->hwndFavBox, NULL, TRUE);
        UpdateWindow(win->hwndFavBox);
    }
}

static WindowInfo* CreateWindowInfo()
{
    RectI windowPos = gGlobalPrefs.windowPos;
    if (!windowPos.IsEmpty())
        EnsureAreaVisibility(windowPos);
    else
        windowPos = GetDefaultWindowPos();

    HWND hwndFrame = CreateWindow(
            FRAME_CLASS_NAME, SUMATRA_WINDOW_TITLE,
            WS_OVERLAPPEDWINDOW,
            windowPos.x, windowPos.y, windowPos.dx, windowPos.dy,
            NULL, NULL,
            ghinst, NULL);
    if (!hwndFrame)
        return NULL;

    assert(NULL == FindWindowInfoByHwnd(hwndFrame));
    WindowInfo *win = new WindowInfo(hwndFrame);

    win->hwndCanvas = CreateWindowEx(
            WS_EX_STATICEDGE, 
            CANVAS_CLASS_NAME, NULL,
            WS_CHILD | WS_HSCROLL | WS_VSCROLL,
            0, 0, 0, 0, /* position and size determined in OnSize */
            hwndFrame, NULL,
            ghinst, NULL);
    if (!win->hwndCanvas) {
        delete win;
        return NULL;
    }

    // hide scrollbars to avoid showing/hiding on empty window
    ShowScrollBar(win->hwndCanvas, SB_BOTH, FALSE);

#if defined(BUILD_RIBBON) && defined(USE_RIBBON)
    if (gGlobalPrefs.useRibbon) {
        win->ribbonSupport = new RibbonSupport(win);
        if (!win->ribbonSupport->Initialize(ghinst, L"APPLICATION_RIBBON")) {
            delete win->ribbonSupport;
            win->ribbonSupport = NULL;
        }
        else if (gGlobalPrefs.ribbonState)
            win->ribbonSupport->SetState(gGlobalPrefs.ribbonState);
        else
            // collapse the ribbon per default
            win->ribbonSupport->SetMinimized(true);
    }
    if (!win->ribbonSupport)
#endif
    {
        assert(!win->menu);
        win->menu = BuildMenu(win);
    }

    ShowWindow(win->hwndCanvas, SW_SHOW);
    UpdateWindow(win->hwndCanvas);

    win->hwndInfotip = CreateWindowEx(WS_EX_TOPMOST,
        TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        win->hwndCanvas, NULL, ghinst, NULL);

    CreateToolbar(win);
#ifdef BUILD_RIBBON
    if (win->ribbonSupport)
        ShowWindow(win->hwndReBar, SW_HIDE);
#endif
    CreateSidebar(win);
    UpdateFindbox(win);
    DragAcceptFiles(win->hwndCanvas, TRUE);

    gWindows.Append(win);
    UpdateWindowRtlLayout(win);

    if (Touch::SupportsGestures()) {
        GESTURECONFIG gc = { 0, GC_ALLGESTURES, 0 };
        Touch::SetGestureConfig(hwndFrame, 0, 1, &gc, sizeof(GESTURECONFIG));
    }

    return win;
}

static void DeleteWindowInfo(WindowInfo *win)
{
    assert(win);
    if (!win) return;

    // must DestroyWindow(win->hwndProperties) before removing win from
    // the list of properties beacuse WM_DESTROY handler needs to find
    // WindowInfo for its HWND
    if (win->hwndProperties) {
        DestroyWindow(win->hwndProperties);
        assert(NULL == win->hwndProperties);
    }
    gWindows.Remove(win);

    ImageList_Destroy((HIMAGELIST)SendMessage(win->hwndToolbar, TB_GETIMAGELIST, 0, 0));
    DragAcceptFiles(win->hwndCanvas, FALSE);

    AbortFinding(win);
    AbortPrinting(win);

#ifdef BUILD_RIBBON
    assert(!win->menu != !win->ribbonSupport);
    if (win->ribbonSupport) {
        free(gGlobalPrefs.ribbonState);
        gGlobalPrefs.ribbonState = win->ribbonSupport->GetState();
    }
    delete win->ribbonSupport;
#endif
    delete win;
}

class FileChangeCallback : public UIThreadWorkItem, public CallbackFunc
{
public:
    FileChangeCallback(WindowInfo *win) : UIThreadWorkItem(win) { }

    virtual void Callback() {
        // We cannot call win->Reload directly as it could cause race conditions
        // between the watching thread and the main thread (and only pass a copy of this
        // callback to the UIThreadMarshaller, as the object will be deleted after use)
        QueueWorkItem(new FileChangeCallback(win));
    }

    virtual void Execute() {
        if (WindowInfoStillValid(win)) {
            // delay the reload slightly, in case we get another request immediately after this one
            SetTimer(win->hwndCanvas, AUTO_RELOAD_TIMER_ID, AUTO_RELOAD_DELAY_IN_MS, NULL);
        }
    }
};

#ifndef THREAD_BASED_FILEWATCH
static void RefreshUpdatedFiles() {
    for (size_t i = 0; i < gWindows.Count(); i++) {
        WindowInfo *win = gWindows.At(i);
        if (win->watcher)
            win->watcher->CheckForChanges();
    }
}
#endif

// for testing only
static void CrashMe()
{
#if 1
    char *p = NULL;
    *p = 0;
#else
    SubmitCrashInfo();
#endif
}

WindowInfo* LoadDocument(const TCHAR *fileName, WindowInfo *win, bool showWin, bool forceReuse, bool suppressPwdUI)
{
    if (gCrashOnOpen)
        CrashMe();

    ScopedMem<TCHAR> fullpath(path::Normalize(fileName));

    // fail with a notification if the file doesn't exist and
    // there is a window the user has just been interacting with
    // Note: embedded documents are referred to by an invalid path
    //       containing more information after a colon (e.g. "C:\file.pdf:3:0")
    if (win && !forceReuse && !file::Exists(fullpath) && !dir::Exists(fullpath) && !str::FindChar(fullpath + 2, ':')) {
        ScopedMem<TCHAR> msg(str::Format(_TR("File %s not found"), fullpath));
        ShowNotification(win, msg, true /* autoDismiss */, true /* highlight */);
        // display the notification ASAP (SavePrefs() can introduce a notable delay)
        win->RedrawAll(true);

        if (gFileHistory.MarkFileInexistent(fullpath)) {
            SavePrefs();
            // update the Frequently Read list
            if (1 == gWindows.Count() && gWindows.At(0)->IsAboutWindow())
                gWindows.At(0)->RedrawAll(true);
        }
        return NULL;
    }

    bool isNewWindow = false;
    if (!win && 1 == gWindows.Count() && gWindows.At(0)->IsAboutWindow()) {
        win = gWindows.At(0);
    } else if (!win || win->IsDocLoaded() && !forceReuse) {
        WindowInfo *currWin = win;
        win = CreateWindowInfo();
        if (!win)
            return NULL;
        isNewWindow = true;
        if (currWin) {
            RememberFavTreeExpansionState(currWin);
            win->expandedFavorites = currWin->expandedFavorites;
        }
    }

    DeleteOldSelectionInfo(win, true);
    win->fwdSearchMark.show = false;
    win->notifications->RemoveAllInGroup(NG_RESPONSE_TO_ACTION);
    win->notifications->RemoveAllInGroup(NG_PAGE_INFO_HELPER);

    win->suppressPwdUI = suppressPwdUI;
    if (!LoadDocIntoWindow(fullpath, *win, NULL, isNewWindow,
                           true /* allowFailure */, showWin, true /* placeWindow */)) {
        /* failed to open */
        if (gFileHistory.MarkFileInexistent(fullpath))
            SavePrefs();
        return win;
    }

    if (!win->watcher)
        win->watcher = new FileWatcher(new FileChangeCallback(win));
    win->watcher->Init(fullpath);
#ifdef THREAD_BASED_FILEWATCH
    win->watcher->StartWatchThread();
#endif

    if (gGlobalPrefs.rememberOpenedFiles) {
        assert(str::Eq(fullpath, win->loadedFilePath));
        gFileHistory.MarkFileLoaded(fullpath);
        if (gGlobalPrefs.showStartPage)
            CreateThumbnailForFile(*win, *gFileHistory.Get(0));
        SavePrefs();
    }

    // Add the file also to Windows' recently used documents (this doesn't
    // happen automatically on drag&drop, reopening from history, etc.)
    if (HasPermission(Perm_DiskAccess) && !gPluginMode)
        SHAddToRecentDocs(SHARD_PATH, fullpath);

    return win;
}

// The current page edit box is updated with the current page number
void WindowInfo::PageNoChanged(int pageNo)
{
    assert(dm && dm->PageCount() > 0);
    if (!dm || dm->PageCount() == 0)
        return;

    if (INVALID_PAGE_NO != pageNo) {
        ScopedMem<TCHAR> buf(dm->engine->GetPageLabel(pageNo));
        win::SetText(hwndPageBox, buf);
        ToolbarUpdateStateForWindow(this, false);
#ifdef BUILD_RIBBON
        if (ribbonSupport)
            ribbonSupport->UpdateState();
#endif
        if (dm->engine && dm->engine->HasPageLabels())
            UpdateToolbarPageText(this, dm->PageCount(), true);
    }
    if (pageNo == currPageNo)
        return;

    UpdateTocSelection(this, pageNo);
    currPageNo = pageNo;

    NotificationWnd *wnd = notifications->GetFirstInGroup(NG_PAGE_INFO_HELPER);
    if (NULL == wnd)
        return;

    ScopedMem<TCHAR> pageInfo(str::Format(_T("%s %d / %d"), _TR("Page:"), pageNo, dm->PageCount()));
    if (dm->engine && dm->engine->HasPageLabels()) {
        ScopedMem<TCHAR> label(dm->engine->GetPageLabel(pageNo));
        pageInfo.Set(str::Format(_T("%s %s (%d / %d)"), _TR("Page:"), label, pageNo, dm->PageCount()));
    }
    wnd->UpdateMessage(pageInfo);
}

bool DoCachePageRendering(WindowInfo *win, int pageNo)
{
    assert(win->dm && win->dm->engine);
    if (!win->dm || !win->dm->engine || !win->dm->engine->IsImageCollection())
        return true;

    // cache large images (mainly photos), as shrinking them
    // for every UI update (WM_PAINT) can cause notable lags
    // TODO: stretching small images also causes minor lags
    RectD page = win->dm->engine->PageMediabox(pageNo);
    return page.dx * page.dy > 1024 * 1024;
}

/* Send the request to render a given page to a rendering thread */
void WindowInfo::RenderPage(int pageNo, bool bForceRender)
{
    assert(dm);
    if (!dm)
        return;
    // don't render any plain images on the rendering thread,
    // they'll be rendered directly in DrawDocument during
    // WM_PAINT on the UI thread
    if (!DoCachePageRendering(this, pageNo))
        return;

    gRenderCache.Render(dm, pageNo, bForceRender, NULL);
}

void WindowInfo::CleanUp(DisplayModel *dm)
{
    assert(dm);
    if (!dm)
        return;

    gRenderCache.CancelRendering(dm);
    gRenderCache.FreeForDisplayModel(dm);
}

static void UpdateCanvasScrollbars(DisplayModel *dm, HWND hwndCanvas, SizeI canvas)
{
    SCROLLINFO si = { 0 };
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;

    SizeI viewPort = dm->viewPort.Size();

    if (viewPort.dx >= canvas.dx) {
        si.nPos = 0;
        si.nMin = 0;
        si.nMax = 99;
        si.nPage = 100;
    } else {
        si.nPos = dm->viewPort.x;
        si.nMin = 0;
        si.nMax = canvas.dx - 1;
        si.nPage = viewPort.dx;
    }
    ShowScrollBar(hwndCanvas, SB_HORZ, viewPort.dx < canvas.dx);
    SetScrollInfo(hwndCanvas, SB_HORZ, &si, TRUE);

    if (viewPort.dy >= canvas.dy) {
        si.nPos = 0;
        si.nMin = 0;
        si.nMax = 99;
        si.nPage = 100;
    } else {
        si.nPos = dm->viewPort.y;
        si.nMin = 0;
        si.nMax = canvas.dy - 1;
        si.nPage = viewPort.dy;

        if (ZOOM_FIT_PAGE != dm->ZoomVirtual()) {
            // keep the top/bottom 5% of the previous page visible after paging down/up
            si.nPage = (UINT)(si.nPage * 0.95);
            si.nMax -= viewPort.dy - si.nPage;
        }
    }
    ShowScrollBar(hwndCanvas, SB_VERT, viewPort.dy < canvas.dy);
    SetScrollInfo(hwndCanvas, SB_VERT, &si, TRUE);
}

void WindowInfo::UpdateScrollbars(SizeI canvas)
{
    UpdateCanvasScrollbars(dm, hwndCanvas, canvas);
}

void AssociateExeWithPdfExtension()
{
    if (!HasPermission(Perm_RegistryAccess)) return;

    DoAssociateExeWithPdfExtension(HKEY_CURRENT_USER);
    DoAssociateExeWithPdfExtension(HKEY_LOCAL_MACHINE);

    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST | SHCNF_FLUSHNOWAIT, 0, 0);

    // Remind the user, when a different application takes over
    gGlobalPrefs.pdfAssociateShouldAssociate = true;
    gGlobalPrefs.pdfAssociateDontAskAgain = false;
}

// Registering happens either through the Installer or the Options dialog;
// here we just make sure that we're still registered
static bool RegisterForPdfExtentions(HWND hwnd)
{
    if (IsRunningInPortableMode() || !HasPermission(Perm_RegistryAccess) || gPluginMode)
        return false;

    if (IsExeAssociatedWithPdfExtension())
        return true;

    /* Ask user for permission, unless he previously said he doesn't want to
       see this dialog */
    if (!gGlobalPrefs.pdfAssociateDontAskAgain) {
        INT_PTR result = Dialog_PdfAssociate(hwnd, &gGlobalPrefs.pdfAssociateDontAskAgain);
        assert(IDYES == result || IDNO == result);
        gGlobalPrefs.pdfAssociateShouldAssociate = (IDYES == result);
    }
    if (!gGlobalPrefs.pdfAssociateShouldAssociate)
        return false;

    AssociateExeWithPdfExtension();
    return true;
}

static void OnDropFiles(HDROP hDrop)
{
    TCHAR       filename[MAX_PATH];
    const int   count = DragQueryFile(hDrop, DRAGQUERY_NUMFILES, 0, 0);

    for (int i = 0; i < count; i++)
    {
        DragQueryFile(hDrop, i, filename, dimof(filename));
        if (str::EndsWithI(filename, _T(".lnk"))) {
            ScopedMem<TCHAR> resolved(ResolveLnk(filename));
            if (resolved)
                str::BufSet(filename, dimof(filename), resolved);
        }
        // The first dropped document may override the current window
        LoadDocument(filename);
    }
    DragFinish(hDrop);
}

static DWORD OnUrlDownloaded(HWND hParent, HttpReqCtx *ctx, bool silent)
{
    if (ctx->error)
        return ctx->error;
    if (!str::StartsWith(ctx->url, SUMATRA_UPDATE_INFO_URL))
        return ERROR_INTERNET_INVALID_URL;

    // See http://code.google.com/p/sumatrapdf/issues/detail?id=725
    // If a user configures os-wide proxy that is not regular ie proxy
    // (which we pick up) we might get complete garbage in response to
    // our query and it might accidentally contain a number bigger than
    // our version number which will make us ask to upgrade every time.
    // To fix that, we reject text that doesn't look like a valid version number.
    ScopedMem<char> txt(ctx->data->StealData());
    if (!IsValidProgramVersion(txt))
        return ERROR_INTERNET_INVALID_URL;

    ScopedMem<TCHAR> verTxt(str::conv::FromAnsi(txt));
    /* reduce the string to a single line (resp. drop the newline) */
    str::TransChars(verTxt, _T("\r\n"), _T("\0\0"));
    if (CompareVersion(verTxt, UPDATE_CHECK_VER) <= 0) {
        /* if automated => don't notify that there is no new version */
        if (!silent) {
            MessageBox(hParent, _TR("You have the latest version."),
                       _TR("SumatraPDF Update"), MB_ICONINFORMATION | MB_OK | (IsUIRightToLeft() ? MB_RTLREADING : 0));
        }
        return 0;
    }

    // if automated, respect gGlobalPrefs.versionToSkip
    if (silent && str::EqI(gGlobalPrefs.versionToSkip, verTxt))
        return 0;

    // ask whether to download the new version and allow the user to
    // either open the browser, do nothing or don't be reminded of
    // this update ever again
    bool skipThisVersion = false;
    INT_PTR res = Dialog_NewVersionAvailable(hParent, UPDATE_CHECK_VER, verTxt, &skipThisVersion);
    if (skipThisVersion)
        str::ReplacePtr(&gGlobalPrefs.versionToSkip, verTxt);
    if (IDYES == res)
        LaunchBrowser(SVN_UPDATE_LINK);
    SavePrefs();

    return 0;
}

class UpdateDownloadWorkItem : public UIThreadWorkItem, public HttpReqCallback
{
    bool autoCheck;
    HttpReqCtx *ctx;

public:
    UpdateDownloadWorkItem(WindowInfo *win, bool autoCheck) :
        UIThreadWorkItem(win), autoCheck(autoCheck), ctx(NULL) { }

    virtual void Callback(HttpReqCtx *ctx) {
        this->ctx = ctx;
        QueueWorkItem(this);
    }

    virtual void Execute() {
        if (WindowInfoStillValid(win) && ctx) {
            DWORD error = OnUrlDownloaded(win->hwndFrame, ctx, autoCheck);
            if (error && !autoCheck) {
                // notify the user about the error during a manual update check
                ScopedMem<TCHAR> msg(str::Format(_TR("Can't connect to the Internet (error %#x)."), error));
                MessageBox(win->hwndFrame, msg, _TR("SumatraPDF Update"), MB_ICONEXCLAMATION | MB_OK | (IsUIRightToLeft() ? MB_RTLREADING : 0));
            }
        }
        delete ctx;
    }
};

static void DownloadSumatraUpdateInfo(WindowInfo& win, bool autoCheck)
{
    if (!HasPermission(Perm_InternetAccess) || gPluginMode)
        return;

    // don't check for updates at the first start, so that privacy
    // sensitive users can disable the update check in time
    if (autoCheck && !gGlobalPrefs.lastUpdateTime) {
        FILETIME lastUpdateTimeFt = { 0 };
        gGlobalPrefs.lastUpdateTime =_MemToHex(&lastUpdateTimeFt);
        return;
    }

    /* For auto-check, only check if at least a day passed since last check */
    if (autoCheck && gGlobalPrefs.lastUpdateTime) {
        FILETIME lastUpdateTimeFt, currentTimeFt;
        _HexToMem(gGlobalPrefs.lastUpdateTime, &lastUpdateTimeFt);
        GetSystemTimeAsFileTime(&currentTimeFt);
        int secs = FileTimeDiffInSecs(currentTimeFt, lastUpdateTimeFt);
        assert(secs >= 0);
        // if secs < 0 => somethings wrong, so ignore that case
        if ((secs > 0) && (secs < SECS_IN_DAY))
            return;
    }

    const TCHAR *url = SUMATRA_UPDATE_INFO_URL _T("?v=") UPDATE_CHECK_VER;
    new HttpReqCtx(url, new UpdateDownloadWorkItem(&win, autoCheck));

    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    free(gGlobalPrefs.lastUpdateTime);
    gGlobalPrefs.lastUpdateTime = _MemToHex(&ft);
}

#ifdef DRAW_PAGE_SHADOWS
#define BORDER_SIZE   1
#define SHADOW_OFFSET 4
static void PaintPageFrameAndShadow(HDC hdc, RectI& bounds, RectI& pageRect, bool presentation)
{
    // Frame info
    RectI frame = bounds;
    frame.Inflate(BORDER_SIZE, BORDER_SIZE);

    // Shadow info
    RectI shadow = frame;
    shadow.Offset(SHADOW_OFFSET, SHADOW_OFFSET);
    if (frame.x < 0) {
        // the left of the page isn't visible, so start the shadow at the left
        int diff = min(-pageRect.x, SHADOW_OFFSET);
        shadow.x -= diff; shadow.dx += diff;
    }
    if (frame.y < 0) {
        // the top of the page isn't visible, so start the shadow at the top
        int diff = min(-pageRect.y, SHADOW_OFFSET);
        shadow.y -= diff; shadow.dy += diff;
    }

    // Draw shadow
    if (!presentation) {
        ScopedGdiObj<HBRUSH> brush(CreateSolidBrush(COL_PAGE_SHADOW));
        FillRect(hdc, &shadow.ToRECT(), brush);
    }

    // Draw frame
    ScopedGdiObj<HPEN> pe(CreatePen(PS_SOLID, 1, presentation ? TRANSPARENT : COL_PAGE_FRAME));
    SelectObject(hdc, pe);
    SelectObject(hdc, gRenderCache.invertColors ? gBrushBlack : gBrushWhite);
    Rectangle(hdc, frame.x, frame.y, frame.x + frame.dx, frame.y + frame.dy);
}
#else
static void PaintPageFrameAndShadow(HDC hdc, RectI& bounds, RectI& pageRect, bool presentation)
{
    ScopedGdiObj<HPEN> pe(CreatePen(PS_NULL, 0, 0));
    SelectObject(hdc, pe);
    SelectObject(hdc, gRenderCache.invertColors ? gBrushBlack : gBrushWhite);
    Rectangle(hdc, bounds.x, bounds.y, bounds.x + bounds.dx + 1, bounds.y + bounds.dy + 1);
}
#endif

/* debug code to visualize links (can block while rendering) */
static void DebugShowLinks(DisplayModel& dm, HDC hdc)
{
    if (!gDebugShowLinks)
        return;

    RectI viewPortRect(PointI(), dm.viewPort.Size());
    HPEN pen = CreatePen(PS_SOLID, 1, RGB(0x00, 0xff, 0xff));
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    for (int pageNo = dm.PageCount(); pageNo >= 1; --pageNo) {
        PageInfo *pageInfo = dm.GetPageInfo(pageNo);
        if (!pageInfo || !pageInfo->shown || 0.0 == pageInfo->visibleRatio)
            continue;

        Vec<PageElement *> *els = dm.engine->GetElements(pageNo);
        if (els) {
            for (size_t i = 0; i < els->Count(); i++) {
                RectI rect = dm.CvtToScreen(pageNo, els->At(i)->GetRect());
                RectI isect = viewPortRect.Intersect(rect);
                if (!isect.IsEmpty())
                    PaintRect(hdc, isect);
            }
            DeleteVecMembers(*els);
            delete els;
        }
    }

    DeletePen(SelectObject(hdc, oldPen));

    if (dm.ZoomVirtual() == ZOOM_FIT_CONTENT) {
        // also display the content box when fitting content
        pen = CreatePen(PS_SOLID, 1, RGB(0xff, 0x00, 0xff));
        oldPen = SelectObject(hdc, pen);

        for (int pageNo = dm.PageCount(); pageNo >= 1; --pageNo) {
            PageInfo *pageInfo = dm.GetPageInfo(pageNo);
            if (!pageInfo->shown || 0.0 == pageInfo->visibleRatio)
                continue;

            RectI rect = dm.CvtToScreen(pageNo, dm.engine->PageContentBox(pageNo));
            PaintRect(hdc, rect);
        }

        DeletePen(SelectObject(hdc, oldPen));
    }
}

static void DrawDocument(WindowInfo& win, HDC hdc, RECT *rcArea)
{
    DisplayModel* dm = win.dm;
    assert(dm);
    if (!dm) return;

    bool paintOnBlackWithoutShadow = win.presentation ||
    // draw comic books and single images on a black background (without frame and shadow)
                                     dm->engine && dm->engine->IsImageCollection();
    if (paintOnBlackWithoutShadow)
        FillRect(hdc, rcArea, gBrushBlack);
    else
        FillRect(hdc, rcArea, gBrushNoDocBg);

    bool rendering = false;
    RectI screen(PointI(), dm->viewPort.Size());

    DBG_OUT("DrawDocument() ");
    for (int pageNo = 1; pageNo <= dm->PageCount(); ++pageNo) {
        PageInfo *pageInfo = dm->GetPageInfo(pageNo);
        if (!pageInfo || 0.0f == pageInfo->visibleRatio)
            continue;
        assert(pageInfo->shown);
        if (!pageInfo->shown)
            continue;

        RectI bounds = pageInfo->pageOnScreen.Intersect(screen);
        // don't paint the frame background for images
        if (!(dm->engine && dm->engine->IsImageCollection()))
            PaintPageFrameAndShadow(hdc, bounds, pageInfo->pageOnScreen, win.presentation);

        bool renderOutOfDateCue = false;
        UINT renderDelay = 0;
        if (!DoCachePageRendering(&win, pageNo))
            dm->engine->RenderPage(hdc, pageInfo->pageOnScreen, pageNo, dm->ZoomReal(pageNo), dm->Rotation());
        else
            renderDelay = gRenderCache.Paint(hdc, &bounds, dm, pageNo, pageInfo, &renderOutOfDateCue);

		/*MyCode*/
		if(g_pIntf)
		{
			RectI rt = pageInfo->pageOnScreen;
			RECT rtDraw;
			rtDraw.left = rt.x;
			rtDraw.top = rt.y;
			rtDraw.right = rt.x + rt.dx;
			rtDraw.bottom = rt.y + rt.dy;
			g_pIntf->AfterDrawPage(pageNo, win.buffer->GetDC(), rtDraw);
		}

        if (renderDelay) {
            ScopedFont fontRightTxt(GetSimpleFont(hdc, _T("MS Shell Dlg"), 14));
            HGDIOBJ hPrevFont = SelectObject(hdc, fontRightTxt);
            SetTextColor(hdc, gRenderCache.invertColors ? WIN_COL_WHITE : WIN_COL_BLACK);
            if (renderDelay != RENDER_DELAY_FAILED) {
                if (renderDelay < REPAINT_MESSAGE_DELAY_IN_MS)
                    win.RepaintAsync(REPAINT_MESSAGE_DELAY_IN_MS / 4);
                else
                    DrawCenteredText(hdc, bounds, _TR("Please wait - rendering..."), IsUIRightToLeft());
                DBG_OUT("drawing empty %d ", pageNo);
                rendering = true;
            } else {
                DrawCenteredText(hdc, bounds, _TR("Couldn't render the page"), IsUIRightToLeft());
                DBG_OUT("   missing bitmap on visible page %d\n", pageNo);
            }
            SelectObject(hdc, hPrevFont);
            continue;
        }

        if (!renderOutOfDateCue)
            continue;

        HDC bmpDC = CreateCompatibleDC(hdc);
        if (bmpDC) {
            SelectObject(bmpDC, gBitmapReloadingCue);
            int size = (int)(16 * win.uiDPIFactor);
            int cx = min(bounds.dx, 2 * size), cy = min(bounds.dy, 2 * size);
            StretchBlt(hdc, bounds.x + bounds.dx - min((cx + size) / 2, cx),
                bounds.y + max((cy - size) / 2, 0), min(cx, size), min(cy, size),
                bmpDC, 0, 0, 16, 16, SRCCOPY);

            DeleteDC(bmpDC);
        }
    }

    if (win.showSelection)
        PaintSelection(&win, hdc);

    if (win.fwdSearchMark.show)
        PaintForwardSearchMark(&win, hdc);

    DBG_OUT("\n");
    if (!rendering)
        DebugShowLinks(*dm, hdc);
}

static void ToggleGdiDebugging()
{
    gUseGdiRenderer = !gUseGdiRenderer;
    DebugGdiPlusDevice(gUseGdiRenderer);

    for (size_t i = 0; i < gWindows.Count(); i++) {
        DisplayModel *dm = gWindows.At(i)->dm;
        if (dm) {
            gRenderCache.CancelRendering(dm);
            gRenderCache.KeepForDisplayModel(dm, dm);
            gWindows.At(i)->RedrawAll(true);
        }
#ifdef BUILD_RIBBON
        if (gWindows.At(i)->ribbonSupport)
            gWindows.At(i)->ribbonSupport->UpdateState();
#endif
    }
}

static void OnDraggingStart(WindowInfo& win, int x, int y, bool right=false)
{
    SetCapture(win.hwndCanvas);
    win.mouseAction = right ? MA_DRAGGING_RIGHT : MA_DRAGGING;
    win.dragPrevPos = PointI(x, y);
    if (GetCursor())
        SetCursor(gCursorDrag);
    DBG_OUT(" dragging start, x=%d, y=%d\n", x, y);
}

static void OnDraggingStop(WindowInfo& win, int x, int y, bool aborted)
{
    if (GetCapture() != win.hwndCanvas)
        return;

    if (GetCursor())
        SetCursor(gCursorArrow);
    ReleaseCapture();

    if (aborted)
        return;

    SizeI drag(x - win.dragPrevPos.x, y - win.dragPrevPos.y);
    DBG_OUT(" dragging ends, x=%d, y=%d, dx=%d, dy=%d\n", x, y, drag.dx, drag.dy);
    win.MoveDocBy(drag.dx, -2 * drag.dy);
}

static void OnMouseMove(WindowInfo& win, int x, int y, WPARAM flags)
{
    if (!win.IsDocLoaded())
        return;
    assert(win.dm);

	if(g_pIntf)
	{
		if(g_pIntf->OnMouseMove(x,y,flags))
			return;
	}

    if (win.presentation) {
        // shortly display the cursor if the mouse has moved and the cursor is hidden
        if (PointI(x, y) != win.dragPrevPos && !GetCursor()) {
            if (win.mouseAction == MA_IDLE)
                SetCursor(gCursorArrow);
            else
                SendMessage(win.hwndCanvas, WM_SETCURSOR, 0, 0);
            SetTimer(win.hwndCanvas, HIDE_CURSOR_TIMER_ID, HIDE_CURSOR_DELAY_IN_MS, NULL);
        }
    }

    if (win.dragStartPending) {
        // have we already started a proper drag?
        if (abs(x - win.dragStart.x) <= GetSystemMetrics(SM_CXDRAG) &&
            abs(y - win.dragStart.y) <= GetSystemMetrics(SM_CYDRAG)) {
            return;
        }
        win.dragStartPending = false;
        delete win.linkOnLastButtonDown;
        win.linkOnLastButtonDown = NULL;
    }

    SizeI drag;
    switch (win.mouseAction) {
    case MA_SCROLLING:
        win.yScrollSpeed = (y - win.dragStart.y) / SMOOTHSCROLL_SLOW_DOWN_FACTOR;
        win.xScrollSpeed = (x - win.dragStart.x) / SMOOTHSCROLL_SLOW_DOWN_FACTOR;
        break;
    case MA_SELECTING_TEXT:
        if (GetCursor())
            SetCursor(gCursorIBeam);
        /* fall through */
    case MA_SELECTING:
        win.selectionRect.dx = x - win.selectionRect.x;
        win.selectionRect.dy = y - win.selectionRect.y;
        win.RepaintAsync();
        OnSelectionEdgeAutoscroll(&win, x, y);
        break;
    case MA_DRAGGING:
    case MA_DRAGGING_RIGHT:
        drag = SizeI(win.dragPrevPos.x - x, win.dragPrevPos.y - y);
        DBG_OUT(" drag move, x=%d, y=%d, dx=%d, dy=%d\n", x, y, drag.dx, drag.dy);
        win.MoveDocBy(drag.dx, drag.dy);
        break;
    }

    win.dragPrevPos = PointI(x, y);
}

static void OnMouseLeftButtonDown(WindowInfo& win, int x, int y, WPARAM key)
{
    //DBG_OUT("Left button clicked on %d %d\n", x, y);
    if (win.IsAboutWindow())
        // remember a link under so that on mouse up we only activate
        // link if mouse up is on the same link as mouse down
        win.url = GetStaticLink(win.staticLinks, x, y);
    if (!win.IsDocLoaded())
        return;

    if (MA_DRAGGING_RIGHT == win.mouseAction)
        return;

    if (MA_SCROLLING == win.mouseAction) {
        win.mouseAction = MA_IDLE;
        return;
    }
    assert(win.mouseAction == MA_IDLE);
    assert(win.dm);

    SetFocus(win.hwndFrame);

	/*MyCode*/
	if(g_pIntf)
	{
		if(g_pIntf->OnMouseLeftButtonDown(x,y,key))
			return;
	}

    assert(!win.linkOnLastButtonDown);
    PageElement *pageEl = win.dm->GetElementAtPos(PointI(x, y));
    if (pageEl && pageEl->AsLink())
        win.linkOnLastButtonDown = pageEl;
    else
        delete pageEl;
    win.dragStartPending = true;
    win.dragStart = PointI(x, y);

    // - without modifiers, clicking on text starts a text selection
    //   and clicking somewhere else starts a drag
    // - pressing Shift forces dragging
    // - pressing Ctrl forces a rectangular selection
    // - pressing Ctrl+Shift forces text selection
    // - not having CopySelection permission forces dragging
    if (!HasPermission(Perm_CopySelection) || ((key & MK_SHIFT) || !win.dm->IsOverText(PointI(x, y))) && !(key & MK_CONTROL))
        OnDraggingStart(win, x, y);
    else
        OnSelectionStart(&win, x, y, key);	
}

static void OnMouseLeftButtonUp(WindowInfo& win, int x, int y, WPARAM key)
{
    if (win.IsAboutWindow()) {
        SetFocus(win.hwndFrame);
        const TCHAR *url = GetStaticLink(win.staticLinks, x, y);
        if (url && url == win.url) {
            if (str::Eq(url, SLINK_OPEN_FILE))
                SendMessage(win.hwndFrame, WM_COMMAND, IDM_OPEN, 0);
            else if (str::Eq(url, SLINK_LIST_HIDE)) {
                gGlobalPrefs.showStartPage = false;
                win.RedrawAll(true);
            } else if (str::Eq(url, SLINK_LIST_SHOW)) {
                gGlobalPrefs.showStartPage = true;
                win.RedrawAll(true);
            } else if (!str::StartsWithI(url, _T("http:")) &&
                       !str::StartsWithI(url, _T("https:")) &&
                       !str::StartsWithI(url, _T("mailto:"))) {
                LoadDocument(url, &win);
            } else
                LaunchBrowser(url);
        }
        win.url = NULL;
    }
    if (!win.IsDocLoaded())
        return;

	/*MyCode*/
	if(g_pIntf)
	{
		if(g_pIntf->OnMouseLeftButtonUp(x,y,key))
			return;
	}
	//////////////////////////////////////////////////////////////////////////

    assert(win.dm);
    if (MA_IDLE == win.mouseAction || MA_DRAGGING_RIGHT == win.mouseAction)
        return;
    assert(MA_SELECTING == win.mouseAction || MA_SELECTING_TEXT == win.mouseAction || MA_DRAGGING == win.mouseAction);	

    bool didDragMouse = !win.dragStartPending ||
        abs(x - win.dragStart.x) > GetSystemMetrics(SM_CXDRAG) ||
        abs(y - win.dragStart.y) > GetSystemMetrics(SM_CYDRAG);
    if (MA_DRAGGING == win.mouseAction)
        OnDraggingStop(win, x, y, !didDragMouse);
    else
        OnSelectionStop(&win, x, y, !didDragMouse);

    PointD ptPage = win.dm->CvtFromScreen(PointI(x, y));

    if (didDragMouse)
        /* pass */;
    else if (win.linkOnLastButtonDown && win.linkOnLastButtonDown->GetRect().Inside(ptPage)) {
        win.linkHandler->GotoLink(win.linkOnLastButtonDown->AsLink());
        SetCursor(gCursorArrow);
    }
    /* if we had a selection and this was just a click, hide the selection */
    else if (win.showSelection)
        ClearSearchResult(&win);
    /* in presentation mode, change pages on left/right-clicks */
    else if (win.fullScreen || PM_ENABLED == win.presentation) {
        if ((key & MK_SHIFT))
            win.dm->GoToPrevPage(0);
        else
            win.dm->GoToNextPage(0);
    }
    /* return from white/black screens in presentation mode */
    else if (PM_BLACK_SCREEN == win.presentation || PM_WHITE_SCREEN == win.presentation)
        win.ChangePresentationMode(PM_ENABLED);

    win.mouseAction = MA_IDLE;
    delete win.linkOnLastButtonDown;
    win.linkOnLastButtonDown = NULL;
}

static void OnMouseLeftButtonDblClk(WindowInfo& win, int x, int y, WPARAM key)
{
    //DBG_OUT("Left button clicked on %d %d\n", x, y);
    if ((win.fullScreen || win.presentation) && !(key & ~MK_LBUTTON) || win.IsAboutWindow()) {
        // in presentation and fullscreen modes, left clicks turn the page,
        // make two quick left clicks (AKA one double-click) turn two pages
        OnMouseLeftButtonDown(win, x, y, key);
        return;
    }

	/*MyCode*/
	if(g_pIntf)
	{
		if(g_pIntf->OnMouseLeftButtonDblClk(x,y,key))
			return;
	}
	//////////////////////////////////////////////////////////////////////////

    bool dontSelect = false;
    if (gGlobalPrefs.enableTeXEnhancements && !(key & ~MK_LBUTTON))
        dontSelect = OnInverseSearch(&win, x, y);

    if (dontSelect || !win.IsDocLoaded() || !win.dm->IsOverText(PointI(x, y)))
        return;

    int pageNo = win.dm->GetPageNoByPoint(PointI(x, y));
    if (win.dm->ValidPageNo(pageNo)) {
        PointD pt = win.dm->CvtFromScreen(PointI(x, y), pageNo);
        win.dm->textSelection->SelectWordAt(pageNo, pt.x, pt.y);
    }

    UpdateTextSelection(&win, false);	

    win.RepaintAsync();
}

static void OnMouseMiddleButtonDown(WindowInfo& win, int x, int y, WPARAM key)
{
    // Handle message by recording placement then moving document as mouse moves.

    switch (win.mouseAction) {
    case MA_IDLE:
        win.mouseAction = MA_SCROLLING;

        // record current mouse position, the farther the mouse is moved
        // from this position, the faster we scroll the document
        win.dragStart = PointI(x, y);
        SetCursor(gCursorScroll);
        break;

    case MA_SCROLLING:
        win.mouseAction = MA_IDLE;
        break;
    }
}

static void OnMouseRightButtonDown(WindowInfo& win, int x, int y, WPARAM key)
{
    //DBG_OUT("Right button clicked on %d %d\n", x, y);
    if (!win.IsDocLoaded()) {
        SetFocus(win.hwndFrame);
        win.dragStart = PointI(x, y);
        return;
    }

    if (MA_SCROLLING == win.mouseAction)
        win.mouseAction = MA_IDLE;
    else if (win.mouseAction != MA_IDLE)
        return;
    assert(win.dm);

    SetFocus(win.hwndFrame);

    win.dragStartPending = true;
    win.dragStart = PointI(x, y);

    OnDraggingStart(win, x, y, true);
}

static void OnMouseRightButtonUp(WindowInfo& win, int x, int y, WPARAM key)
{
    if (!win.IsDocLoaded()) {
        bool didDragMouse =
            abs(x - win.dragStart.x) > GetSystemMetrics(SM_CXDRAG) ||
            abs(y - win.dragStart.y) > GetSystemMetrics(SM_CYDRAG);
        if (!didDragMouse)
            OnAboutContextMenu(&win, x, y);
        return;
    }

    assert(win.dm);
    if (MA_DRAGGING_RIGHT != win.mouseAction)
        return;

    bool didDragMouse = !win.dragStartPending ||
        abs(x - win.dragStart.x) > GetSystemMetrics(SM_CXDRAG) ||
        abs(y - win.dragStart.y) > GetSystemMetrics(SM_CYDRAG);
    OnDraggingStop(win, x, y, !didDragMouse);

    if (didDragMouse)
        /* pass */;
    else if (win.fullScreen || PM_ENABLED == win.presentation) {
        if ((key & MK_CONTROL))
            OnContextMenu(&win, x, y);
        else if ((key & MK_SHIFT))
            win.dm->GoToNextPage(0);
        else
            win.dm->GoToPrevPage(0);
    }
    /* return from white/black screens in presentation mode */
    else if (PM_BLACK_SCREEN == win.presentation || PM_WHITE_SCREEN == win.presentation)
        win.ChangePresentationMode(PM_ENABLED);
    else
        OnContextMenu(&win, x, y);

    win.mouseAction = MA_IDLE;
}

static void OnMouseRightButtonDblClick(WindowInfo& win, int x, int y, WPARAM key)
{
    if ((win.fullScreen || win.presentation) && !(key & ~MK_RBUTTON)) {
        // in presentation and fullscreen modes, right clicks turn the page,
        // make two quick right clicks (AKA one double-click) turn two pages
        OnMouseRightButtonDown(win, x, y, key);
        return;
    }
}

static void OnPaint(WindowInfo& win)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(win.hwndCanvas, &ps);

    if (win.IsAboutWindow()) {
        if (HasPermission(Perm_SavePreferences | Perm_DiskAccess) && gGlobalPrefs.rememberOpenedFiles && gGlobalPrefs.showStartPage)
            DrawStartPage(win, win.buffer->GetDC(), gFileHistory, gRenderCache.invertColors);
        else
            DrawAboutPage(win, win.buffer->GetDC());
        win.buffer->Flush(hdc);
    } else if (!win.IsDocLoaded()) {
        // TODO: replace with notifications as far as reasonably possible
        ScopedFont fontRightTxt(GetSimpleFont(hdc, _T("MS Shell Dlg"), 14));
        HGDIOBJ hPrevFont = SelectObject(hdc, fontRightTxt);
        SetBkMode(hdc, TRANSPARENT);
        FillRect(hdc, &ps.rcPaint, gBrushNoDocBg);
        ScopedMem<TCHAR> msg(str::Format(_TR("Error loading %s"), win.loadedFilePath));
        DrawCenteredText(hdc, ClientRect(win.hwndCanvas), msg, IsUIRightToLeft());
        SelectObject(hdc, hPrevFont);
    } else {
        switch (win.presentation) {
        case PM_BLACK_SCREEN:
            FillRect(hdc, &ps.rcPaint, gBrushBlack);
            break;
        case PM_WHITE_SCREEN:
            FillRect(hdc, &ps.rcPaint, gBrushWhite);
            break;
        default:
            DrawDocument(win, win.buffer->GetDC(), &ps.rcPaint);

			if(g_pIntf)
				g_pIntf->BeforePaintFlush(hdc);

            win.buffer->Flush(hdc);
        }
    }

    EndPaint(win.hwndCanvas, &ps);
}

static void OnMenuExit()
{
    if (gPluginMode)
        return;

    for (size_t i = 0; i < gWindows.Count(); i++) {
        WindowInfo *win = gWindows.At(i);
        if (win->IsChm() && InHtmlNestedMessagePump()) {
            return;
        }
        AbortFinding(win);
        AbortPrinting(win);
    }

    SavePrefs();
    PostQuitMessage(0);
}

// Note: not sure if this is a good idea, if gWindows or its WindowInfo objects
// are corrupted, we might crash trying to access them. Maybe use IsBadReadPtr()
// to test pointers (even if it's not advised in general)
void GetFilesInfo(str::Str<char>& s)
{
    // only add paths to files encountered during an explicit stress test
    // (for privacy reasons, users should be able to decide themselves
    // whether they want to share what files they had opened during a crash)
    if (!gIsStressTesting)
        return;

    for (size_t i = 0; i < gWindows.Count(); i++) {
        WindowInfo *w = gWindows.At(i);
        if (!w || !w->dm || !w->loadedFilePath)
            continue;

        s.Append("File: ");
        s.AppendAndFree(str::conv::ToUtf8(w->loadedFilePath));
        s.AppendAndFree(GetStressTestInfo(w->stressTest));
        s.Append("\r\n");
    }
}

/* Close the document associated with window 'hwnd'.
   Closes the window unless this is the last window in which
   case it switches to empty window and disables the "File\Close"
   menu item. */
void CloseWindow(WindowInfo *win, bool quitIfLast, bool forceClose)
{
    assert(win);
    if (!win) return;
    // when used as an embedded plugin, closing should happen automatically
    // when the parent window is destroyed (cf. WM_DESTROY)
    if (gPluginMode && !forceClose)
        return;

    bool wasChm = win->IsChm();
    if (wasChm && InHtmlNestedMessagePump()) {
        return;
    }

    if (win->IsDocLoaded())
        win->dm->dontRenderFlag = true;
    if (win->presentation)
        ExitFullscreen(*win);

    bool lastWindow = (1 == gWindows.Count());
    if (lastWindow) {
#ifdef BUILD_RIBBON
        if (gWindows.At(0)->ribbonSupport) {
            free(gGlobalPrefs.ribbonState);
            gGlobalPrefs.ribbonState = gWindows.At(0)->ribbonSupport->GetState();
        }
#endif
        SavePrefs();
    } else {
        UpdateCurrentFileDisplayStateForWin(win);
    }

    if (lastWindow && !quitIfLast) {
        if (wasChm)
            UnsubclassCanvas(win->hwndCanvas);
        /* last window - don't delete it */
        delete win->watcher;
        win->watcher = NULL;
        SetSidebarVisibility(win, false, gGlobalPrefs.favVisible);
        ClearTocBox(win);
        AbortFinding(win, true);
        delete win->dm;
        win->dm = NULL;
        str::ReplacePtr(&win->loadedFilePath, NULL);
        delete win->pdfsync;
        win->pdfsync = NULL;
        win->notifications->RemoveAllInGroup(NG_RESPONSE_TO_ACTION);
        win->notifications->RemoveAllInGroup(NG_PAGE_INFO_HELPER);

        if (win->hwndProperties) {
            DestroyWindow(win->hwndProperties);
            assert(!win->hwndProperties);
        }
        UpdateToolbarPageText(win, 0);
        UpdateToolbarFindText(win);
        if (wasChm)
            // restore the non-Chm menu
            RebuildMenuBarForWindow(win);
#ifdef BUILD_RIBBON
        if (win->ribbonSupport)
            win->ribbonSupport->UpdateState();
#endif
        DeleteOldSelectionInfo(win, true);
        win->RedrawAll();
        UpdateFindbox(win);
        SetFocus(win->hwndFrame);
    } else {
        HWND hwndToDestroy = win->hwndFrame;
        DeleteWindowInfo(win);
        DestroyWindow(hwndToDestroy);
    }

    if (lastWindow && quitIfLast) {
        assert(0 == gWindows.Count());
        PostQuitMessage(0);
    } else if (lastWindow && !quitIfLast) {
        assert(gWindows.Find(win) != -1);
        UpdateToolbarAndScrollbarState(*win);
    }
}

static void OnMenuSaveAs(WindowInfo& win)
{
    if (!HasPermission(Perm_DiskAccess)) return;
    assert(win.dm);
    if (!win.IsDocLoaded()) return;

    const TCHAR *srcFileName = win.dm->FileName();
    ScopedMem<TCHAR> urlName;
    if (gPluginMode) {
        urlName.Set(ExtractFilenameFromURL(gPluginURL));
        // fall back to a generic "filename" instead of the more confusing temporary filename
        srcFileName = urlName ? urlName : _T("filename");
    }

    assert(srcFileName);
    if (!srcFileName) return;

    // Can't save a document's content as plain text if text copying isn't allowed
    bool hasCopyPerm = !win.dm->engine->IsImageCollection() &&
                       win.dm->engine->IsCopyingTextAllowed();
    bool canConvertToPDF = Engine_PS == win.dm->engineType;

    const TCHAR *defExt = win.dm->engine->GetDefaultFileExt();

    // Prepare the file filters (use \1 instead of \0 so that the
    // double-zero terminated string isn't cut by the string handling
    // methods too early on)
    str::Str<TCHAR> fileFilter(256);
    switch (win.dm->engineType) {
    case Engine_XPS:    fileFilter.Append(_TR("XPS documents")); break;
    case Engine_DjVu:   fileFilter.Append(_TR("DjVu documents")); break;
    case Engine_ComicBook: fileFilter.Append(_TR("Comic books")); break;
    case Engine_Image:  fileFilter.AppendFmt(_TR("Image files (*.%s)"), defExt + 1); break;
    case Engine_PS:     fileFilter.Append(_TR("Postscript documents")); break;
    case Engine_Chm:    fileFilter.Append(_TR("CHM documents")); break;
    default:            fileFilter.Append(_TR("PDF documents")); break;
    }
    fileFilter.AppendFmt(_T("\1*%s\1"), defExt);
    if (hasCopyPerm) {
        fileFilter.Append(_TR("Text documents"));
        fileFilter.Append(_T("\1*.txt\1"));
    }
    if (canConvertToPDF) {
        fileFilter.Append(_TR("PDF documents"));
        fileFilter.Append(_T("\1*.pdf\1"));
    }
    fileFilter.Append(_TR("All files"));
    fileFilter.Append(_T("\1*.*\1"));
    str::TransChars(fileFilter.Get(), _T("\1"), _T("\0"));

    TCHAR dstFileName[MAX_PATH];
    str::BufSet(dstFileName, dimof(dstFileName), path::GetBaseName(srcFileName));
    // TODO: fix saving embedded PDF documents
    str::TransChars(dstFileName, _T(":"), _T("_"));
    // Remove the extension so that it can be re-added depending on the chosen filter
    if (str::EndsWithI(dstFileName, defExt))
        dstFileName[str::Len(dstFileName) - str::Len(defExt)] = '\0';

    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = win.hwndFrame;
    ofn.lpstrFile = dstFileName;
    ofn.nMaxFile = dimof(dstFileName);
    ofn.lpstrFilter = fileFilter.Get();
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrDefExt = defExt + 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    bool ok = GetSaveFileName(&ofn);
    if (!ok)
        return;

    TCHAR * realDstFileName = dstFileName;
    // Make sure that the file has a valid ending
    if (!str::EndsWithI(dstFileName, defExt) &&
        !(hasCopyPerm && str::EndsWithI(dstFileName, _T(".txt"))) &&
        !(canConvertToPDF && str::EndsWithI(dstFileName, _T(".pdf")))) {
        if (hasCopyPerm && 2 == ofn.nFilterIndex)
            defExt = _T(".txt");
        else if (canConvertToPDF && (hasCopyPerm ? 3 : 2) == ofn.nFilterIndex)
            defExt = _T(".pdf");
        realDstFileName = str::Format(_T("%s%s"), dstFileName, defExt);
    }

    // Extract all text when saving as a plain text file
    if (hasCopyPerm && str::EndsWithI(realDstFileName, _T(".txt"))) {
        str::Str<TCHAR> text(1024);
        for (int pageNo = 1; pageNo <= win.dm->PageCount(); pageNo++)
            text.AppendAndFree(win.dm->engine->ExtractPageText(pageNo, _T("\r\n"), NULL, Target_Export));

        ScopedMem<char> textUTF8(str::conv::ToUtf8(text.LendData()));
        ScopedMem<char> textUTF8BOM(str::Join("\xEF\xBB\xBF", textUTF8));
        ok = file::WriteAll(realDstFileName, textUTF8BOM, str::Len(textUTF8BOM));
        if (!ok)
            MessageBox(win.hwndFrame, _TR("Failed to save a file"), _TR("Warning"), MB_OK | MB_ICONEXCLAMATION | (IsUIRightToLeft() ? MB_RTLREADING : 0));
    }
    // Convert the Postscript file into a PDF one
    else if (canConvertToPDF && str::EndsWithI(realDstFileName, _T(".pdf"))) {
        size_t dataLen;
        ScopedMem<unsigned char> data(static_cast<PsEngine *>(win.dm->engine)->GetPDFData(&dataLen));
        ok = data && file::WriteAll(realDstFileName, data, dataLen);
        if (!ok)
            MessageBox(win.hwndFrame, _TR("Failed to save a file"), _TR("Warning"), MB_OK | MB_ICONEXCLAMATION | (IsUIRightToLeft() ? MB_RTLREADING : 0));
    }
    // Recreate inexistant files from memory...
    else if (!file::Exists(srcFileName)) {
        size_t dataLen;
        ScopedMem<unsigned char> data(win.dm->engine->GetFileData(&dataLen));
        ok = data && file::WriteAll(realDstFileName, data, dataLen);
        if (!ok)
            MessageBox(win.hwndFrame, _TR("Failed to save a file"), _TR("Warning"), MB_OK | MB_ICONEXCLAMATION | (IsUIRightToLeft() ? MB_RTLREADING : 0));
    }
    // ... else just copy the file
    else {
        ok = CopyFileEx(srcFileName, realDstFileName, NULL, NULL, NULL, 0);
        if (ok) {
            // Make sure that the copy isn't write-locked or hidden
            const DWORD attributesToDrop = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM;
            DWORD attributes = GetFileAttributes(realDstFileName);
            if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & attributesToDrop))
                SetFileAttributes(realDstFileName, attributes & ~attributesToDrop);
        } else {
            TCHAR *msgBuf, *errorMsg;
            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), 0, (LPTSTR)&msgBuf, 0, NULL)) {
                errorMsg = str::Format(_T("%s\n\n%s"), _TR("Failed to save a file"), msgBuf);
                LocalFree(msgBuf);
            } else {
                errorMsg = str::Dup(_TR("Failed to save a file"));
            }
            MessageBox(win.hwndFrame, errorMsg, _TR("Warning"), MB_OK | MB_ICONEXCLAMATION | (IsUIRightToLeft() ? MB_RTLREADING : 0));
            free(errorMsg);
        }
    }

    if (ok && IsUntrustedFile(win.dm->FileName(), gPluginURL))
        file::SetZoneIdentifier(realDstFileName);

    if (realDstFileName != dstFileName)
        free(realDstFileName);
}

bool LinkSaver::SaveEmbedded(unsigned char *data, int len)
{
    if (!HasPermission(Perm_DiskAccess))
        return false;

    TCHAR dstFileName[MAX_PATH];
    str::BufSet(dstFileName, dimof(dstFileName), fileName ? fileName : _T(""));

    // Prepare the file filters (use \1 instead of \0 so that the
    // double-zero terminated string isn't cut by the string handling
    // methods too early on)
    ScopedMem<TCHAR> fileFilter(str::Format(_T("%s\1*.*\1"), _TR("All files")));
    str::TransChars(fileFilter, _T("\1"), _T("\0"));

    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner->hwndFrame;
    ofn.lpstrFile = dstFileName;
    ofn.nMaxFile = dimof(dstFileName);
    ofn.lpstrFilter = fileFilter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    bool ok = GetSaveFileName(&ofn);
    if (!ok)
        return false;
    ok = file::WriteAll(dstFileName, data, len);
    if (ok && IsUntrustedFile(owner->dm ? owner->dm->FileName() : owner->loadedFilePath, gPluginURL))
        file::SetZoneIdentifier(dstFileName);
    return ok;
}

static void OnMenuSaveBookmark(WindowInfo& win)
{
    if (!HasPermission(Perm_DiskAccess) || gPluginMode) return;
    assert(win.dm);
    if (!win.IsDocLoaded()) return;

    const TCHAR *defExt = win.dm->engine->GetDefaultFileExt();

    TCHAR dstFileName[MAX_PATH];
    // Remove the extension so that it can be re-added depending on the chosen filter
    str::BufSet(dstFileName, dimof(dstFileName), path::GetBaseName(win.dm->FileName()));
    str::TransChars(dstFileName, _T(":"), _T("_"));
    if (str::EndsWithI(dstFileName, defExt))
        dstFileName[str::Len(dstFileName) - str::Len(defExt)] = '\0';

    // Prepare the file filters (use \1 instead of \0 so that the
    // double-zero terminated string isn't cut by the string handling
    // methods too early on)
    ScopedMem<TCHAR> fileFilter(str::Format(_T("%s\1*.lnk\1"), _TR("Bookmark Shortcuts")));
    str::TransChars(fileFilter, _T("\1"), _T("\0"));

    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = win.hwndFrame;
    ofn.lpstrFile = dstFileName;
    ofn.nMaxFile = dimof(dstFileName);
    ofn.lpstrFilter = fileFilter;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = _T("lnk");
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;

    if (FALSE == GetSaveFileName(&ofn))
        return;

    ScopedMem<TCHAR> filename(str::Dup(dstFileName));
    if (!str::EndsWithI(dstFileName, _T(".lnk")))
        filename.Set(str::Join(dstFileName, _T(".lnk")));

    ScrollState ss = win.dm->GetScrollState();
    const TCHAR *viewMode = DisplayModeConv::NameFromEnum(win.dm->displayMode());
    ScopedMem<TCHAR> ZoomVirtual(str::Format(_T("%.2f"), win.dm->ZoomVirtual()));
    if (ZOOM_FIT_PAGE == win.dm->ZoomVirtual())
        ZoomVirtual.Set(str::Dup(_T("fitpage")));
    else if (ZOOM_FIT_WIDTH == win.dm->ZoomVirtual())
        ZoomVirtual.Set(str::Dup(_T("fitwidth")));
    else if (ZOOM_FIT_CONTENT == win.dm->ZoomVirtual())
        ZoomVirtual.Set(str::Dup(_T("fitcontent")));

    ScopedMem<TCHAR> exePath(GetExePath());
    ScopedMem<TCHAR> args(str::Format(_T("\"%s\" -page %d -view \"%s\" -zoom %s -scroll %d,%d -reuse-instance"),
                          win.dm->FileName(), ss.page, viewMode, ZoomVirtual, (int)ss.x, (int)ss.y));
    ScopedMem<TCHAR> label(win.dm->engine->GetPageLabel(ss.page));
    ScopedMem<TCHAR> desc(str::Format(_TR("Bookmark shortcut to page %s of %s"),
                          label, path::GetBaseName(win.dm->FileName())));

    CreateShortcut(filename, exePath, args, desc, 1);
}

#if 0
// code adapted from http://support.microsoft.com/kb/131462/en-us
static UINT_PTR CALLBACK FileOpenHook(HWND hDlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uiMsg) {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        break;
    case WM_NOTIFY:
        if (((LPOFNOTIFY)lParam)->hdr.code == CDN_SELCHANGE) {
            LPOPENFILENAME lpofn = (LPOPENFILENAME)GetWindowLongPtr(hDlg, GWLP_USERDATA);
            // make sure that the filename buffer is large enough to hold
            // all the selected filenames
            int cbLength = CommDlg_OpenSave_GetSpec(GetParent(hDlg), NULL, 0) + MAX_PATH;
            if (cbLength >= 0 && lpofn->nMaxFile < (DWORD)cbLength) {
                TCHAR *oldBuffer = lpofn->lpstrFile;
                lpofn->lpstrFile = (LPTSTR)realloc(lpofn->lpstrFile, cbLength * sizeof(TCHAR));
                if (lpofn->lpstrFile)
                    lpofn->nMaxFile = cbLength;
                else
                    lpofn->lpstrFile = oldBuffer;
            }
        }
        break;
    }

    return 0;
}
#endif

static void OnMenuOpen(WindowInfo& win)
{
    if (!HasPermission(Perm_DiskAccess)) return;
    // don't allow opening different files in plugin mode
    if (gPluginMode)
        return;

    struct {
        const TCHAR *name;
        TCHAR *filter;
        bool available;
    } fileFormats[] = {
        { _TR("PDF documents"),         _T("*.pdf"),        true },
        { _TR("XPS documents"),         _T("*.xps"),        true },
        { _TR("DjVu documents"),        _T("*.djvu"),       true },
        { _TR("Postscript documents"),  _T("*.ps;*.eps"),   PsEngine::IsAvailable() },
        { _TR("Comic books"),           _T("*.cbz;*.cbr"),  true },
        { _TR("CHM documents"),          _T("*.chm"),        true },
    };
    // Prepare the file filters (use \1 instead of \0 so that the
    // double-zero terminated string isn't cut by the string handling
    // methods too early on)
    str::Str<TCHAR> fileFilter;
    StrVec filters;
    for (int i = 0; i < dimof(fileFormats); i++)
        if (fileFormats[i].available)
            filters.Append(fileFormats[i].filter);
    fileFilter.Append(_TR("All supported documents"));
    fileFilter.Append('\1');
    fileFilter.AppendAndFree(filters.Join(_T(";")));
    fileFilter.Append('\1');
    filters.Reset();

    for (int i = 0; i < dimof(fileFormats); i++)
        if (fileFormats[i].available)
            fileFilter.AppendAndFree(str::Format(_T("%s\1%s\1"), fileFormats[i].name, fileFormats[i].filter));
    fileFilter.AppendAndFree(str::Format(_T("%s\1*.*\1"), _TR("All files")));
    str::TransChars(fileFilter.Get(), _T("\1"), _T("\0"));

    OPENFILENAME ofn = { 0 };
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = win.hwndFrame;

    ofn.lpstrFilter = fileFilter.Get();
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY |
                OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    // OFN_ENABLEHOOK disables the new Open File dialog under Windows Vista
    // and later, so don't use it and just allocate enough memory to contain
    // several dozen file paths and hope that this is enough
    // TODO: Use IFileOpenDialog instead (requires a Vista SDK, though)
    ofn.nMaxFile = MAX_PATH * 100;
#if 0
    if (!WindowsVerVistaOrGreater())
    {
        ofn.lpfnHook = FileOpenHook;
        ofn.Flags |= OFN_ENABLEHOOK;
        ofn.nMaxFile = MAX_PATH / 2;
    }
    // note: ofn.lpstrFile can be reallocated by GetOpenFileName -> FileOpenHook
#endif
    ScopedMem<TCHAR> file(SAZA(TCHAR, ofn.nMaxFile));
    ofn.lpstrFile = file;

    if (!GetOpenFileName(&ofn))
        return;

    TCHAR *fileName = ofn.lpstrFile + ofn.nFileOffset;
    if (*(fileName - 1)) {
        // special case: single filename without NULL separator
        LoadDocument(ofn.lpstrFile, &win);
        return;
    }

    while (*fileName) {
        ScopedMem<TCHAR> filePath(path::Join(ofn.lpstrFile, fileName));
        if (filePath)
            LoadDocument(filePath, &win);
        fileName += str::Len(fileName) + 1;
    }
}

static void BrowseFolder(WindowInfo& win, bool forward)
{
    assert(win.loadedFilePath);
    if (win.IsAboutWindow()) return;
    if (!HasPermission(Perm_DiskAccess) || gPluginMode) return;

    // TODO: browse through all supported file types at the same time?
    const TCHAR *fileExt = path::GetExt(win.loadedFilePath);
    if (win.IsDocLoaded())
        fileExt = win.dm->engine->GetDefaultFileExt();
    ScopedMem<TCHAR> pattern(str::Format(_T("*%s"), fileExt));
    ScopedMem<TCHAR> dir(path::GetDir(win.loadedFilePath));
    pattern.Set(path::Join(dir, pattern));

    StrVec files;
    if (!CollectPathsFromDirectory(pattern, files))
        return;

    if (-1 == files.Find(win.loadedFilePath))
        files.Append(str::Dup(win.loadedFilePath));
    files.SortNatural();

    int index = files.Find(win.loadedFilePath);
    if (forward)
        index = (index + 1) % files.Count();
    else
        index = (int)(index + files.Count() - 1) % files.Count();

    UpdateCurrentFileDisplayStateForWin(&win);
    LoadDocument(files.At(index), &win, true, true);
}

static void OnVScroll(WindowInfo& win, WPARAM wParam)
{
    if (!win.IsDocLoaded())
        return;
    assert(win.dm);

    SCROLLINFO si = { 0 };
    si.cbSize = sizeof (si);
    si.fMask  = SIF_ALL;
    GetScrollInfo(win.hwndCanvas, SB_VERT, &si);

    int iVertPos = si.nPos;
    int lineHeight = 16;
    if (!displayModeContinuous(win.dm->displayMode()) && ZOOM_FIT_PAGE == win.dm->ZoomVirtual())
        lineHeight = 1;

    switch (LOWORD(wParam)) {
    case SB_TOP:        si.nPos = si.nMin; break;
    case SB_BOTTOM:     si.nPos = si.nMax; break;
    case SB_LINEUP:     si.nPos -= lineHeight; break;
    case SB_LINEDOWN:   si.nPos += lineHeight; break;
    case SB_PAGEUP:     si.nPos -= si.nPage; break;
    case SB_PAGEDOWN:   si.nPos += si.nPage; break;
    case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
    }

    // Set the position and then retrieve it.  Due to adjustments
    // by Windows it may not be the same as the value set.
    si.fMask = SIF_POS;
    SetScrollInfo(win.hwndCanvas, SB_VERT, &si, TRUE);
    GetScrollInfo(win.hwndCanvas, SB_VERT, &si);

    // If the position has changed, scroll the window and update it
    if (win.IsDocLoaded() && (si.nPos != iVertPos))
        win.dm->ScrollYTo(si.nPos);
}

static void OnHScroll(WindowInfo& win, WPARAM wParam)
{
    if (!win.IsDocLoaded())
        return;
    assert(win.dm);

    SCROLLINFO si = { 0 };
    si.cbSize = sizeof (si);
    si.fMask  = SIF_ALL;
    GetScrollInfo(win.hwndCanvas, SB_HORZ, &si);

    int iVertPos = si.nPos;
    switch (LOWORD(wParam)) {
    case SB_LEFT:       si.nPos = si.nMin; break;
    case SB_RIGHT:      si.nPos = si.nMax; break;
    case SB_LINELEFT:   si.nPos -= 16; break;
    case SB_LINERIGHT:  si.nPos += 16; break;
    case SB_PAGELEFT:   si.nPos -= si.nPage; break;
    case SB_PAGERIGHT:  si.nPos += si.nPage; break;
    case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
    }

    // Set the position and then retrieve it.  Due to adjustments
    // by Windows it may not be the same as the value set.
    si.fMask = SIF_POS;
    SetScrollInfo(win.hwndCanvas, SB_HORZ, &si, TRUE);
    GetScrollInfo(win.hwndCanvas, SB_HORZ, &si);

    // If the position has changed, scroll the window and update it
    if (win.IsDocLoaded() && (si.nPos != iVertPos))
        win.dm->ScrollXTo(si.nPos);
}

static void AdjustWindowEdge(WindowInfo& win)
{
    DWORD exStyle = GetWindowLong(win.hwndCanvas, GWL_EXSTYLE);
    DWORD newStyle = exStyle;

    // Remove the canvas' edge in the cases where the vertical scrollbar
    // would otherwise touch the screen's edge, making the scrollbar much
    // easier to hit with the mouse (cf. Fitts' law)
    // TODO: should we just always remove the canvas' edge?
    if (IsZoomed(win.hwndFrame) || win.fullScreen || win.presentation || gPluginMode)
        newStyle &= ~WS_EX_STATICEDGE;
    else
        newStyle |= WS_EX_STATICEDGE;

    if (newStyle != exStyle) {
        SetWindowLong(win.hwndCanvas, GWL_EXSTYLE, newStyle);
        SetWindowPos(win.hwndCanvas, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

static void FrameOnSize(WindowInfo* win, int dx, int dy)
{
    int rebBarDy = 0;
    if (gGlobalPrefs.toolbarVisible && !(win->presentation || win->fullScreen)) {
        SetWindowPos(win->hwndReBar, NULL, 0, 0, dx, 0, SWP_NOZORDER);
#ifdef BUILD_RIBBON
        if (win->ribbonSupport)
            rebBarDy = win->ribbonSupport->ribbonDy;
        else
#endif
        rebBarDy = WindowRect(win->hwndReBar).dy;
    }

    bool tocVisible = win->tocLoaded && win->tocVisible;
    if (tocVisible || gGlobalPrefs.favVisible)
        SetSidebarVisibility(win, tocVisible, gGlobalPrefs.favVisible);
    else
        SetWindowPos(win->hwndCanvas, NULL, 0, rebBarDy, dx, dy - rebBarDy, SWP_NOZORDER);

    if (win->presentation || win->fullScreen) {
        RectI rect = GetFullscreenRect(win->hwndFrame);
        // Windows XP sometimes seems to change the window size on it's own
        if (rect != WindowRect(win->hwndFrame))
            MoveWindow(win->hwndFrame, rect);
    }
}

void ChangeLanguage(const char *langName)
{
    CurrLangNameSet(langName);
    UpdateRtlLayoutForAllWindows();
    RebuildMenuBarForAllWindows();
    UpdateUITextForLanguage();
}

static void OnMenuChangeLanguage(WindowInfo& win)
{
    int langId = Trans::GetLanguageIndex(gGlobalPrefs.currentLanguage);
    int newLangId = Dialog_ChangeLanguge(win.hwndFrame, langId);

    if (newLangId != -1 && langId != newLangId) {
        const char *langName = Trans::GetLanguageCode(newLangId);
        assert(langName);
        if (!langName)
            return;

        ChangeLanguage(langName);

        if (gWindows.Count() > 0 && gWindows.At(0)->IsAboutWindow())
            gWindows.At(0)->RedrawAll(true);
        SavePrefs();
    }
}

static void OnMenuViewShowHideToolbar()
{
    gGlobalPrefs.toolbarVisible = !gGlobalPrefs.toolbarVisible;
    ShowOrHideToolbarGlobally();
}

static void OnMenuSettings(WindowInfo& win)
{
    if (!HasPermission(Perm_SavePreferences)) return;

    if (IDOK != Dialog_Settings(win.hwndFrame, &gGlobalPrefs))
        return;

    if (!gGlobalPrefs.rememberOpenedFiles) {
        gFileHistory.Clear();
        CleanUpThumbnailCache(gFileHistory);
    }
    if (gWindows.Count() > 0 && gWindows.At(0)->IsAboutWindow())
        gWindows.At(0)->RedrawAll(true);

    SavePrefs();
}

// toggles 'show pages continuously' state
static void OnMenuViewContinuous(WindowInfo& win)
{
    if (!win.IsDocLoaded())
        return;

    DisplayMode newMode = win.dm->displayMode();
    switch (newMode) {
        case DM_SINGLE_PAGE:
        case DM_CONTINUOUS:
            newMode = displayModeContinuous(newMode) ? DM_SINGLE_PAGE : DM_CONTINUOUS;
            break;
        case DM_FACING:
        case DM_CONTINUOUS_FACING:
            newMode = displayModeContinuous(newMode) ? DM_FACING : DM_CONTINUOUS_FACING;
            break;
        case DM_BOOK_VIEW:
        case DM_CONTINUOUS_BOOK_VIEW:
            newMode = displayModeContinuous(newMode) ? DM_BOOK_VIEW : DM_CONTINUOUS_BOOK_VIEW;
            break;
    }
    SwitchToDisplayMode(&win, newMode);
}

static void ChangeZoomLevel(WindowInfo *win, float newZoom, bool pagesContinuously)
{
    if (!win->IsDocLoaded())
        return;

    float zoom = win->dm->ZoomVirtual();
    DisplayMode mode = win->dm->displayMode();
    DisplayMode newMode = pagesContinuously ? DM_CONTINUOUS : DM_SINGLE_PAGE;

    if (mode != newMode || zoom != newZoom) {
        DisplayMode prevMode = win->prevDisplayMode;
        float prevZoom = win->prevZoomVirtual;

        if (mode != newMode)
            SwitchToDisplayMode(win, newMode);
        OnMenuZoom(win, MenuIdFromVirtualZoom(newZoom));

        // remember the previous values for when the toolbar button is unchecked
        if (INVALID_ZOOM == prevZoom) {
            win->prevZoomVirtual = zoom;
            win->prevDisplayMode = mode;
        }
        // keep the rememberd values when toggling between the two toolbar buttons
        else {
            win->prevZoomVirtual = prevZoom;
            win->prevDisplayMode = prevMode;
        }
    }
    else if (win->prevZoomVirtual != INVALID_ZOOM) {
        float prevZoom = win->prevZoomVirtual;
        SwitchToDisplayMode(win, win->prevDisplayMode);
        ZoomToSelection(win, prevZoom, false);
    }
}

static void FocusPageNoEdit(HWND hwndPageBox)
{
    if (GetFocus() == hwndPageBox)
        SendMessage(hwndPageBox, WM_SETFOCUS, 0, 0);
    else
        SetFocus(hwndPageBox);
}

static void OnMenuGoToPage(WindowInfo& win)
{
    if (!win.IsDocLoaded())
        return;

    // Don't show a dialog if we don't have to - use the Toolbar instead
    if (gGlobalPrefs.toolbarVisible && !win.fullScreen && !win.presentation
#ifdef BUILD_RIBBON
        && !win.ribbonSupport
#endif
        ) {
        FocusPageNoEdit(win.hwndPageBox);
        return;
    }

    ScopedMem<TCHAR> label(win.dm->engine->GetPageLabel(win.dm->CurrentPageNo()));
    ScopedMem<TCHAR> newPageLabel(Dialog_GoToPage(win.hwndFrame, label, win.dm->PageCount(),
                                                  !win.dm->engine->HasPageLabels()));
    if (!newPageLabel)
        return;

    int newPageNo = win.dm->engine->GetPageByLabel(newPageLabel);
    if (win.dm->ValidPageNo(newPageNo))
        win.dm->GoToPage(newPageNo, 0, true);
}

static void EnterFullscreen(WindowInfo& win, bool presentation)
{
    if ((presentation ? win.presentation : win.fullScreen) ||
        !IsWindowVisible(win.hwndFrame) || gPluginMode)
        return;

    assert(presentation ? !win.fullScreen : !win.presentation);
    if (presentation) {
        assert(win.dm);
        if (!win.IsDocLoaded())
            return;

        if (IsZoomed(win.hwndFrame))
            win.windowStateBeforePresentation = WIN_STATE_MAXIMIZED;
        else
            win.windowStateBeforePresentation = WIN_STATE_NORMAL;
        win.presentation = PM_ENABLED;
        win.tocBeforeFullScreen = win.tocVisible;

        SetTimer(win.hwndCanvas, HIDE_CURSOR_TIMER_ID, HIDE_CURSOR_DELAY_IN_MS, NULL);
    }
    else {
        win.fullScreen = true;
        win.tocBeforeFullScreen = win.IsDocLoaded() ? win.tocVisible : false;
    }

    // Remove TOC and favorites from full screen, add back later on exit fullscreen
    bool favVisibleTmp = gGlobalPrefs.favVisible;
    if (win.tocVisible || gGlobalPrefs.favVisible) {
        SetSidebarVisibility(&win, false, false);
        // restore gGlobalPrefs.favVisible changed by SetSidebarVisibility()
    }

    long ws = GetWindowLong(win.hwndFrame, GWL_STYLE);
    if (!presentation || !win.fullScreen)
        win.prevStyle = ws;
    ws &= ~(WS_BORDER|WS_CAPTION|WS_THICKFRAME);
    ws |= WS_MAXIMIZE;

    win.frameRc = WindowRect(win.hwndFrame);
    RectI rect = GetFullscreenRect(win.hwndFrame);

#ifdef BUILD_RIBBON
    if (win.ribbonSupport)
        win.ribbonSupport->SetVisibility(false);
    else
#endif
    {
        SetMenu(win.hwndFrame, NULL);
        ShowWindow(win.hwndReBar, SW_HIDE);
    }
    SetWindowLong(win.hwndFrame, GWL_STYLE, ws);
    SetWindowPos(win.hwndFrame, NULL, rect.x, rect.y, rect.dx, rect.dy, SWP_FRAMECHANGED | SWP_NOZORDER);
    SetWindowPos(win.hwndCanvas, NULL, 0, 0, rect.dx, rect.dy, SWP_NOZORDER);

    if (presentation)
        win.dm->SetPresentationMode(true);

    // Make sure that no toolbar/sidebar keeps the focus
    SetFocus(win.hwndFrame);
    gGlobalPrefs.favVisible = favVisibleTmp;
}

static void ExitFullscreen(WindowInfo& win)
{
    if (!win.fullScreen && !win.presentation) 
        return;

    bool wasPresentation = PM_DISABLED != win.presentation;
    if (wasPresentation && win.dm) {
        win.dm->SetPresentationMode(false);
        win.presentation = PM_DISABLED;
    }
    else
        win.fullScreen = false;

    if (wasPresentation) {
        KillTimer(win.hwndCanvas, HIDE_CURSOR_TIMER_ID);
        SetCursor(gCursorArrow);
    }

    bool tocVisible = win.IsDocLoaded() && win.tocBeforeFullScreen;
    if (tocVisible || gGlobalPrefs.favVisible)
        SetSidebarVisibility(&win, tocVisible, gGlobalPrefs.favVisible);

#ifdef BUILD_RIBBON
    if (win.ribbonSupport)
        win.ribbonSupport->SetVisibility(true);
    else
#endif
    {
        if (gGlobalPrefs.toolbarVisible)
            ShowWindow(win.hwndReBar, SW_SHOW);
        SetMenu(win.hwndFrame, win.menu);
    }
    SetWindowLong(win.hwndFrame, GWL_STYLE, win.prevStyle);
    SetWindowPos(win.hwndFrame, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE);
    MoveWindow(win.hwndFrame, win.frameRc);
    assert(WindowRect(win.hwndFrame) == win.frameRc);
}

static void OnMenuViewFullscreen(WindowInfo& win, bool presentation=false)
{
    bool enterFullscreen = presentation ? !win.presentation : !win.fullScreen;

    if (!win.presentation && !win.fullScreen)
        RememberWindowPosition(win);
    else
        ExitFullscreen(win);

    if (enterFullscreen && (!presentation || win.IsDocLoaded()))
        EnterFullscreen(win, presentation);
}

static void OnMenuViewPresentation(WindowInfo& win)
{
    OnMenuViewFullscreen(win, true);
}

void AdvanceFocus(WindowInfo* win)
{
    // Tab order: Frame -> Page -> Find -> ToC -> Favorites -> Frame -> ...

    bool hasToolbar = !win->fullScreen && !win->presentation &&
                      gGlobalPrefs.toolbarVisible && win->IsDocLoaded();
#ifdef BUILD_RIBBON
    hasToolbar = hasToolbar && !win->ribbonSupport;
#endif
    int direction = IsShiftPressed() ? -1 : 1;

    struct {
        HWND hwnd;
        bool isAvailable;
    } tabOrder[] = {
        { win->hwndFrame,   true                                },
        { win->hwndPageBox, hasToolbar                          },
        { win->hwndFindBox, hasToolbar && NeedsFindUI(win)      },
        { win->hwndTocTree, win->tocLoaded && win->tocVisible   },
        { win->hwndFavTree, gGlobalPrefs.favVisible             },
    };

    /* // make sure that at least one element is available
    bool hasAvailable = false;
    for (int i = 0; i < dimof(tabOrder) && !hasAvailable; i++)
        hasAvailable = tabOrder[i].isAvailable;
    if (!hasAvailable)
        return;
    // */

    // find the currently focused element
    HWND current = GetFocus();
    int ix;
    for (ix = 0; ix < dimof(tabOrder); ix++)
        if (tabOrder[ix].hwnd == current)
            break;
    // if it's not in the tab order, start at the beginning
    if (ix == dimof(tabOrder))
        ix = -direction;

    // focus the next available element
    do {
        ix = (ix + direction + dimof(tabOrder)) % dimof(tabOrder);
    } while (!tabOrder[ix].isAvailable);
    SetFocus(tabOrder[ix].hwnd);
}

// allow to distinguish a '/' caused by VK_DIVIDE (rotates a document)
// from one typed on the main keyboard (focuses the find textbox)
static bool gIsDivideKeyDown = false;

static bool ChmForwardKey(WPARAM key)
{
    if ((VK_LEFT == key) || (VK_RIGHT == key))
        return true;
    if ((VK_UP == key) || (VK_DOWN == key))
        return true;
    if ((VK_HOME == key) || (VK_END == key))
        return true;
    if ((VK_PRIOR == key) || (VK_NEXT == key))
        return true;
    if ((VK_MULTIPLY == key) || (VK_DIVIDE == key))
        return true;
    return false;
}

bool FrameOnKeydown(WindowInfo *win, WPARAM key, LPARAM lparam, bool inTextfield)
{
	/*MyCode*/
	if(g_pIntf)
	{
		if(g_pIntf->OnKeydown(key,lparam))
		{
			return true;
		}
	}
	//////////////////////////////////////////////////////////////////////////


    if ((VK_LEFT == key || VK_RIGHT == key) &&
        IsShiftPressed() && IsCtrlPressed() &&
        win->loadedFilePath && !inTextfield) {
        // folder browsing should also work when an error page is displayed,
        // so special-case it before the win.IsDocLoaded() check
        BrowseFolder(*win, VK_RIGHT == key);
        return true;
    }

    if (!win->IsDocLoaded())
        return false;

    if (win->IsChm()) {
        if (ChmForwardKey(key)) {
            ChmEngine *chmEng = win->dm->AsChmEngine();
            HtmlWindow *htmlWin = chmEng->GetHtmlWindow();
            htmlWin->SendMsg(WM_KEYDOWN, key, lparam);
            return true;
        }
    }
    //DBG_OUT("key=%d,%c,shift=%d\n", key, (char)key, (int)WasKeyDown(VK_SHIFT));

    if (PM_BLACK_SCREEN == win->presentation || PM_WHITE_SCREEN == win->presentation)
        return false;

    if (VK_PRIOR == key) {
        int currentPos = GetScrollPos(win->hwndCanvas, SB_VERT);
        if (win->dm->ZoomVirtual() != ZOOM_FIT_CONTENT)
            SendMessage(win->hwndCanvas, WM_VSCROLL, SB_PAGEUP, 0);
        if (GetScrollPos(win->hwndCanvas, SB_VERT) == currentPos)
            win->dm->GoToPrevPage(-1);
    } else if (VK_NEXT == key) {
        int currentPos = GetScrollPos(win->hwndCanvas, SB_VERT);
        if (win->dm->ZoomVirtual() != ZOOM_FIT_CONTENT)
            SendMessage(win->hwndCanvas, WM_VSCROLL, SB_PAGEDOWN, 0);
        if (GetScrollPos(win->hwndCanvas, SB_VERT) == currentPos)
            win->dm->GoToNextPage(0);
    } else if (VK_UP == key) {
        if (win->dm->NeedVScroll())
            SendMessage(win->hwndCanvas, WM_VSCROLL, SB_LINEUP, 0);
        else
            win->dm->GoToPrevPage(-1);
    } else if (VK_DOWN == key) {
        if (win->dm->NeedVScroll())
            SendMessage(win->hwndCanvas, WM_VSCROLL, SB_LINEDOWN, 0);
        else
            win->dm->GoToNextPage(0);
    } else if (inTextfield) {
        // The remaining keys have a different meaning
        return false;
    } else if (VK_LEFT == key) {
        if (win->dm->NeedHScroll())
            SendMessage(win->hwndCanvas, WM_HSCROLL, IsShiftPressed() ? SB_PAGELEFT : SB_LINELEFT, 0);
        else
            win->dm->GoToPrevPage(0);
    } else if (VK_RIGHT == key) {
        if (win->dm->NeedHScroll())
            SendMessage(win->hwndCanvas, WM_HSCROLL, IsShiftPressed() ? SB_PAGERIGHT : SB_LINERIGHT, 0);
        else
            win->dm->GoToNextPage(0);
    } else if (VK_HOME == key) {
        win->dm->GoToFirstPage();
    } else if (VK_END == key) {
        win->dm->GoToLastPage();
    } else if (VK_MULTIPLY == key) {
        win->dm->RotateBy(90);
    } else if (VK_DIVIDE == key) {
        win->dm->RotateBy(-90);
        gIsDivideKeyDown = true;
    } else {
        return false;
    }

    return true;
}

static void FrameOnChar(WindowInfo& win, WPARAM key)
{
//    DBG_OUT("char=%d,%c\n", key, (char)key);

	/*MyCode*/
	if(g_pIntf)
	{
		if(g_pIntf->OnChar(key))
		{
			return;
		}
	}
	//////////////////////////////////////////////////////////////////////////

    if (IsCharUpper((TCHAR)key))
        key = (TCHAR)CharLower((LPTSTR)(TCHAR)key);

    if (PM_BLACK_SCREEN == win.presentation || PM_WHITE_SCREEN == win.presentation) {
        win.ChangePresentationMode(PM_ENABLED);
        return;
    }

    switch (key) {
    case VK_ESCAPE:
        if (win.findThread)
            AbortFinding(&win);
        else if (win.notifications->GetFirstInGroup(NG_PAGE_INFO_HELPER))
            win.notifications->RemoveAllInGroup(NG_PAGE_INFO_HELPER);
        else if (win.presentation)
            OnMenuViewPresentation(win);
        else if (gGlobalPrefs.escToExit)
            DestroyWindow(win.hwndFrame);
        else if (win.fullScreen)
            OnMenuViewFullscreen(win);
        else if (win.showSelection)
            ClearSearchResult(&win);
        return;
    case 'q':
        if (!gPluginMode)
            DestroyWindow(win.hwndFrame);
        return;
    case 'r':
        ReloadDocument(&win);
        return;
    case VK_TAB:
        AdvanceFocus(&win);
        break;
    }

    if (!win.IsDocLoaded())
        return;

    switch (key) {
    case VK_SPACE:
    case VK_RETURN:
        FrameOnKeydown(&win, IsShiftPressed() ? VK_PRIOR : VK_NEXT, 0);
        break;
    case VK_BACK:
        {
            bool forward = IsShiftPressed();
            win.dm->Navigate(forward ? 1 : -1);
        }
        break;
    case 'g':
        OnMenuGoToPage(win);
        break;
    case 'j':
        SendMessage(win.hwndCanvas, WM_VSCROLL, SB_LINEDOWN, 0);
        break;
    case 'k':
        SendMessage(win.hwndCanvas, WM_VSCROLL, SB_LINEUP, 0);
        break;
    case 'n':
        win.dm->GoToNextPage(0);
        break;
    case 'p':
        win.dm->GoToPrevPage(0);
        break;
    case 'z':
        win.ToggleZoom();
        break;
    case '+':
        ZoomToSelection(&win, ZOOM_IN_FACTOR, true);
        break;
    case '-':
        ZoomToSelection(&win, ZOOM_OUT_FACTOR, true);
        break;
    case '/':
        if (!gIsDivideKeyDown)
            OnMenuFind(&win);
        gIsDivideKeyDown = false;
        break;
    case 'c':
        OnMenuViewContinuous(win);
        break;
    case 'b':
        if (!displayModeSingle(win.dm->displayMode())) {
            // "e-book view": flip a single page
            bool forward = !IsShiftPressed();
            int currPage = win.dm->CurrentPageNo();
            if (forward ? win.dm->LastBookPageVisible() : win.dm->FirstBookPageVisible())
                break;

            DisplayMode newMode = DM_BOOK_VIEW;
            if (displayModeShowCover(win.dm->displayMode()))
                newMode = DM_FACING;
            SwitchToDisplayMode(&win, newMode, true);

            if (forward && currPage >= win.dm->CurrentPageNo() && (currPage > 1 || newMode == DM_BOOK_VIEW))
                win.dm->GoToNextPage(0);
            else if (!forward && currPage <= win.dm->CurrentPageNo())
                win.dm->GoToPrevPage(0);
        }
        break;
    case '.':
        // for Logitech's wireless presenters which target PowerPoint's shortcuts
        if (win.presentation)
            win.ChangePresentationMode(PM_BLACK_SCREEN);
        break;
    case 'w':
        if (win.presentation)
            win.ChangePresentationMode(PM_WHITE_SCREEN);
        break;
    case 'i':
        // experimental "page info" tip: make figuring out current page and
        // total pages count a one-key action (unless they're already visible)
        if (!gGlobalPrefs.toolbarVisible || win.fullScreen || PM_ENABLED == win.presentation
#ifdef BUILD_RIBBON
            || win.ribbonSupport
#endif
            ) {
            int current = win.dm->CurrentPageNo(), total = win.dm->PageCount();
            ScopedMem<TCHAR> pageInfo(str::Format(_T("%s %d / %d"), _TR("Page:"), current, total));
            if (win.dm->engine && win.dm->engine->HasPageLabels()) {
                ScopedMem<TCHAR> label(win.dm->engine->GetPageLabel(current));
                pageInfo.Set(str::Format(_T("%s %s (%d / %d)"), _TR("Page:"), label, current, total));
            }
            bool autoDismiss = !IsShiftPressed();
            ShowNotification(&win, pageInfo, autoDismiss, false, NG_PAGE_INFO_HELPER);
        }
        break;
#ifdef DEBUG
    case '$':
        ToggleGdiDebugging();
        break;
#endif
    }
}

static void UpdateSidebarTitles(WindowInfo& win)
{
    HWND tocTitle = GetDlgItem(win.hwndTocBox, IDC_TOC_TITLE);
    win::SetText(tocTitle, _TR("Bookmarks"));
    if (win.tocVisible) {
        InvalidateRect(win.hwndTocBox, NULL, TRUE);
        UpdateWindow(win.hwndTocBox);
    }

    HWND favTitle = GetDlgItem(win.hwndFavBox, IDC_FAV_TITLE);
    win::SetText(favTitle, _TR("Favorites"));
    if (gGlobalPrefs.favVisible) {
        InvalidateRect(win.hwndFavBox, NULL, TRUE);
        UpdateWindow(win.hwndFavBox);
    }
}

static void UpdateUITextForLanguage()
{
    for (size_t i = 0; i < gWindows.Count(); i++) {
        WindowInfo *win = gWindows.At(i);
        UpdateToolbarPageText(win, -1);
        UpdateToolbarFindText(win);
        UpdateToolbarButtonsToolTipsForWindow(win);
        // also update the sidebar title at this point
        UpdateSidebarTitles(*win);
    }        
}

// TODO: the layout logic here is similar to what we do in SetSidebarVisibility()
// would be nice to consolidate.
static void ResizeSidebar(WindowInfo *win)
{
    POINT pcur;
    GetCursorPos(&pcur);
    ScreenToClient(win->hwndFrame, &pcur);
    int sidebarDx = pcur.x; // without splitter

    ClientRect rToc(win->hwndTocBox);
    ClientRect rFav(win->hwndFavBox);
    assert(rToc.dx == rFav.dx);
    ClientRect rFrame(win->hwndFrame);

    // make sure to keep this in sync with the calculations in SetSidebarVisibility
    // note: without the min/max(..., rToc.dx), the sidebar will be
    //       stuck at its width if it accidentally got too wide or too narrow
    if (sidebarDx < min(SIDEBAR_MIN_WIDTH, rToc.dx) ||
        sidebarDx > max(rFrame.dx / 2, rToc.dx)) {
        SetCursor(gCursorNo);
        return;
    }

    SetCursor(gCursorSizeWE);

    int favSplitterDy = 0;
    bool favSplitterVisible = win->tocVisible && gGlobalPrefs.favVisible;
    if (favSplitterVisible)
        favSplitterDy = SPLITTER_DY;

    int canvasDx = rFrame.dx - sidebarDx - SPLITTER_DX;
    int y = 0;
    int totalDy = rFrame.dy;
#ifdef BUILD_RIBBON
    if (win->ribbonSupport && !win->fullScreen && !win->presentation)
        y = win->ribbonSupport->ribbonDy;
    else
#endif
    if (gGlobalPrefs.toolbarVisible && !win->fullScreen && !win->presentation)
        y = WindowRect(win->hwndReBar).dy;
    totalDy -= y;

    // rToc.y is always 0, as rToc is a ClientRect, so we first have
    // to convert it into coordinates relative to hwndFrame:
    assert(MapRectToWindow(rToc, win->hwndTocBox, win->hwndFrame).y == y);
    //assert(totalDy == (rToc.dy + rFav.dy));

    MoveWindow(win->hwndTocBox,      0, y,                           sidebarDx, rToc.dy, TRUE);
    MoveWindow(win->hwndFavSplitter, 0, y + rToc.dy,                 sidebarDx, favSplitterDy, TRUE);
    MoveWindow(win->hwndFavBox,      0, y + rToc.dy + favSplitterDy, sidebarDx, rFav.dy, TRUE);

    MoveWindow(win->hwndSidebarSplitter, sidebarDx, y, SPLITTER_DX, totalDy, TRUE);
    MoveWindow(win->hwndCanvas, sidebarDx + SPLITTER_DX, y, canvasDx, totalDy, TRUE);
}

// TODO: the layout logic here is similar to what we do in SetSidebarVisibility()
// would be nice to consolidate.
static void ResizeFav(WindowInfo *win)
{
    POINT pcur;
    GetCursorPos(&pcur);
    ScreenToClient(win->hwndTocBox, &pcur);
    int tocDy = pcur.y; // without splitter

    ClientRect rToc(win->hwndTocBox);
    ClientRect rFav(win->hwndFavBox);
    assert(rToc.dx == rFav.dx);
    ClientRect rFrame(win->hwndFrame);
    int tocDx = rToc.dx;

    // make sure to keep this in sync with the calculations in SetSidebarVisibility
    if (tocDy < min(TOC_MIN_DY, rToc.dy) ||
        tocDy > max(rFrame.dy - TOC_MIN_DY, rToc.dy)) {
        SetCursor(gCursorNo);
        return;
    }

    SetCursor(gCursorSizeNS);

    int y = 0;
    int totalDy = rFrame.dy;
    if (gGlobalPrefs.toolbarVisible && !win->fullScreen && !win->presentation)
        y = WindowRect(win->hwndReBar).dy;
    totalDy -= y;

    // rToc.y is always 0, as rToc is a ClientRect, so we first have
    // to convert it into coordinates relative to hwndFrame:
    assert(MapRectToWindow(rToc, win->hwndTocBox, win->hwndFrame).y == y);
    //assert(totalDy == (rToc.dy + rFav.dy));
    int favDy = totalDy - tocDy - SPLITTER_DY;
    assert(favDy >= 0);

    MoveWindow(win->hwndTocBox,      0, y,                       tocDx, tocDy,       TRUE);
    MoveWindow(win->hwndFavSplitter, 0, y + tocDy,               tocDx, SPLITTER_DY, TRUE);
    MoveWindow(win->hwndFavBox,      0, y + tocDy + SPLITTER_DY, tocDx, favDy,       TRUE);

    gGlobalPrefs.tocDy = tocDy;
}

static LRESULT CALLBACK WndProcFavSplitter(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WindowInfo *win = FindWindowInfoByHwnd(hwnd);
    if (!win)
        return DefWindowProc(hwnd, message, wParam, lParam);

    switch (message)
    {
        case WM_MOUSEMOVE:
            if (hwnd == GetCapture()) {
                ResizeFav(win);
                return 0;
            }
            break;
        case WM_LBUTTONDOWN:
            SetCapture(hwnd);
            break;
        case WM_LBUTTONUP:
            ReleaseCapture();
            break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

static LRESULT CALLBACK WndProcSidebarSplitter(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WindowInfo *win = FindWindowInfoByHwnd(hwnd);
    if (!win)
        return DefWindowProc(hwnd, message, wParam, lParam);

    switch (message)
    {
        case WM_MOUSEMOVE:
            if (hwnd == GetCapture()) {
                ResizeSidebar(win);
                return 0;
            }
            break;
        case WM_LBUTTONDOWN:
            SetCapture(hwnd);
            break;
        case WM_LBUTTONUP:
            ReleaseCapture();
            break;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

// A tree container, used for toc and favorites, is a window with following children:
// - title (id + 1)
// - close button (id + 2)
// - tree window (id + 3)
// This function lays out the child windows inside the container based
// on the container size.
void LayoutTreeContainer(HWND hwndContainer, int id)
{
    HWND hwndTitle = GetDlgItem(hwndContainer, id + 1);
    HWND hwndClose = GetDlgItem(hwndContainer, id + 2);
    HWND hwndTree  = GetDlgItem(hwndContainer, id + 3);

    ScopedMem<TCHAR> title(win::GetText(hwndTitle));
    SizeI size = TextSizeInHwnd(hwndTitle, title);

    WindowInfo *win = FindWindowInfoByHwnd(hwndContainer);
    assert(win);
    int offset = win ? (int)(2 * win->uiDPIFactor) : 2;
    if (size.dy < 16)
        size.dy = 16;
    size.dy += 2 * offset;

    WindowRect rc(hwndContainer);   
    MoveWindow(hwndTitle, offset, offset, rc.dx - 2 * offset - 16, size.dy - 2 * offset, TRUE);
    MoveWindow(hwndClose, rc.dx - 16, (size.dy - 16) / 2, 16, 16, TRUE);
    MoveWindow(hwndTree, 0, size.dy, rc.dx, rc.dy - size.dy, TRUE);
}

#define BUTTON_HOVER_TEXT _T("1")

#define COL_CLOSE_X RGB(0xa0, 0xa0, 0xa0)
#define COL_CLOSE_X_HOVER RGB(0xf9, 0xeb, 0xeb)  // white-ish
#define COL_CLOSE_HOVER_BG RGB(0xC1, 0x35, 0x35) // red-ish

// Draws the 'x' close button in regular state or onhover state
// Tries to mimic visual style of Chrome tab close button
void DrawCloseButton(DRAWITEMSTRUCT *dis)
{
    using namespace Gdiplus;

    RectI r(RectI::FromRECT(dis->rcItem));
    ScopedMem<TCHAR> s(win::GetText(dis->hwndItem));
    bool onHover = str::Eq(s, BUTTON_HOVER_TEXT);

    Graphics g(dis->hDC);
    g.SetCompositingQuality(CompositingQualityHighQuality);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPageUnit(UnitPixel);

    Color c;
    c.SetFromCOLORREF(GetSysColor(COLOR_BTNFACE)); // hoping this is always the right color
    SolidBrush bgBrush(c); 
    g.FillRectangle(&bgBrush, r.x, r.y, r.dx, r.dy);

    // in onhover state, background is a red-ish circle
    if (onHover) {
        c.SetFromCOLORREF(COL_CLOSE_HOVER_BG);
        SolidBrush b(c);
        g.FillEllipse(&b, r.x, r.y, r.dx-2, r.dy-2);
    }

    // draw 'x'
    c.SetFromCOLORREF(onHover ? COL_CLOSE_X_HOVER : COL_CLOSE_X);
    Pen p(c, 2);
    if (onHover) {
        Gdiplus::Point p1(4,      4);
        Gdiplus::Point p2(r.dx-6, r.dy-6);
        Gdiplus::Point p3(r.dx-6, 4);
        Gdiplus::Point p4(4,      r.dy-6);
        g.DrawLine(&p, p1, p2);
        g.DrawLine(&p, p3, p4);
    } else {
        Gdiplus::Point p1(4,      5);
        Gdiplus::Point p2(r.dx-6, r.dy-5);
        Gdiplus::Point p3(r.dx-6, 5);
        Gdiplus::Point p4(4,      r.dy-5);
        g.DrawLine(&p, p1, p2);
        g.DrawLine(&p, p3, p4);
    }
}

WNDPROC DefWndProcCloseButton = NULL;
// logic needed to track OnHover state of a close button by looking at
// WM_MOUSEMOVE and WM_MOUSELEAVE messages.
// We call it a button, but hwnd is really a static text.
// We persist the state by setting hwnd's text: if cursor is over the hwnd,
// text is "1", else it's something else.
LRESULT CALLBACK WndProcCloseButton(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    bool stateChanged = false;
    if (WM_MOUSEMOVE == msg) {
        ScopedMem<TCHAR> s(win::GetText(hwnd));
        if (!str::Eq(s, BUTTON_HOVER_TEXT)) {
            win::SetText(hwnd, BUTTON_HOVER_TEXT);
            stateChanged = true;
            // ask for WM_MOUSELEAVE notifications
            TRACKMOUSEEVENT tme = { 0 };
            tme.cbSize = sizeof(tme);
            tme.dwFlags = TME_LEAVE;
            tme.hwndTrack = hwnd;
            TrackMouseEvent(&tme);
        }
    } else if (WM_MOUSELEAVE == msg) {
        win::SetText(hwnd, _T(""));
        stateChanged = true;
    }

    if (stateChanged) {
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }

    return CallWindowProc(DefWndProcCloseButton, hwnd, msg, wParam, lParam);
}

static void SetWinPos(HWND hwnd, RectI r, bool isVisible)
{
    UINT flags = SWP_NOZORDER | (isVisible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
    SetWindowPos(hwnd, NULL, r.x, r.y, r.dx, r.dy, flags);
}

void SetSidebarVisibility(WindowInfo *win, bool tocVisible, bool favVisible)
{
    if (gPluginMode || !HasPermission(Perm_DiskAccess))
        favVisible = false;

    if (!win->IsDocLoaded() || !win->dm->HasTocTree())
        tocVisible = false;

    if (PM_BLACK_SCREEN == win->presentation || PM_WHITE_SCREEN == win->presentation) {
        tocVisible = false;
        favVisible = false;
    }

    bool sidebarVisible = tocVisible || favVisible;

    if (tocVisible)
        LoadTocTree(win);
    if (favVisible)
        PopulateFavTreeIfNeeded(win);

    win->tocVisible = tocVisible;
    gGlobalPrefs.favVisible = favVisible;

    ClientRect rFrame(win->hwndFrame);
    int toolbarDy = 0;
#ifdef BUILD_RIBBON
    if (win->ribbonSupport && !win->fullScreen && !win->presentation)
        toolbarDy = win->ribbonSupport->ribbonDy;
    else
#endif
    if (gGlobalPrefs.toolbarVisible && !win->fullScreen && !win->presentation)
        toolbarDy = WindowRect(win->hwndReBar).dy;
    int dy = rFrame.dy - toolbarDy;

    if (!sidebarVisible) {
        if (GetFocus() == win->hwndTocTree || GetFocus() == win->hwndFavTree)
            SetFocus(win->hwndFrame);

        SetWindowPos(win->hwndCanvas, NULL, 0, toolbarDy, rFrame.dx, dy, SWP_NOZORDER);
        ShowWindow(win->hwndSidebarSplitter, SW_HIDE);
        ShowWindow(win->hwndTocBox, SW_HIDE);
        ShowWindow(win->hwndFavSplitter, SW_HIDE);
        ShowWindow(win->hwndFavBox, SW_HIDE);
        return;
    }

    if (rFrame.IsEmpty()) {
        // don't adjust the ToC sidebar size while the window is minimized
        if (win->tocVisible)
            UpdateTocSelection(win, win->dm->CurrentPageNo());
        return;
    }

    int y = toolbarDy;
    ClientRect sidebarRc(win->hwndTocBox);
    int tocDx = sidebarRc.dx;
    if (tocDx == 0) {
        // TODO: use saved panelDx from saved preferences
        tocDx = rFrame.dx / 4;
    }

    // make sure that the sidebar is never too wide or too narrow
    // (when changing these values, also adjust ResizeSidebar() and ResizeFav())
    // TODO: we should also limit minimum size of the frame or else
    // limitValue() blows up with an assert() if frame.dx / 2 < SIDEBAR_MIN_WIDTH
    tocDx = limitValue(tocDx, SIDEBAR_MIN_WIDTH, rFrame.dx / 2);

    bool favSplitterVisible = tocVisible && favVisible;

    int tocDy = 0; // if !tocVisible
    if (tocVisible) {
        if (!favVisible)
            tocDy = dy;
        else if (gGlobalPrefs.tocDy)
            tocDy = gGlobalPrefs.tocDy;
        else
            tocDy = dy / 2; // default value
    }
    if (favSplitterVisible) {
        // TODO: we should also limit minimum size of the frame or else
        // limitValue() blows up with an assert() if TOC_MIN_DY < dy - TOC_MIN_DY
        tocDy = limitValue(tocDy, TOC_MIN_DY, dy-TOC_MIN_DY);
    }

    int canvasX = tocDx + SPLITTER_DX;
    RectI rToc(0, y, tocDx, tocDy);
    RectI rFavSplitter(0, y + tocDy, tocDx, SPLITTER_DY);
    int favSplitterDy = favSplitterVisible ? SPLITTER_DY : 0;
    RectI rFav(0, y + tocDy + favSplitterDy, tocDx, dy - tocDy - favSplitterDy);

    RectI rSplitter(tocDx, y, SPLITTER_DX, dy);
    RectI rCanvas(canvasX, y, rFrame.dx - canvasX, dy);

    SetWinPos(win->hwndTocBox,          rToc,           tocVisible);
    SetWinPos(win->hwndFavSplitter,     rFavSplitter,   favSplitterVisible);
    SetWinPos(win->hwndFavBox,          rFav,           favVisible);
    SetWinPos(win->hwndSidebarSplitter, rSplitter,      true);
    SetWinPos(win->hwndCanvas,          rCanvas,        true);

    if (tocVisible)
        UpdateTocSelection(win, win->dm->CurrentPageNo());
}

static LRESULT OnSetCursor(WindowInfo& win, HWND hwnd)
{
    POINT pt;

    if (win.IsAboutWindow()) {
        if (GetCursorPos(&pt) && ScreenToClient(hwnd, &pt)) {
            StaticLinkInfo linkInfo;
            if (GetStaticLink(win.staticLinks, pt.x, pt.y, &linkInfo)) {
                win.CreateInfotip(linkInfo.infotip, linkInfo.rect);
                SetCursor(gCursorHand);
                return TRUE;
            }
        }
    }
    if (!win.IsDocLoaded()) {
        win.DeleteInfotip();
        return FALSE;
    }

    if (win.mouseAction != MA_IDLE)
        win.DeleteInfotip();

    switch (win.mouseAction) {
        case MA_DRAGGING:
        case MA_DRAGGING_RIGHT:
            SetCursor(gCursorDrag);
            return TRUE;
        case MA_SCROLLING:
            SetCursor(gCursorScroll);
            return TRUE;
        case MA_SELECTING_TEXT:
            SetCursor(gCursorIBeam);
            return TRUE;
        case MA_SELECTING:
            break;
        case MA_IDLE:
            if (GetCursor() && GetCursorPos(&pt) && ScreenToClient(hwnd, &pt)) {
                PointI pti(pt.x, pt.y);
                PageElement *pageEl = win.dm->GetElementAtPos(pti);
                if (pageEl) {
                    ScopedMem<TCHAR> text(pageEl->GetValue());
                    RectI rc = win.dm->CvtToScreen(pageEl->GetPageNo(), pageEl->GetRect());
                    win.CreateInfotip(text, rc, true);

                    bool isLink = pageEl->AsLink() != NULL;
                    delete pageEl;

                    if (isLink) {
                        SetCursor(gCursorHand);
                        return TRUE;
                    }
                }
                else
                    win.DeleteInfotip();
                if (win.dm->IsOverText(pti))
                    SetCursor(gCursorIBeam);
                else
                    SetCursor(gCursorArrow);
                return TRUE;
            }
            win.DeleteInfotip();
    }
    if (win.presentation)
        return TRUE;
    return FALSE;
}

static void OnTimer(WindowInfo& win, HWND hwnd, WPARAM timerId)
{
    POINT pt;

    switch (timerId) {
    case REPAINT_TIMER_ID:
        win.delayedRepaintTimer = 0;
        KillTimer(hwnd, REPAINT_TIMER_ID);
        win.RedrawAll();
        break;

    case SMOOTHSCROLL_TIMER_ID:
        if (MA_SCROLLING == win.mouseAction)
            win.MoveDocBy(win.xScrollSpeed, win.yScrollSpeed);
        else if (MA_SELECTING == win.mouseAction || MA_SELECTING_TEXT == win.mouseAction) {
            GetCursorPos(&pt);
            ScreenToClient(win.hwndCanvas, &pt);
            OnMouseMove(win, pt.x, pt.y, MK_CONTROL);
        }
        else {
            KillTimer(hwnd, SMOOTHSCROLL_TIMER_ID);
            win.yScrollSpeed = 0;
            win.xScrollSpeed = 0;
        }
        break;

    case HIDE_CURSOR_TIMER_ID:
        KillTimer(hwnd, HIDE_CURSOR_TIMER_ID);
        if (win.presentation)
            SetCursor(NULL);
        break;

    case HIDE_FWDSRCHMARK_TIMER_ID:
        win.fwdSearchMark.hideStep++;
        if (1 == win.fwdSearchMark.hideStep) {
            SetTimer(hwnd, HIDE_FWDSRCHMARK_TIMER_ID, HIDE_FWDSRCHMARK_DECAYINTERVAL_IN_MS, NULL);
        }
        else if (win.fwdSearchMark.hideStep >= HIDE_FWDSRCHMARK_STEPS) {
            KillTimer(hwnd, HIDE_FWDSRCHMARK_TIMER_ID);
            win.fwdSearchMark.show = false;
            win.RepaintAsync();
        }
        else
            win.RepaintAsync();
        break;

    case AUTO_RELOAD_TIMER_ID:
        KillTimer(hwnd, AUTO_RELOAD_TIMER_ID);
        ReloadDocument(&win, true);
        break;

    case DIR_STRESS_TIMER_ID:
        win.stressTest->Callback();
        break;
    }
}

// these can be global, as the mouse wheel can't affect more than one window at once
static int  gDeltaPerLine = 0;         // for mouse wheel logic
static bool gWheelMsgRedirect = false; // set when WM_MOUSEWHEEL has been passed on (to prevent recursion)

static LRESULT CanvasOnMouseWheel(WindowInfo& win, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!win.IsDocLoaded())
        return 0;

    // Scroll the ToC sidebar, if it's visible and the cursor is in it
    if (win.tocVisible && IsCursorOverWindow(win.hwndTocTree) && !gWheelMsgRedirect) {
        // Note: hwndTocTree's window procedure doesn't always handle
        //       WM_MOUSEWHEEL and when it's bubbling up, we'd return
        //       here recursively - prevent that
        gWheelMsgRedirect = true;
        LRESULT res = SendMessage(win.hwndTocTree, message, wParam, lParam);
        gWheelMsgRedirect = false;
        return res;
    }

    short delta = GET_WHEEL_DELTA_WPARAM(wParam);

    // Note: not all mouse drivers correctly report the Ctrl key's state
    if ((LOWORD(wParam) & MK_CONTROL) || IsCtrlPressed() || (LOWORD(wParam) & MK_RBUTTON)) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(win.hwndCanvas, &pt);

        float factor = delta < 0 ? ZOOM_OUT_FACTOR : ZOOM_IN_FACTOR;
        win.dm->ZoomBy(factor, &PointI(pt.x, pt.y));
        UpdateToolbarState(&win);
#ifdef BUILD_RIBBON
        if (win.ribbonSupport)
            win.ribbonSupport->UpdateState();
#endif

        // don't show the context menu when zooming with the right mouse-button down
        if ((LOWORD(wParam) & MK_RBUTTON))
            win.dragStartPending = false;

        return 0;
    }

    // make sure to scroll whole pages in non-continuous Fit Content mode
    if (!displayModeContinuous(win.dm->displayMode()) &&
        ZOOM_FIT_CONTENT == win.dm->ZoomVirtual()) {
        if (delta > 0)
            win.dm->GoToPrevPage(0);
        else
            win.dm->GoToNextPage(0);
        return 0;
    }

    if (gDeltaPerLine == 0)
       return 0;

    if (gDeltaPerLine < 0) {
        // scroll by (fraction of a) page
        SCROLLINFO si = { 0 };
        si.cbSize = sizeof(si);
        si.fMask  = SIF_PAGE;
        GetScrollInfo(win.hwndCanvas, SB_VERT, &si);
        win.dm->ScrollYBy(-MulDiv(si.nPage, delta, WHEEL_DELTA), true);
        return 0;
    }

    win.wheelAccumDelta += delta;
    int currentScrollPos = GetScrollPos(win.hwndCanvas, SB_VERT);

    while (win.wheelAccumDelta >= gDeltaPerLine) {
        SendMessage(win.hwndCanvas, WM_VSCROLL, SB_LINEUP, 0);
        win.wheelAccumDelta -= gDeltaPerLine;
    }
    while (win.wheelAccumDelta <= -gDeltaPerLine) {
        SendMessage(win.hwndCanvas, WM_VSCROLL, SB_LINEDOWN, 0);
        win.wheelAccumDelta += gDeltaPerLine;
    }

    if (!displayModeContinuous(win.dm->displayMode()) &&
        GetScrollPos(win.hwndCanvas, SB_VERT) == currentScrollPos) {
        if (delta > 0)
            win.dm->GoToPrevPage(-1);
        else
            win.dm->GoToNextPage(0);
    }

    return 0;
}

static LRESULT OnGesture(WindowInfo& win, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!Touch::SupportsGestures())
        return DefWindowProc(win.hwndFrame, message, wParam, lParam);

    HGESTUREINFO hgi = (HGESTUREINFO)lParam;
    GESTUREINFO gi = { 0 };
    gi.cbSize = sizeof(GESTUREINFO);

    BOOL ok = Touch::GetGestureInfo(hgi, &gi);
    if (!ok || !win.IsDocLoaded()) {
        Touch::CloseGestureInfoHandle(hgi);
        return 0;
    }

    switch (gi.dwID) {
        case GID_ZOOM:
            if (gi.dwFlags != GF_BEGIN) {
                float zoom = (float)LODWORD(gi.ullArguments) / (float)win.startArg;
                ZoomToSelection(&win, zoom, true);
            }
            win.startArg = LODWORD(gi.ullArguments);
            break;

        case GID_PAN:
            // Flicking left or right changes the page,
            // panning moves the document in the scroll window
            if (gi.dwFlags == GF_BEGIN) {
                win.panStarted = true;
                win.panPos = gi.ptsLocation;
                win.panScrollOrig.x = GetScrollPos(win.hwndCanvas, SB_HORZ);
                win.panScrollOrig.y = GetScrollPos(win.hwndCanvas, SB_VERT);

                if (GetCursor())
                    SetCursor(gCursorDrag);
            }
            else if (win.panStarted) {
                int deltaX = win.panPos.x - gi.ptsLocation.x;
                int deltaY = win.panPos.y - gi.ptsLocation.y;
                win.panPos = gi.ptsLocation;

                if ((gi.dwFlags & GF_INERTIA) && abs(deltaX) > abs(deltaY)) {
                    // Switch pages once we hit inertia in a horizontal direction
                    if (deltaX < 0)
                        win.dm->GoToPrevPage(0);
                    else if (deltaX > 0)
                        win.dm->GoToNextPage(0);
                    // When we switch pages, go back to the initial scroll position
                    // and prevent further pan movement caused by the inertia
                    win.dm->ScrollXTo(win.panScrollOrig.x);
                    win.dm->ScrollYTo(win.panScrollOrig.y);
                    win.panStarted = false;
                }
                else {
                    // Pan/Scroll
                    win.MoveDocBy(deltaX, deltaY);
                }

                if ((!win.panStarted || (gi.dwFlags & GF_END)) && GetCursor())
                    SetCursor(gCursorArrow);
            }
            break;

        case GID_ROTATE:
            // Rotate the PDF 90 degrees in one direction
            if (gi.dwFlags == GF_END) {
                // This is in radians
                double rads = GID_ROTATE_ANGLE_FROM_ARGUMENT(LODWORD(gi.ullArguments));
                // The angle from the rotate is the opposite of the Sumatra rotate, thus the negative
                double degrees = -rads * 180 / M_PI;

                // Playing with the app, I found that I often didn't go quit a full 90 or 180
                // degrees. Allowing rotate without a full finger rotate seemed more natural.
                if (degrees < -120 || degrees > 120)
                    win.dm->RotateBy(180);
                else if (degrees < -45)
                    win.dm->RotateBy(-90);
                else if (degrees > 45)
                    win.dm->RotateBy(90);
            }
            break;

        case GID_TWOFINGERTAP:
            // Two-finger tap toggles fullscreen mode
            OnMenuViewFullscreen(win);
            break;

        case GID_PRESSANDTAP:
            // Toggle between Fit Page, Fit Width and Fit Content (same as 'z')
            if (gi.dwFlags == GF_BEGIN)
                win.ToggleZoom();
            break;

        default:
            // A gesture was not recognized
            break;
    }

    Touch::CloseGestureInfoHandle(hgi);
    return 0;
}

static LRESULT CALLBACK WndProcCanvas(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // messages that don't require win
    switch (msg) {
        case WM_DROPFILES:
            OnDropFiles((HDROP)wParam);
            break;

        case WM_ERASEBKGND:
            // do nothing, helps to avoid flicker
            return TRUE;
    }

    WindowInfo *win = FindWindowInfoByHwnd(hwnd);
    if (!win)
        return DefWindowProc(hwnd, msg, wParam, lParam);

    // messages that require win
    switch (msg) {
        case WM_VSCROLL:
            OnVScroll(*win, wParam);
            break;

        case WM_HSCROLL:
            OnHScroll(*win, wParam);
            break;

        case WM_MOUSEMOVE:
            OnMouseMove(*win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
            break;

        case WM_LBUTTONDBLCLK:
            OnMouseLeftButtonDblClk(*win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
            break;

        case WM_LBUTTONDOWN:
            OnMouseLeftButtonDown(*win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
            break;

        case WM_LBUTTONUP:
            OnMouseLeftButtonUp(*win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
            break;

        case WM_MBUTTONDOWN:
            if (win->IsDocLoaded()) {
                SetTimer(hwnd, SMOOTHSCROLL_TIMER_ID, SMOOTHSCROLL_DELAY_IN_MS, NULL);
                // TODO: Create window that shows location of initial click for reference
                OnMouseMiddleButtonDown(*win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
            }
            break;

        case WM_RBUTTONDBLCLK:
            OnMouseRightButtonDblClick(*win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
            break;

        case WM_RBUTTONDOWN:
            OnMouseRightButtonDown(*win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
            break;

        case WM_RBUTTONUP:
            OnMouseRightButtonUp(*win, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), wParam);
            break;

        case WM_CONTEXTMENU:
            if (win->IsDocLoaded())
                OnContextMenu(win, 0, 0);
            else
                OnAboutContextMenu(win, 0, 0);
            break;

        case WM_SETCURSOR:
            if (OnSetCursor(*win, hwnd))
                return TRUE;
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_TIMER:
            OnTimer(*win, hwnd, wParam);
            break;

        case WM_PAINT:
            OnPaint(*win);
            break;

        case WM_SIZE:
            if (!IsIconic(win->hwndFrame))
                win->UpdateCanvasSize();
            break;

        case WM_MOUSEWHEEL:
            return CanvasOnMouseWheel(*win, msg, wParam, lParam);

        case WM_GESTURE:
            return OnGesture(*win, msg, wParam, lParam);

        default:
            // process thread queue events happening during an inner message loop
            // (else the scrolling position isn't updated until the scroll bar is released)
            gUIThreadMarshaller.Execute();
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

class RepaintCanvasWorkItem : public UIThreadWorkItem
{
    UINT delay;

public:
    RepaintCanvasWorkItem(WindowInfo *win, UINT delay) 
        : UIThreadWorkItem(win), delay(delay)
    {}

    virtual void Execute() {
        if (!WindowInfoStillValid(win))
            return;
        if (!delay)
            WndProcCanvas(win->hwndCanvas, WM_TIMER, REPAINT_TIMER_ID, 0);
        else if (!win->delayedRepaintTimer)
            win->delayedRepaintTimer = SetTimer(win->hwndCanvas, REPAINT_TIMER_ID, delay, NULL);
    }
};

void WindowInfo::RepaintAsync(UINT delay)
{
    // even though RepaintAsync is mostly called from the UI thread,
    // we depend on the repaint message to happen asynchronously
    // and let QueueWorkItem call PostMessage for us
    QueueWorkItem(new RepaintCanvasWorkItem(this, delay));
}

static LRESULT FrameOnCommand(WindowInfo *win, HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    int wmId = LOWORD(wParam);

    // check if the menuId belongs to an entry in the list of
    // recently opened files and load the referenced file if it does
    if ((wmId >= IDM_FILE_HISTORY_FIRST) && (wmId <= IDM_FILE_HISTORY_LAST))
    {
        DisplayState *state = gFileHistory.Get(wmId - IDM_FILE_HISTORY_FIRST);
        if (state) {
            if (HasPermission(Perm_DiskAccess))
                LoadDocument(state->filePath, win);
            return 0;
        }
    }

    // 10 submenus max with 10 items each max (=100) plus generous buffer => 200
    CASSERT(IDM_FAV_LAST - IDM_FAV_FIRST == 200, enough_fav_menu_ids);
    if ((wmId >= IDM_FAV_FIRST) && (wmId <= IDM_FAV_LAST))
    {
        GoToFavoriteByMenuId(win, wmId);
    }

    if (!win)
        return DefWindowProc(hwnd, msg, wParam, lParam);

    // most of them require a win, the few exceptions are no-ops without
    switch (wmId)
    {
        case IDM_OPEN:
        case IDT_FILE_OPEN:
            OnMenuOpen(*win);
            break;
        case IDM_SAVEAS:
            OnMenuSaveAs(*win);
            break;

        case IDT_FILE_PRINT:
        case IDM_PRINT:
            OnMenuPrint(win);
            break;

        case IDT_FILE_EXIT:
        case IDM_CLOSE:
            CloseWindow(win, false);
            break;

        case IDM_EXIT:
            OnMenuExit();
            break;

        case IDM_REFRESH:
            ReloadDocument(win);
            break;

        case IDM_SAVEAS_BOOKMARK:
            OnMenuSaveBookmark(*win);
            break;

        case IDT_VIEW_FIT_WIDTH:
            ChangeZoomLevel(win, ZOOM_FIT_WIDTH, true);
            break;

        case IDT_VIEW_FIT_PAGE:
            ChangeZoomLevel(win, ZOOM_FIT_PAGE, false);
            break;

        case IDT_VIEW_ZOOMIN:
            ZoomToSelection(win, ZOOM_IN_FACTOR, true);
            break;

        case IDT_VIEW_ZOOMOUT:
            ZoomToSelection(win, ZOOM_OUT_FACTOR, true);
            break;

        case IDM_ZOOM_6400:
        case IDM_ZOOM_3200:
        case IDM_ZOOM_1600:
        case IDM_ZOOM_800:
        case IDM_ZOOM_400:
        case IDM_ZOOM_200:
        case IDM_ZOOM_150:
        case IDM_ZOOM_125:
        case IDM_ZOOM_100:
        case IDM_ZOOM_50:
        case IDM_ZOOM_25:
        case IDM_ZOOM_12_5:
        case IDM_ZOOM_8_33:
        case IDM_ZOOM_FIT_PAGE:
        case IDM_ZOOM_FIT_WIDTH:
        case IDM_ZOOM_FIT_CONTENT:
        case IDM_ZOOM_ACTUAL_SIZE:
            OnMenuZoom(win, wmId);
            break;

        case IDM_ZOOM_CUSTOM:
            OnMenuCustomZoom(win);
            break;

        case IDM_VIEW_SINGLE_PAGE:
            SwitchToDisplayMode(win, DM_SINGLE_PAGE, true);
            break;

        case IDM_VIEW_FACING:
            SwitchToDisplayMode(win, DM_FACING, true);
            break;

        case IDM_VIEW_BOOK:
            SwitchToDisplayMode(win, DM_BOOK_VIEW, true);
            break;

        case IDM_VIEW_CONTINUOUS:
            OnMenuViewContinuous(*win);
            break;

        case IDM_VIEW_SHOW_HIDE_TOOLBAR:
            OnMenuViewShowHideToolbar();
            break;

        case IDM_CHANGE_LANGUAGE:
            OnMenuChangeLanguage(*win);
            break;

        case IDM_VIEW_BOOKMARKS:
            ToggleTocBox(win);
            break;

        case IDM_GOTO_NEXT_PAGE:
            if (win->IsDocLoaded())
                win->dm->GoToNextPage(0);
            break;

        case IDM_GOTO_PREV_PAGE:
            if (win->IsDocLoaded())
                win->dm->GoToPrevPage(0);
            break;

        case IDM_GOTO_FIRST_PAGE:
            if (win->IsDocLoaded())
                win->dm->GoToFirstPage();
            break;

        case IDM_GOTO_LAST_PAGE:
            if (win->IsDocLoaded())
                win->dm->GoToLastPage();
            break;

        case IDM_GOTO_PAGE:
            OnMenuGoToPage(*win);
            break;

        case IDM_VIEW_PRESENTATION_MODE:
            OnMenuViewPresentation(*win);
            break;

        case IDM_VIEW_FULLSCREEN:
            OnMenuViewFullscreen(*win);
            break;

        case IDM_VIEW_ROTATE_LEFT:
            if (win->IsDocLoaded())
                win->dm->RotateBy(-90);
            break;

        case IDM_VIEW_ROTATE_RIGHT:
            if (win->IsDocLoaded())
                win->dm->RotateBy(90);
            break;

        case IDM_FIND_FIRST:
            OnMenuFind(win);
            break;

        case IDM_FIND_NEXT:
            OnMenuFindNext(win);
            break;

        case IDM_FIND_PREV:
            OnMenuFindPrev(win);
            break;

        case IDM_FIND_MATCH:
            OnMenuFindMatchCase(win);
            break;

        case IDM_VISIT_WEBSITE:
            LaunchBrowser(_T("http://blog.kowalczyk.info/software/sumatrapdf/"));
            break;

        case IDM_MANUAL:
            LaunchBrowser(_T("http://blog.kowalczyk.info/software/sumatrapdf/manual.html"));
            break;

        case IDM_CONTRIBUTE_TRANSLATION:
            LaunchBrowser(_T("http://blog.kowalczyk.info/software/sumatrapdf/translations.html"));
            break;

        case IDM_ABOUT:
            OnMenuAbout();
            break;

        case IDM_CHECK_UPDATE:
            DownloadSumatraUpdateInfo(*win, false);
            break;

        case IDM_SETTINGS:
            OnMenuSettings(*win);
            break;

        case IDM_VIEW_WITH_ACROBAT:
            ViewWithAcrobat(win);
            break;

        case IDM_VIEW_WITH_FOXIT:
            ViewWithFoxit(win);
            break;

        case IDM_VIEW_WITH_PDF_XCHANGE:
            ViewWithPDFXChange(win);
            break;

        case IDM_SEND_BY_EMAIL:
            SendAsEmailAttachment(win);
            break;

        case IDM_PROPERTIES:
            OnMenuProperties(*win);
            break;

        case IDM_MOVE_FRAME_FOCUS:
            if (win->hwndFrame != GetFocus())
                SetFocus(win->hwndFrame);
            else if (win->tocVisible)
                SetFocus(win->hwndTocTree);
            break;

        case IDM_GOTO_NAV_BACK:
            if (win->IsDocLoaded())
                win->dm->Navigate(-1);
            break;

        case IDM_GOTO_NAV_FORWARD:
            if (win->IsDocLoaded())
                win->dm->Navigate(1);
            break;

        case IDM_COPY_SELECTION:
            // Don't break the shortcut for text boxes
            if (win->hwndFindBox == GetFocus() || win->hwndPageBox == GetFocus())
                SendMessage(GetFocus(), WM_COPY, 0, 0);
            else if (win->hwndProperties == GetForegroundWindow())
                CopyPropertiesToClipboard(win->hwndProperties);
            else if (!HasPermission(Perm_CopySelection))
                break;
            else if (win->IsChm())
                win->dm->AsChmEngine()->CopySelection();
            else if (win->selectionOnPage)
                CopySelectionToClipboard(win);
            else
                ShowNotification(win, _TR("Select content with Ctrl+left mouse button"));
            break;

        case IDM_SELECT_ALL:
            OnSelectAll(win);
            break;

        case IDM_DEBUG_SHOW_LINKS:
            gDebugShowLinks = !gDebugShowLinks;
            for (size_t i = 0; i < gWindows.Count(); i++) {
                gWindows.At(i)->RedrawAll(true);
#ifdef BUILD_RIBBON
                if (gWindows.At(i)->ribbonSupport)
                    gWindows.At(i)->ribbonSupport->UpdateState();
#endif
            }
            break;

        case IDM_DEBUG_GDI_RENDERER:
            ToggleGdiDebugging();
            break;

        case IDM_DEBUG_CRASH_ME:
            CrashMe();
            break;

        case IDM_FAV_ADD:
            AddFavorite(win);
            break;

        case IDM_FAV_DEL:
            DelFavorite(win);
            break;

        case IDM_FAV_TOGGLE:
            ToggleFavorites(win);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    return 0;
}

static LRESULT CALLBACK WndProcFrame(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WindowInfo *    win;
    ULONG           ulScrollLines;                   // for mouse wheel logic

    win = FindWindowInfoByHwnd(hwnd);

    switch (msg)
    {
        case WM_CREATE:
            // do nothing
            goto InitMouseWheelInfo;

        case WM_SIZE:
            if (win && SIZE_MINIMIZED != wParam) {
                RememberWindowPosition(*win);
                AdjustWindowEdge(*win);

                int dx = LOWORD(lParam);
                int dy = HIWORD(lParam);
                FrameOnSize(win, dx, dy);
            }
            break;

        case WM_MOVE:
            if (win) {
                RememberWindowPosition(*win);
                AdjustWindowEdge(*win);
            }
            break;

        case WM_INITMENUPOPUP:
            UpdateMenu(win, (HMENU)wParam);
            break;

        case WM_COMMAND:
            return FrameOnCommand(win, hwnd, msg, wParam, lParam);

        case WM_APPCOMMAND:
            // both keyboard and mouse drivers should produce WM_APPCOMMAND
            // messages for their special keys, so handle these here and return
            // TRUE so as to not make them bubble up further
            switch (GET_APPCOMMAND_LPARAM(lParam)) {
            case APPCOMMAND_BROWSER_BACKWARD:
                SendMessage(hwnd, WM_COMMAND, IDM_GOTO_NAV_BACK, 0);
                return TRUE;
            case APPCOMMAND_BROWSER_FORWARD:
                SendMessage(hwnd, WM_COMMAND, IDM_GOTO_NAV_FORWARD, 0);
                return TRUE;
            case APPCOMMAND_BROWSER_REFRESH:
                SendMessage(hwnd, WM_COMMAND, IDM_REFRESH, 0);
                return TRUE;
            case APPCOMMAND_BROWSER_SEARCH:
                SendMessage(hwnd, WM_COMMAND, IDM_FIND_FIRST, 0);
                return TRUE;
            case APPCOMMAND_BROWSER_FAVORITES:
                SendMessage(hwnd, WM_COMMAND, IDM_VIEW_BOOKMARKS, 0);
                return TRUE;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_CHAR:
            if (win)
                FrameOnChar(*win, wParam);
            break;

        case WM_KEYDOWN:
            if (win)
                FrameOnKeydown(win, wParam, lParam);
            break;

        case WM_CONTEXTMENU:
            // opening the context menu with a keyboard doesn't call the canvas'
            // WM_CONTEXTMENU, as it never has the focus (mouse right-clicks are
            // handled as expected)
            if (win && GET_X_LPARAM(lParam) == -1 && GET_Y_LPARAM(lParam) == -1 && GetFocus() != win->hwndTocTree)
                return SendMessage(win->hwndCanvas, WM_CONTEXTMENU, wParam, lParam);
            return DefWindowProc(hwnd, msg, wParam, lParam);

        case WM_SETTINGCHANGE:
InitMouseWheelInfo:
            SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &ulScrollLines, 0);
            // ulScrollLines usually equals 3 or 0 (for no scrolling) or -1 (for page scrolling)
            // WHEEL_DELTA equals 120, so iDeltaPerLine will be 40
            if (ulScrollLines == (ULONG)-1)
                gDeltaPerLine = -1;
            else if (ulScrollLines != 0)
                gDeltaPerLine = WHEEL_DELTA / ulScrollLines;
            else
                gDeltaPerLine = 0;
            return 0;

        case WM_MOUSEWHEEL:
            if (!win)
                break;

            if (win->IsChm()) {
                ChmEngine *chmEngine = win->dm->AsChmEngine();
                HtmlWindow *htmlWin = chmEngine->GetHtmlWindow();
                htmlWin->SendMsg(msg, wParam, lParam);
                break;
            }

            // Pass the message to the canvas' window procedure
            // (required since the canvas itself never has the focus and thus
            // never receives WM_MOUSEWHEEL messages)
            return SendMessage(win->hwndCanvas, msg, wParam, lParam);

        case WM_DESTROY:
            /* WM_DESTROY might be sent as a result of File\Close, in which case CloseWindow() has already been called */
            if (win)
                CloseWindow(win, true, true);
            break;

        case WM_DDE_INITIATE:
            if (gPluginMode)
                break;
            return OnDDEInitiate(hwnd, wParam, lParam);
        case WM_DDE_EXECUTE:
            return OnDDExecute(hwnd, wParam, lParam);
        case WM_DDE_TERMINATE:
            return OnDDETerminate(hwnd, wParam, lParam);

        case UWM_PREFS_FILE_UPDATED:
            ReloadPrefs();
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

static bool RegisterWinClass(HINSTANCE hinst)
{
    WNDCLASSEX  wcex;
    ATOM        atom;

    FillWndClassEx(wcex, hinst);
    wcex.lpfnWndProc    = WndProcFrame;
    wcex.lpszClassName  = FRAME_CLASS_NAME;
    wcex.hIcon          = LoadIcon(hinst, MAKEINTRESOURCE(IDI_SUMATRAPDF));
    wcex.hIconSm        = LoadIcon(hinst, MAKEINTRESOURCE(IDI_SMALL));
    atom = RegisterClassEx(&wcex);
    if (!atom)
        return false;

    FillWndClassEx(wcex, hinst);
    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc    = WndProcCanvas;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName  = CANVAS_CLASS_NAME;
    atom = RegisterClassEx(&wcex);
    if (!atom)
        return false;

    FillWndClassEx(wcex, hinst);
    wcex.lpfnWndProc    = WndProcAbout;
    wcex.hIcon          = LoadIcon(hinst, MAKEINTRESOURCE(IDI_SUMATRAPDF));
    wcex.lpszClassName  = ABOUT_CLASS_NAME;
    atom = RegisterClassEx(&wcex);
    if (!atom)
        return false;

    FillWndClassEx(wcex, hinst);
    wcex.lpfnWndProc    = WndProcProperties;
    wcex.hIcon          = LoadIcon(hinst, MAKEINTRESOURCE(IDI_SUMATRAPDF));
    wcex.lpszClassName  = PROPERTIES_CLASS_NAME;
    atom = RegisterClassEx(&wcex);
    if (!atom)
        return false;

    FillWndClassEx(wcex, hinst);
    wcex.lpfnWndProc    = WndProcSidebarSplitter;
    wcex.hCursor        = LoadCursor(NULL, IDC_SIZEWE);
    wcex.hbrBackground  = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    wcex.lpszClassName  = SIDEBAR_SPLITTER_CLASS_NAME;
    atom = RegisterClassEx(&wcex);
    if (!atom)
        return false;

    FillWndClassEx(wcex, hinst);
    wcex.lpfnWndProc    = WndProcFavSplitter;
    wcex.hCursor        = LoadCursor(NULL, IDC_SIZENS);
    wcex.hbrBackground  = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
    wcex.lpszClassName  = FAV_SPLITTER_CLASS_NAME;
    atom = RegisterClassEx(&wcex);
    if (!atom)
        return false;

    if (!RegisterNotificationsWndClass(hinst))
        return false;

    return true;
}

static bool InstanceInit(HINSTANCE hInstance, int nCmdShow)
{
    ghinst = hInstance;

    gCursorArrow = LoadCursor(NULL, IDC_ARROW);
    gCursorIBeam = LoadCursor(NULL, IDC_IBEAM);
    gCursorHand  = LoadCursor(NULL, IDC_HAND);
    if (!gCursorHand) // IDC_HAND isn't available if WINVER < 0x0500
        gCursorHand = LoadCursor(ghinst, MAKEINTRESOURCE(IDC_CURSORDRAG));

    gCursorScroll   = LoadCursor(NULL, IDC_SIZEALL);
    gCursorDrag     = LoadCursor(ghinst, MAKEINTRESOURCE(IDC_CURSORDRAG));
    gCursorSizeWE   = LoadCursor(NULL, IDC_SIZEWE);
    gCursorSizeNS   = LoadCursor(NULL, IDC_SIZENS);
    gCursorNo       = LoadCursor(NULL, IDC_NO);
    gBrushNoDocBg   = CreateSolidBrush(COL_WINDOW_BG);
    if (ABOUT_BG_COLOR_DEFAULT != gGlobalPrefs.bgColor)
        gBrushAboutBg = CreateSolidBrush(gGlobalPrefs.bgColor);
    else
        gBrushAboutBg = CreateSolidBrush(ABOUT_BG_COLOR);
    gBrushWhite     = GetStockBrush(WHITE_BRUSH);
    gBrushBlack     = GetStockBrush(BLACK_BRUSH);

    NONCLIENTMETRICS ncm = { 0 };
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    gDefaultGuiFont = CreateFontIndirect(&ncm.lfMessageFont);
    gBitmapReloadingCue = LoadBitmap(ghinst, MAKEINTRESOURCE(IDB_RELOADING_CUE));

    return true;
}

void UIThreadWorkItemQueue::Queue(UIThreadWorkItem *item)
{
    if (!item)
        return;

    ScopedCritSec scope(&cs);
    items.Append(item);

    if (item->win) {
        // hwndCanvas is less likely to enter internal message pump (during which
        // the messages are not visible to our processing in top-level message pump)
        PostMessage(item->win->hwndCanvas, WM_NULL, 0, 0);
    }
}

static void MakePluginWindow(WindowInfo& win, HWND hwndParent)
{
    assert(IsWindow(hwndParent));
    assert(gPluginMode);

    long ws = GetWindowLong(win.hwndFrame, GWL_STYLE);
    ws &= ~(WS_POPUP | WS_BORDER | WS_CAPTION | WS_THICKFRAME);
    ws |= WS_CHILD;
    SetWindowLong(win.hwndFrame, GWL_STYLE, ws);

    SetParent(win.hwndFrame, hwndParent);
    MoveWindow(win.hwndFrame, ClientRect(hwndParent));
    ShowWindow(win.hwndFrame, SW_SHOW);
    UpdateWindow(win.hwndFrame);

    // from here on, we depend on the plugin's host to resize us
    SetFocus(win.hwndFrame);
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    MSG msg = { 0 };

#ifdef DEBUG
    // Memory leak detection
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(421);
#endif

    EnableNx();

    // ensure that C functions behave consistently under all OS locales
    // (use Win32 functions where localized input or output is desired)
    setlocale(LC_ALL, "C");

#ifdef DEBUG
    extern void BaseUtils_UnitTests();
    BaseUtils_UnitTests();
    extern void TrivialHtmlParser_UnitTests();
    TrivialHtmlParser_UnitTests();
    extern void SumatraPDF_UnitTests();
    SumatraPDF_UnitTests();
#endif

    // don't show system-provided dialog boxes when accessing files on drives
    // that are not mounted (e.g. a: drive without floppy or cd rom drive
    // without a cd).
    SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);
    srand((unsigned int)time(NULL));

    ScopedMem<TCHAR> crashDumpPath(AppGenDataFilename(CRASH_DUMP_FILE_NAME));
    InstallCrashHandler(crashDumpPath);

    ScopedOle ole;
    InitAllCommonControls();
    ScopedGdiPlus gdiPlus(true);

    ScopedMem<TCHAR> prefsFilename(GetPrefsFileName());
    if (!file::Exists(prefsFilename)) {
        // guess the ui language on first start
        CurrLangNameSet(Trans::GuessLanguage());
        gFavorites = new Favorites();
    } else {
        assert(gFavorites == NULL);
        Prefs::Load(prefsFilename, gGlobalPrefs, gFileHistory, &gFavorites);
        CurrLangNameSet(gGlobalPrefs.currentLanguage);
    }
    prefsFilename.Set(NULL);

    CommandLineInfo i;
    i.bgColor = gGlobalPrefs.bgColor;
    i.fwdSearch.offset = gGlobalPrefs.fwdSearch.offset;
    i.fwdSearch.width = gGlobalPrefs.fwdSearch.width;
    i.fwdSearch.color = gGlobalPrefs.fwdSearch.color;
    i.fwdSearch.permanent = gGlobalPrefs.fwdSearch.permanent;
    i.escToExit = gGlobalPrefs.escToExit;

    i.ParseCommandLine(GetCommandLine());

    if (i.showConsole)
        RedirectIOToConsole();
    if (i.makeDefault)
        AssociateExeWithPdfExtension();
    if (i.filesToBenchmark.Count() > 0) {
        Bench(i.filesToBenchmark);
        if (i.showConsole)
            system("pause");
    }
    if (i.exitImmediately)
        goto Exit;
    gCrashOnOpen = i.crashOnOpen;

    gGlobalPrefs.bgColor = i.bgColor;
    gGlobalPrefs.fwdSearch.offset = i.fwdSearch.offset;
    gGlobalPrefs.fwdSearch.width = i.fwdSearch.width;
    gGlobalPrefs.fwdSearch.color = i.fwdSearch.color;
    gGlobalPrefs.fwdSearch.permanent = i.fwdSearch.permanent;
    gGlobalPrefs.escToExit = i.escToExit;
    gPolicyRestrictions = GetPolicies(i.restrictedUse);
    gRenderCache.invertColors = i.invertColors;
    DebugGdiPlusDevice(gUseGdiRenderer);

    if (i.inverseSearchCmdLine) {
        str::ReplacePtr(&gGlobalPrefs.inverseSearchCmdLine, i.inverseSearchCmdLine);
        gGlobalPrefs.enableTeXEnhancements = true;
    }
    CurrLangNameSet(i.lang);

    msg.wParam = 1; // set an error code, in case we prematurely have to goto Exit
    if (!RegisterWinClass(hInstance))
        goto Exit;
    if (!InstanceInit(hInstance, nCmdShow))
        goto Exit;

    if (i.hwndPluginParent) {
        if (!IsWindow(i.hwndPluginParent) || i.fileNames.Count() == 0)
            goto Exit;

        gPluginURL = i.pluginURL;
        if (!gPluginURL)
            gPluginURL = i.fileNames.At(0);

        assert(i.fileNames.Count() == 1);
        while (i.fileNames.Count() > 1)
            free(i.fileNames.Pop());
        i.reuseInstance = i.exitOnPrint = false;
        // always display the toolbar when embedded (as there's no menubar in that case)
        gGlobalPrefs.toolbarVisible = true;
#ifdef BUILD_RIBBON
        // always use the classic toolbar for the plugin
        gGlobalPrefs.useRibbon = false;
#endif
        // never allow esc as a shortcut to quit
        gGlobalPrefs.escToExit = false;
        // never show the sidebar by default
        gGlobalPrefs.tocVisible = false;
        if (DM_AUTOMATIC == gGlobalPrefs.defaultDisplayMode) {
            // if the user hasn't changed the default display mode,
            // display documents as single page/continuous/fit width
            // (similar to Adobe Reader, Google Chrome and how browsers display HTML)
            gGlobalPrefs.defaultDisplayMode = DM_CONTINUOUS;
            gGlobalPrefs.defaultZoom = ZOOM_FIT_WIDTH;
        }
    }

    WindowInfo *win = NULL;
    bool firstIsDocLoaded = false;
    msg.wParam = 0;

    if (i.printerName) {
        // note: this prints all PDF files. Another option would be to
        // print only the first one
        for (size_t n = 0; n < i.fileNames.Count(); n++) {
            bool ok = PrintFile(i.fileNames.At(n), i.printerName, !i.silent, i.printSettings);
            if (!ok)
                msg.wParam++;
        }
        goto Exit;
    }

    if (i.fileNames.Count() == 0 && gGlobalPrefs.rememberOpenedFiles && gGlobalPrefs.showStartPage) {
        // make the shell prepare the image list, so that it's ready when the first window's loaded
        SHFILEINFO sfi;
        SHGetFileInfo(_T(".pdf"), 0, &sfi, sizeof(sfi), SHGFI_SYSICONINDEX | SHGFI_SMALLICON | SHGFI_USEFILEATTRIBUTES);
    }

    for (size_t n = 0; n < i.fileNames.Count(); n++) {
        if (i.reuseInstance && !i.printDialog) {
            // delegate file opening to a previously running instance by sending a DDE message 
            TCHAR fullpath[MAX_PATH];
            GetFullPathName(i.fileNames.At(n), dimof(fullpath), fullpath, NULL);
            ScopedMem<TCHAR> command(str::Format(_T("[") DDECOMMAND_OPEN _T("(\"%s\", 0, 1, 0)]"), fullpath));
            DDEExecute(PDFSYNC_DDE_SERVICE, PDFSYNC_DDE_TOPIC, command);
            if (i.destName && !firstIsDocLoaded) {
                ScopedMem<TCHAR> command(str::Format(_T("[") DDECOMMAND_GOTO _T("(\"%s\", \"%s\")]"), fullpath, i.destName));
                DDEExecute(PDFSYNC_DDE_SERVICE, PDFSYNC_DDE_TOPIC, command);
            }
            else if (i.pageNumber > 0 && !firstIsDocLoaded) {
                ScopedMem<TCHAR> command(str::Format(_T("[") DDECOMMAND_PAGE _T("(\"%s\", %d)]"), fullpath, i.pageNumber));
                DDEExecute(PDFSYNC_DDE_SERVICE, PDFSYNC_DDE_TOPIC, command);
            }
            if ((i.startView != DM_AUTOMATIC || i.startZoom != INVALID_ZOOM ||
                 i.startScroll.x != -1 && i.startScroll.y != -1) && !firstIsDocLoaded) {
                const TCHAR *viewMode = DisplayModeConv::NameFromEnum(i.startView);
                ScopedMem<TCHAR> command(str::Format(_T("[") DDECOMMAND_SETVIEW _T("(\"%s\", \"%s\", %.2f, %d, %d)]"),
                                         fullpath, viewMode, i.startZoom, i.startScroll.x, i.startScroll.y));
                DDEExecute(PDFSYNC_DDE_SERVICE, PDFSYNC_DDE_TOPIC, command);
            }
            if (i.forwardSearchOrigin && i.forwardSearchLine) {
                ScopedMem<TCHAR> command(str::Format(_T("[") DDECOMMAND_SYNC _T("(\"%s\", \"%s\", %d, 0, 0, 1)]"),
                                         i.fileNames.At(n), i.forwardSearchOrigin, i.forwardSearchLine));
                DDEExecute(PDFSYNC_DDE_SERVICE, PDFSYNC_DDE_TOPIC, command);
            }
        }
        else {
            bool showWin = !(i.printDialog && i.exitOnPrint) && !gPluginMode;
            win = LoadDocument(i.fileNames.At(n), NULL, showWin);
            if (!win || !win->IsDocLoaded())
                msg.wParam++; // set an error code for the next goto Exit
            if (!win)
                goto Exit;
            if (win->IsDocLoaded() && i.destName && !firstIsDocLoaded) {
                win->linkHandler->GotoNamedDest(i.destName);
            }
            else if (win->IsDocLoaded() && i.pageNumber > 0 && !firstIsDocLoaded) {
                if (win->dm->ValidPageNo(i.pageNumber))
                    win->dm->GoToPage(i.pageNumber, 0);
            }
            if (i.hwndPluginParent)
                MakePluginWindow(*win, i.hwndPluginParent);
            if (win->IsDocLoaded() && !firstIsDocLoaded) {
                if (i.enterPresentation || i.enterFullscreen)
                    EnterFullscreen(*win, i.enterPresentation);
                if (i.startView != DM_AUTOMATIC)
                    SwitchToDisplayMode(win, i.startView);
                if (i.startZoom != INVALID_ZOOM)
                    ZoomToSelection(win, i.startZoom, false);
                if (i.startScroll.x != -1 || i.startScroll.y != -1) {
                    ScrollState ss = win->dm->GetScrollState();
                    ss.x = i.startScroll.x;
                    ss.y = i.startScroll.y;
                    win->dm->SetScrollState(ss);
                }
                if (i.forwardSearchOrigin && i.forwardSearchLine && win->pdfsync) {
                    UINT page;
                    Vec<RectI> rects;
                    int ret = win->pdfsync->SourceToDoc(i.forwardSearchOrigin, i.forwardSearchLine, 0, &page, rects);
                    ShowForwardSearchResult(win, i.forwardSearchOrigin, i.forwardSearchLine, 0, ret, page, rects);
                }
            }
        }

        if (i.printDialog)
            OnMenuPrint(win, i.exitOnPrint);
        firstIsDocLoaded = true;
    }

    if (i.reuseInstance || i.printDialog && i.exitOnPrint)
        goto Exit;

    if (!firstIsDocLoaded) {
        bool enterFullscreen = (WIN_STATE_FULLSCREEN == gGlobalPrefs.windowState);
        win = CreateWindowInfo();
        if (!win) {
            msg.wParam = 1;
            goto Exit;
        }

        if (WIN_STATE_FULLSCREEN == gGlobalPrefs.windowState ||
            WIN_STATE_MAXIMIZED == gGlobalPrefs.windowState)
            ShowWindow(win->hwndFrame, SW_MAXIMIZE);
        else
            ShowWindow(win->hwndFrame, SW_SHOW);
        UpdateWindow(win->hwndFrame);

        if (enterFullscreen)
            EnterFullscreen(*win);
    }

    UpdateUITextForLanguage(); // needed for RTL languages
    if (!firstIsDocLoaded)
        UpdateToolbarAndScrollbarState(*win);

    // Make sure that we're still registered as default,
    // if the user has explicitly told us to be
    if (gGlobalPrefs.pdfAssociateShouldAssociate && win)
        RegisterForPdfExtentions(win->hwndFrame);

    if (gGlobalPrefs.enableAutoUpdate && gWindows.Count() > 0)
        DownloadSumatraUpdateInfo(*gWindows.At(0), true);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SUMATRAPDF));
#ifndef THREAD_BASED_FILEWATCH
    const UINT_PTR timerID = SetTimer(NULL, -1, FILEWATCH_DELAY_IN_MS, NULL);
#endif

    if (i.stressTestPath) {
        gIsStressTesting = true;
        StartStressTest(win, i.stressTestPath, i.stressTestFilter,
                        i.stressTestRanges, i.stressTestCycles, &gRenderCache);
    }

    while (GetMessage(&msg, NULL, 0, 0) > 0) {
#ifndef THREAD_BASED_FILEWATCH
        if (NULL == msg.hwnd && WM_TIMER == msg.message && timerID == msg.wParam) {
            RefreshUpdatedFiles();
            continue;
        }
#endif
        // Dispatch the accelerator to the correct window
        win = FindWindowInfoByHwnd(msg.hwnd);
        HWND accHwnd = win ? win->hwndFrame : msg.hwnd;
        if (TranslateAccelerator(accHwnd, hAccelTable, &msg))
            continue;

        TranslateMessage(&msg);
        DispatchMessage(&msg);

        // process these messages here so that we don't have to add this
        // handling to every WndProc that might receive those messages
        // note: this isn't called during an inner message loop, so
        //       Execute() also has to be called from a WndProc
        gUIThreadMarshaller.Execute();
    }

#ifndef THREAD_BASED_FILEWATCH
    KillTimer(NULL, timerID);
#endif

    CleanUpThumbnailCache(gFileHistory);

Exit:
    while (gWindows.Count() > 0)
        DeleteWindowInfo(gWindows.At(0));
    DeleteObject(gBrushNoDocBg);
    DeleteObject(gBrushAboutBg);
    DeleteObject(gBrushWhite);
    DeleteObject(gBrushBlack);
    DeleteObject(gDefaultGuiFont);
    DeleteBitmap(gBitmapReloadingCue);

    delete gFavorites;

    return (int)msg.wParam;
}

/*MyCode*/
SumatraPdfIntf* g_pIntf;

// static PdfObj* ExtraPdfObjects(INT pageNo)
// {
// 	WindowInfo* win = WindowInfo::g_pWinInf;
// 	if(!win)
// 		return NULL;
// 
// 	if(!win->dm || !win->dm->engine)
// 		return NULL;
// 
//  	return win->dm->engine->ExtractObjs(pageNo);
// }

static HPDFOBJ GetFirstPdfObj(INT pageNo)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return NULL;

	if(!win->dm || !win->dm->engine)
		return NULL;

	return win->dm->engine->GetPageFirstObj(pageNo);
}

static HPDFOBJ GetNextPdfObj(HPDFOBJ hObj)
{
	if(!hObj)
		return NULL;

	fz_display_node* node = (fz_display_node*)hObj;
	return (HPDFOBJ)node->next;
}

static SumatraPdfIntf::ObjType GetObjType(HPDFOBJ hObj)
{
	if(!hObj)
		SumatraPdfIntf::OT_Unknown;

	fz_display_node* node = (fz_display_node*)hObj;
	switch(node->cmd)
	{
	case FZ_CMD_FILL_TEXT:
	case FZ_CMD_STROKE_TEXT:
		return SumatraPdfIntf::OT_Text;
		break;
	case FZ_CMD_CLIP_IMAGE_MASK:
	case FZ_CMD_FILL_IMAGE_MASK:
		return SumatraPdfIntf::OT_Image;
		break;
	case FZ_CMD_STROKE_PATH:
		return SumatraPdfIntf::OT_Path;
		break;
	default:
		break;
	}

	return SumatraPdfIntf::OT_Unknown;
}

static WCHAR* ExtractObjLineText(int pageNo, HPDFOBJ hObj, const FPoint* fPt, SumatraPdfIntf::LineTextResult& ltr)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return NULL;

	if(!win->dm || !win->dm->engine)
		return NULL;

	if(!hObj)
		return NULL;

	PointD* pPtD = NULL;
	PointD ptD;
	if(fPt)
	{
		ptD.x = fPt->x;
		ptD.y = fPt->y;
		pPtD = &ptD;
	}

	WCHAR* rText = win->dm->textSelection->ExtractObjLineText(pageNo,hObj,pPtD,ltr);
	return rText;
}

static BOOL MoveObject(int pageNo, HPDFOBJ hObj, const FPoint& relMove)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return FALSE;

	if(!win->dm || !win->dm->engine)
		return FALSE;

	if(!hObj)
		return FALSE;

	BOOL bRet = FALSE;

	SumatraPdfIntf::ObjType ot = GetObjType(hObj);

	if(ot==SumatraPdfIntf::OT_Text)
	{
		bRet = win->dm->textSelection->MoveObject(pageNo,hObj,relMove);
		if(bRet)
		{
			gRenderCache.DropAllCache();
			win->dm->Redraw();
		}
	}
	else if(ot==SumatraPdfIntf::OT_Image)
	{
		fz_display_node* node = (fz_display_node*)hObj;
		if(node->cmd==FZ_CMD_CLIP_IMAGE_MASK)
		{
			if(node->next && node->next->cmd==FZ_CMD_FILL_IMAGE)
			{
				if(AnyEqual(node->rect,node->next->rect))
				{
					BOOL bPopClip = FALSE;
					if(node->next->next && node->next->next->cmd==FZ_CMD_POP_CLIP)
					{
						if(AnyEqual(node->rect,node->next->next->rect))
						{
							bPopClip = TRUE;
						}
					}

					node->rect.x0 += (float)relMove.x;
					node->rect.y0 += (float)relMove.y;
					node->rect.x1 += (float)relMove.x;
					node->rect.y1 += (float)relMove.y;

					node->ctm.e += (float)relMove.x;
					node->ctm.f += (float)relMove.y;

					node->next->rect = node->rect;

					node->next->ctm.e += (float)relMove.x;
					node->next->ctm.f += (float)relMove.y;
	
					if(bPopClip)
						node->next->next->rect = node->rect;
					

					gRenderCache.DropAllCache();
					win->dm->Redraw();

					bRet = TRUE;
				}
			}
		}
	}
	else if(ot==SumatraPdfIntf::OT_Path)
	{
		fz_display_node* node = (fz_display_node*)hObj;
		if(node->cmd==FZ_CMD_STROKE_PATH)
		{
			node->rect.x0 += (float)relMove.x;
			node->rect.y0 += (float)relMove.y;
			node->rect.x1 += (float)relMove.x;
			node->rect.y1 += (float)relMove.y;

			node->ctm.e += (float)relMove.x;
			node->ctm.f += (float)relMove.y;

			gRenderCache.DropAllCache();
			win->dm->Redraw();

			bRet = TRUE;
		}
	}
	return bRet;
}

static void GetObjRect(HPDFOBJ hObj,FRect& rtObj)
{
	fz_display_node *node = (fz_display_node*)(hObj);

	rtObj.x0 = node->rect.x0;
	rtObj.y0 = node->rect.y0;
	rtObj.x1 = node->rect.x1;
	rtObj.y1 = node->rect.y1;
}

static BOOL DeleteCharByPos(int pageNo, HPDFOBJ hObj, const FPoint& fPt, BOOL bBackspace, DOUBLE* xCursor)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return FALSE;

	if(!win->dm || !win->dm->engine)
		return FALSE;

	if(!hObj)
		return FALSE;

	PointD ptD;
	ptD.x = fPt.x;
	ptD.y = fPt.y;
#if 0
	BOOL bRet = win->dm->engine->DeleteCharByPos(pageNo,pObj->m_hObj,ptD,bBackspace,xCursor);
#else
	RectD updateRect;
	BOOL bRet = win->dm->textSelection->DeleteCharByPos(pageNo,hObj,ptD,bBackspace,xCursor,&updateRect);
#endif
	if(bRet)
	{
		win->dm->Redraw(&updateRect);
	}
	return bRet;
}

static BOOL InsertCharByPos(int pageNo, HPDFOBJ hObj, const FPoint& fPt, WCHAR chIns, DOUBLE* xCursor)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return FALSE;

	if(!win->dm || !win->dm->engine)
		return FALSE;

	if(!hObj)
		return FALSE;

	PointD ptD;
	ptD.x = fPt.x;
	ptD.y = fPt.y;
	RectD updateRect;
	BOOL bRet = win->dm->textSelection->InsertCharByPos(pageNo,hObj,ptD,chIns,xCursor,&updateRect);
	if(bRet)
	{
		win->dm->Redraw(&updateRect);
	}
	return bRet;
}

static void FreeMem(void* pMem)
{
	if(!pMem)
		return;

	free(pMem);
}

static HWND GetCanvasWnd()
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return NULL;

	return win->hwndCanvas;
}

static RECT CvtToScreen(int pageNo, const FRect& r)
{
	RECT ret = {0};

	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return ret;

	if(!win->dm)
		return ret;

	RectD rd;
	rd.x = r.x0;
	rd.y = r.y0;
	rd.dx = r.x1 - r.x0;
	rd.dy = r.y1 - r.y0;
	RectI ri = win->dm->CvtToScreen(pageNo,rd);

	ret.left = ri.x;
	ret.top = ri.y;
	ret.right = ri.x + ri.dx;
	ret.bottom = ri.y + ri.dy;
	return ret;
}

static FPoint CvtFromScreen(const POINT& pt, int pageNo)
{
	FPoint ret;

	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return ret;

	if(!win->dm)
		return ret;

	PointI ptI;
	ptI.x = pt.x;
	ptI.y = pt.y;
	PointD ptD = win->dm->CvtFromScreen(ptI,pageNo);

	ret.x = ptD.x;
	ret.y = ptD.y;
	return ret;
}

static void UpdateView()
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win || !win->hwndCanvas)
		return;

	::InvalidateRect(win->hwndCanvas,NULL,TRUE);
}

static BOOL MoveCursor(int pageNo, HPDFOBJ hObj, const FPoint& fPt, INT nMove, DOUBLE* xCursor)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return FALSE;

	if(!win->dm || !win->dm->engine)
		return FALSE;

	if(!hObj)
		return FALSE;

	PointD ptD;
	ptD.x = fPt.x;
	ptD.y = fPt.y;
	return win->dm->textSelection->MoveCursor(pageNo,hObj,ptD,nMove,xCursor);
}

static INT GetPageNoByPoint(INT x, INT y)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return 0;

	if(!win->dm)
		return 0;

	int pageNo = win->dm->GetPageNoByPoint(PointI(x, y));
	if (win->dm->ValidPageNo(pageNo))
		return pageNo;

	return 0;
}

static BOOL IsColorProperty(const fz_display_node* node,LPCTSTR lpPropName,fz_display_node** node_use)
{
	*node_use = NULL;

	if(node->cmd==FZ_CMD_FILL_TEXT && lstrcmp(lpPropName,_T("Fill Color"))==0)
		return TRUE;

	if(node->cmd==FZ_CMD_STROKE_TEXT)
	{
		if(lstrcmp(lpPropName,_T("Stroke Color"))==0)
			return TRUE;

		if(lstrcmp(lpPropName,_T("Fill Color"))==0)
		{
			if(node->last && node->last->is_dup && node->last->cmd==FZ_CMD_FILL_TEXT)
			{
				*node_use = node->last;
				return TRUE;
			}
		}
	}

	return FALSE;
}

static LPCSTR GetStdFontBuiltinName(LPCSTR lpFontName)
{
	static LPCSTR _std_font[][2] = {
		{"NimbusMonL-Regu"				, "Courier"},
		{"NimbusMonL-ReguObli"			, "Courier-Oblique"},
		{"NimbusMonL-Bold"				, "Courier-Bold"},
		{"NimbusMonL-BoldObli"			, "Courier-BoldOblique"},

		{"NimbusSanL-Regu"				, "Helvetica"},			
		{"NimbusSanL-ReguItal"			, "Helvetica-Oblique"},
		{"NimbusSanL"					, "Helvetica-Bold"},
		{"NimbusSanL-Bold"				, "Helvetica-Bold"},		
		{"NimbusSanL-BoldItal"			, "Helvetica-BoldOblique"},

		{"NimbusRomNo9L-Regu"			, "Times-Roman"},
		{"NimbusRomNo9L-ReguItal"		, "Times-Italic"},
		{"NimbusRomNo9L-Medi"			, "Times-Bold"},
		{"NimbusRomNo9L-MediItal"		, "Times-BoldItalic"},

		{"StandardSymL"				, "Symbol"},
		{"Dingbats"					, "ZapfDingbats"}
	};

	for(INT i = 0;i < sizeof(_std_font)/sizeof(_std_font[0]);i++)
	{
		if(lstrcmpA(_std_font[i][0],lpFontName)==0)
			return _std_font[i][1];
	}

	return NULL;
}

static BOOL SetObjectFont(int pageNo, HPDFOBJ hObj,LPCSTR lpFontName, FRect* rtText)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return FALSE;

	if(!win->dm || !win->dm->engine)
		return FALSE;

	if(!hObj)
		return FALSE;

	fz_display_node* node = (fz_display_node*)hObj;

	if(!node->item.text || !node->item.text->font)
		return FALSE;

	if(lstrcmpA(node->item.text->font->name,lpFontName)==0)
		return TRUE;

	pdf_font_desc *fontdesc = NULL;

	LPCSTR lpBuiltinName = GetStdFontBuiltinName(lpFontName);
	if(lpBuiltinName)
	{
		fz_error error = my_pdf_load_simple_font(&fontdesc, (char*)lpBuiltinName);
		if (error)
		{
			return FALSE;
		}
	}

	if(!fontdesc)
		return FALSE;

	fz_drop_font(node->item.text->font);
	node->item.text->font = fz_keep_font(fontdesc->font);

	if(node->item.text->gstate.font)
	{
		pdf_drop_font(node->item.text->gstate.font);
		node->item.text->gstate.font = pdf_keep_font(fontdesc);
	}

	//����cid��gid
	{
		for(INT i = 0;i < node->item.text->len;i++)
		{
			WCHAR wbuf[] = {node->item.text->items[i].ucs,0};
			unsigned char buf[MAX_PATH] = {0};
			WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,wbuf,-1,(LPSTR)buf,sizeof(buf),NULL,NULL);

			int cid = 0;
			if(!ansii_to_cid(fontdesc,buf,cid))
				continue;

			node->item.text->items[i].gid = pdf_font_cid_to_gid(fontdesc, cid);
		}
	}

	if(node->cmd==FZ_CMD_STROKE_TEXT)
	{
		if(node->last && node->last->is_dup && node->last->cmd==FZ_CMD_FILL_TEXT)
		{
			node = node->last;

			assert(node->item.text->font);
			fz_drop_font(node->item.text->font);
			node->item.text->font = fz_keep_font(fontdesc->font);

			if(node->item.text->gstate.font)
			{
				pdf_drop_font(node->item.text->gstate.font);
				node->item.text->gstate.font = pdf_keep_font(fontdesc);
			}

			//����cid��gid
			{
				for(INT i = 0;i < node->item.text->len;i++)
				{
					WCHAR wbuf[] = {node->item.text->items[i].ucs,0};
					unsigned char buf[MAX_PATH] = {0};
					WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK,wbuf,-1,(LPSTR)buf,sizeof(buf),NULL,NULL);

					int cid = 0;
					if(!ansii_to_cid(fontdesc,buf,cid))
						continue;

					node->item.text->items[i].gid = pdf_font_cid_to_gid(fontdesc, cid);
				}
			}
		}
	}

	pdf_drop_font(fontdesc);

	win->dm->textSelection->UpdateTextPos(pageNo,node,rtText);

	gRenderCache.DropAllCache();
	win->dm->Redraw();
	return TRUE;
}

static BOOL GetPropertyDescr(HPDFOBJ hObj,LPCTSTR lpPropName,LPSTR lpDescr)
{
	fz_display_node* node = (fz_display_node*)hObj;

	SumatraPdfIntf::ObjType ot = GetObjType(hObj);

	if(lstrcmp(lpPropName,_T("Char Space"))==0)
	{
		if(ot != SumatraPdfIntf::OT_Text)
			return FALSE;

		snprintf(lpDescr,MAX_PATH - 1,"%.2f",node->item.text->gstate.char_space);

		return TRUE;
	}
	else if(lstrcmp(lpPropName,_T("Word Space"))==0)
	{
		if(ot != SumatraPdfIntf::OT_Text)
			return FALSE;

		snprintf(lpDescr,MAX_PATH - 1,"%.2f",node->item.text->gstate.word_space);

		return TRUE;
	}
	else if(lstrcmp(lpPropName,_T("Text Mode"))==0)
	{
		if(ot != SumatraPdfIntf::OT_Text)
			return FALSE;

		switch(node->cmd)
		{
		case FZ_CMD_FILL_TEXT:
			lstrcpynA(lpDescr,"Fill Text",MAX_PATH - 1);
			return TRUE;
			break;
		case FZ_CMD_STROKE_TEXT:
			if(node->last && node->last->is_dup && node->last->cmd==FZ_CMD_FILL_TEXT)
				lstrcpynA(lpDescr,"Fill then Stroke text",MAX_PATH - 1);
			else
				lstrcpynA(lpDescr,"Stroke Text",MAX_PATH - 1);
			return TRUE;
			break;
		}		
	}
	else if(lstrcmp(lpPropName,_T("Font"))==0)
	{
		if(ot != SumatraPdfIntf::OT_Text)
			return FALSE;

		if(node->item.text && node->item.text->font)
		{
			lstrcpynA(lpDescr,node->item.text->font->name,MAX_PATH - 1);
			return TRUE;
		}
	}
	else if(lstrcmp(lpPropName,_T("Font Size"))==0)
	{
		if(ot != SumatraPdfIntf::OT_Text)
			return FALSE;

		if(node->item.text)
		{
			float size = fz_matrix_expansion(node->item.text->trm);
			snprintf(lpDescr,MAX_PATH - 1,"%.2f",size); //trm.d��û��תʱ�����С,trm.a����
			return TRUE;
		}
	}
	else if(lstrcmp(lpPropName,_T("Position X(points)"))==0)
	{
#if 0
		if(node->item.text)
		{
			snprintf(lpDescr,MAX_PATH - 1,"%.2f",node->item.text->gstate.tm.e);
			return TRUE;
		}
#else
		snprintf(lpDescr,MAX_PATH - 1,"%.2f",node->ctm.e);
		return TRUE;
#endif
	}
	else if(lstrcmp(lpPropName,_T("Position Y(points)"))==0)
	{
#if 0
		if(node->item.text)
		{
			snprintf(lpDescr,MAX_PATH - 1,"%.2f",node->item.text->gstate.tm.f);
			return TRUE;
		}
#else
		snprintf(lpDescr,MAX_PATH - 1,"%.2f",node->ctm.f);
		return TRUE;
#endif
	}
	else if(lstrcmp(lpPropName,_T("Horizontal Scale(%)"))==0)
	{
		if(ot != SumatraPdfIntf::OT_Text)
			return FALSE;

		if(node->item.text)
		{
			double fRate = (node->item.text->trm.a / node->item.text->trm.d) * 100.0;
			snprintf(lpDescr,MAX_PATH - 1,"%d",(int)ceilf((float)fRate - 0.001f));
			return TRUE;
		}
	}
	else if(lstrcmp(lpPropName,_T("Fill Color"))==0 && node->item.text)
	{
		if(ot != SumatraPdfIntf::OT_Text)
			return FALSE;

		if(node->cmd==FZ_CMD_STROKE_TEXT)
		{
			if(node->last && node->last->is_dup && node->last->cmd==FZ_CMD_FILL_TEXT)
				node = node->last;
		}

		INT r = 0,g = 0,b = 0,a = 100;

		if(node->item.text->gstate.fill_colorspace_n >= 1)
		{
			float f = node->item.text->gstate.fill_v[0] * 100.0f;
			r = (INT)ceilf((float)f - 0.001f);
		}
		if(node->item.text->gstate.fill_colorspace_n >= 2)
		{
			float f = node->item.text->gstate.fill_v[1] * 100.0f;
			g = (INT)ceilf((float)f - 0.001f);
		}
		if(node->item.text->gstate.fill_colorspace_n >= 3)
		{
			float f = node->item.text->gstate.fill_v[2] * 100.0f;
			b = (INT)ceilf((float)f - 0.001f);
		}
		{
			float f = node->item.text->gstate.fill_alpha * 100.0f;
			a = (INT)ceilf((float)f - 0.001f);
		}

		snprintf(lpDescr,MAX_PATH - 1,"R:%d%%, G:%d%%, B:%d%%, A:%d%%",r,g,b,a);
	}
	else if(lstrcmp(lpPropName,_T("Stroke Color"))==0 && node->item.text)
	{
		if(ot != SumatraPdfIntf::OT_Text)
			return FALSE;

		INT r = 0,g = 0,b = 0,a = 100;

		if(node->item.text->gstate.stroke_colorspace_n >= 1)
		{
			float f = node->item.text->gstate.stroke_v[0] * 100.0f;
			r = (INT)ceilf((float)f - 0.001f);
		}
		if(node->item.text->gstate.stroke_colorspace_n >= 2)
		{
			float f = node->item.text->gstate.stroke_v[1] * 100.0f;
			g = (INT)ceilf((float)f - 0.001f);
		}
		if(node->item.text->gstate.stroke_colorspace_n >= 3)
		{
			float f = node->item.text->gstate.stroke_v[2] * 100.0f;
			b = (INT)ceilf((float)f - 0.001f);
		}
		{
			float f = node->item.text->gstate.stroke_alpha * 100.0f;
			a = (INT)ceilf((float)f - 0.001f);
		}

		snprintf(lpDescr,MAX_PATH - 1,"R:%d%%, G:%d%%, B:%d%%, A:%d%%",r,g,b,a);
	}

	return FALSE;
}

static BOOL SetFillColor(HPDFOBJ hObj, INT r, INT g, INT b, INT a)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return FALSE;

	if(!win->dm || !win->dm->engine)
		return FALSE;

	if(!hObj)
		return FALSE;

	fz_display_node* node = (fz_display_node*)hObj;

	node->item.text->gstate.fill_alpha = (float)a / 100.0f;
	node->item.text->gstate.fill_colorspace_n = 3;
	node->item.text->gstate.fill_v[0] = (float)r / 100.0f;
	node->item.text->gstate.fill_v[1] = (float)g / 100.0f;
	node->item.text->gstate.fill_v[2] = (float)b / 100.0f;

	if(node->cmd==FZ_CMD_FILL_TEXT)
	{
		node->alpha = (float)a / 100.0f;
		
		fz_drop_colorspace(node->colorspace);
		node->colorspace = fz_keep_colorspace(fz_find_device_colorspace("DeviceRGB"));

		node->color[0] = (float)r / 100.0f;
		node->color[1] = (float)g / 100.0f;
		node->color[2] = (float)b / 100.0f;

		gRenderCache.DropAllCache();
		win->dm->Redraw();
	}
	else if(node->cmd==FZ_CMD_STROKE_TEXT)
	{
		if(node->last && node->last->is_dup && node->last->cmd==FZ_CMD_FILL_TEXT)
		{
			node = node->last;

			node->item.text->gstate.fill_alpha = (float)a / 100.0f;
			node->item.text->gstate.fill_colorspace_n = 3;
			node->item.text->gstate.fill_v[0] = (float)r / 100.0f;
			node->item.text->gstate.fill_v[1] = (float)g / 100.0f;
			node->item.text->gstate.fill_v[2] = (float)b / 100.0f;

			node->alpha = (float)a / 100.0f;

			fz_drop_colorspace(node->colorspace);
			node->colorspace = fz_keep_colorspace(fz_find_device_colorspace("DeviceRGB"));

			node->color[0] = (float)r / 100.0f;
			node->color[1] = (float)g / 100.0f;
			node->color[2] = (float)b / 100.0f;

			gRenderCache.DropAllCache();
			win->dm->Redraw();
		}
	}

	return TRUE;
}

static BOOL SetStrokeColor(HPDFOBJ hObj, INT r, INT g, INT b, INT a)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return FALSE;

	if(!win->dm || !win->dm->engine)
		return FALSE;

	if(!hObj)
		return FALSE;

	fz_display_node* node = (fz_display_node*)hObj;

	node->item.text->gstate.stroke_alpha = (float)a / 100.0f;
	node->item.text->gstate.stroke_colorspace_n = 3;
	node->item.text->gstate.stroke_v[0] = (float)r / 100.0f;
	node->item.text->gstate.stroke_v[1] = (float)g / 100.0f;
	node->item.text->gstate.stroke_v[2] = (float)b / 100.0f;

	if(node->cmd==FZ_CMD_STROKE_TEXT)
	{
		node->alpha = (float)a / 100.0f;

		fz_drop_colorspace(node->colorspace);
		node->colorspace = fz_keep_colorspace(fz_find_device_colorspace("DeviceRGB"));

		node->color[0] = (float)r / 100.0f;
		node->color[1] = (float)g / 100.0f;
		node->color[2] = (float)b / 100.0f;

		gRenderCache.DropAllCache();
		win->dm->Redraw();
	}

	return TRUE;
}

static BOOL SetFontSize(int pageNo, HPDFOBJ hObj, float fontSize, FRect* rtText)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return FALSE;

	if(!win->dm || !win->dm->engine)
		return FALSE;

	if(!hObj)
		return FALSE;

	fz_display_node* node = (fz_display_node*)hObj;

	if(!node->item.text)
		return FALSE;

	float old_size = fz_matrix_expansion(node->item.text->trm);
	float rate = (float)fontSize / old_size;
	if(rate <= 0.0)
		return FALSE;
	
	node->item.text->trm.d *= rate;
	node->item.text->trm.a *= rate;
	node->item.text->trm.b *= rate;
	node->item.text->trm.c *= rate;

	if(node->item.text->gstate.font)
	{
		node->item.text->gstate.tm.d *= rate;
		node->item.text->gstate.tm.a *= rate;
		node->item.text->gstate.tm.b *= rate;
		node->item.text->gstate.tm.c *= rate;
	}

	if(node->cmd==FZ_CMD_STROKE_TEXT)
	{
		if(node->last && node->last->is_dup && node->last->cmd==FZ_CMD_FILL_TEXT)
		{
			node = node->last;

			node->item.text->trm.d *= rate;
			node->item.text->trm.a *= rate;
			node->item.text->trm.b *= rate;
			node->item.text->trm.c *= rate;

			if(node->item.text->gstate.font)
			{
				node->item.text->gstate.tm.d *= rate;
				node->item.text->gstate.tm.a *= rate;
				node->item.text->gstate.tm.b *= rate;
				node->item.text->gstate.tm.c *= rate;
			}
		}
	}

	win->dm->textSelection->UpdateTextPos(pageNo,(fz_display_node*)hObj,rtText);

	gRenderCache.DropAllCache();
	win->dm->Redraw();

	return TRUE;
}
static BOOL SetCharSpace(int pageNo, HPDFOBJ hObj,float char_space, FRect* rtText)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return FALSE;

	if(!win->dm || !win->dm->engine)
		return FALSE;

	if(!hObj)
		return FALSE;

	fz_display_node* node = (fz_display_node*)hObj;

	if(!node->item.text)
		return FALSE;

	node->item.text->gstate.char_space = char_space;

	if(node->last && node->last->is_dup)
	{
		node->last->item.text->gstate.char_space = char_space;
	}

	win->dm->textSelection->UpdateTextPos(pageNo,node,rtText);

	gRenderCache.DropAllCache();
	win->dm->Redraw();

	return TRUE;
}
static BOOL SetWordSpace(int pageNo, HPDFOBJ hObj,float word_space, FRect* rtText)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return FALSE;

	if(!win->dm || !win->dm->engine)
		return FALSE;

	if(!hObj)
		return FALSE;

	fz_display_node* node = (fz_display_node*)hObj;

	if(!node->item.text)
		return FALSE;

	node->item.text->gstate.word_space = word_space;

	if(node->last && node->last->is_dup)
	{
		node->last->item.text->gstate.word_space = word_space;
	}

	win->dm->textSelection->UpdateTextPos(pageNo,node,rtText);

	gRenderCache.DropAllCache();
	win->dm->Redraw();

	return TRUE;
}
static BOOL RotateObject(int pageNo, HPDFOBJ hObj,INT rotation, FRect* rtText)
{
	WindowInfo* win = WindowInfo::g_pWinInf;
	if(!win)
		return FALSE;

	if(!win->dm || !win->dm->engine)
		return FALSE;

	if(!hObj)
		return FALSE;

	fz_display_node* node = (fz_display_node*)hObj;

	if(!node->item.text)
		return FALSE;

	node->item.text->trm = fz_concat(node->item.text->trm, fz_rotate((float)rotation));
	node->item.text->gstate.tm = fz_concat(node->item.text->gstate.tm, fz_rotate((float)rotation));

	win->dm->textSelection->UpdateTextPos(pageNo,node,rtText);

	if(node->cmd==FZ_CMD_STROKE_TEXT)
	{
		if(node->last && node->last->is_dup && node->last->cmd==FZ_CMD_FILL_TEXT)
		{
			node = node->last;

			node->item.text->trm = fz_concat(node->item.text->trm, fz_rotate((float)rotation));
			node->item.text->gstate.tm = fz_concat(node->item.text->gstate.tm, fz_rotate((float)rotation));
		}
	}

	gRenderCache.DropAllCache();
	win->dm->Redraw();

	return TRUE;
}

int APIENTRY LaunchPdf(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, SumatraPdfIntf* pIntf)
{
	g_pIntf = pIntf;
	g_pIntf->GetCanvasWnd = GetCanvasWnd;
	g_pIntf->GetFirstPdfObj = GetFirstPdfObj;
	g_pIntf->GetNextPdfObj = GetNextPdfObj;
	g_pIntf->ExtractObjLineText = ExtractObjLineText;
	g_pIntf->FreeMem = FreeMem;
	g_pIntf->GetObjRect = GetObjRect;
	g_pIntf->GetObjType = GetObjType;
	g_pIntf->CvtToScreen = CvtToScreen;
	g_pIntf->CvtFromScreen = CvtFromScreen;
	g_pIntf->GetPageNoByPoint = GetPageNoByPoint;
	g_pIntf->UpdateView = UpdateView;
	g_pIntf->DeleteCharByPos = DeleteCharByPos;
	g_pIntf->InsertCharByPos = InsertCharByPos;
	g_pIntf->MoveCursor = MoveCursor;
	g_pIntf->GetPropertyDescr = GetPropertyDescr;
	g_pIntf->MoveObject = MoveObject;
	g_pIntf->SetFillColor = SetFillColor;
	g_pIntf->SetStrokeColor = SetStrokeColor;
	g_pIntf->SetObjectFont = SetObjectFont;
	g_pIntf->SetFontSize = SetFontSize;
	g_pIntf->SetCharSpace = SetCharSpace;
	g_pIntf->SetWordSpace = SetWordSpace;
	g_pIntf->RotateObject = RotateObject;

	return WinMain(hInstance,hPrevInstance,lpCmdLine,SW_SHOW);
}
//////////////////////////////////////////////////////////////////////////