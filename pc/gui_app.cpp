#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WINVER 0x0601
#define _WIN32_WINNT 0x0601

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <shlobj.h>
#include <mmsystem.h>
#include <ctime>
#include <cstring>
#include <utility>
#include <ole2.h>
#include <gdiplus.h>
#include <tchar.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "database.h"
#include "storage_handler.h"
#include "nebula_ui.h"

// ============================================================
// NebulaDrive desktop shell
//
// A native GDI+ rendering of the NebulaDrive workspace so the PC
// build matches the web app: same palette, same layout, same
// motion language. Everything is painted by hand and every
// interactive region is registered during paint so hit-testing
// stays in sync with what is actually on screen.
// ============================================================

using namespace Gdiplus;
using namespace neb;

namespace {

// ------------------------------------------------------------------
// Interaction identifiers
// ------------------------------------------------------------------
enum HitId {
    HIT_NONE = 0,
    HIT_TAB_FILES,
    HIT_TAB_FAVORITES,
    HIT_TAB_HISTORY,
    HIT_TAB_TRASH,
    HIT_LANG,
    HIT_SETTINGS,
    HIT_INFO,
    HIT_BACK,
    HIT_FORWARD,
    HIT_REFRESH,
    HIT_NEW_FOLDER,
    HIT_SORT,
    HIT_VIEW,
    HIT_FAV_FILTER,
    HIT_SEARCH,
    HIT_DROPZONE,
    HIT_SELECT_TOGGLE,
    HIT_CARD,
    HIT_CARD_STAR,
    HIT_CARD_MORE,
    HIT_CARD_OPEN,
    HIT_TOAST_CLOSE,
    HIT_MODAL_CLOSE,
    HIT_TGL_LIQUID,
    HIT_TGL_ANIM,
    HIT_TGL_CALM,
    HIT_TGL_PREMIUM_FX,
    HIT_TGL_SAVE_LOGIN,
    HIT_TGL_CONFIRM,
    HIT_TGL_EXTENSIONS,
    HIT_TGL_OPENFOLDER,
    HIT_TGL_REMEMBER,
    HIT_THEME,
    HIT_MODAL_SCAN,
    HIT_MODEL_BACKUP,
    HIT_MODAL_VACUUM,
    HIT_MODAL_EMPTY_TRASH,
    HIT_FOOTER_PRIVACY,
    HIT_FOOTER_TERMS,
    HIT_FOOTER_COOKIE,
    HIT_FOOTER_FEEDBACK,
    HIT_BOTTOM_OPEN,
    HIT_BOTTOM_CLEAR,
    HIT_MENU_ITEM,       // generic row inside a dropdown or popup menu
    HIT_LANGUAGE_ITEM,   // a row in the language picker
    HIT_THEME_SEGMENT,   // All / Standard / Premium segmented control
    HIT_NEW_FOLDER_OK,
    HIT_NEW_FOLDER_CANCEL,
    HIT_USER_CHIP,
    HIT_SELECT_ALL,
    HIT_SELECT_DESELECT,
    HIT_SELECT_MOVE,
    HIT_SELECT_EXPORT,
    HIT_SELECT_DELETE
};

// Commands carried by menu rows.
enum Command {
    CMD_NONE = 0,
    CMD_SORT_NAME,
    CMD_SORT_SIZE,
    CMD_SORT_DATE,
    CMD_SORT_TYPE,
    CMD_SORT_EXTENSION,
    CMD_SORT_ASC,
    CMD_SORT_DESC,
    CMD_VIEW_MEDIUM,
    CMD_VIEW_SMALL,
    CMD_VIEW_LARGE,
    CMD_VIEW_LIST,
    CMD_THEME_ALL,
    CMD_THEME_STANDARD,
    CMD_THEME_PREMIUM,
    CMD_LANG_EN,
    CMD_LANG_ES,
    CMD_LANG_FR,
    CMD_LANG_DE,
    CMD_LANG_PT,
    CMD_LANG_HI,
    CMD_LANG_AR,
    CMD_LANG_ZH,
    CMD_SCAN,
    CMD_ZIP_EXPORT,
    CMD_ZIP_IMPORT,
    CMD_SET_PASSWORD,
    CMD_EXPORT_LOGIN,
    CMD_LOGOUT,
    CMD_RESET_CACHE,
    CMD_FEEDBACK,
    CMD_DANGER_CLEAR,
    CMD_DANGER_DELETE,
    CMD_DANGER_TERMINATE,
    CMD_CUT,
    CMD_MOVE,
    CMD_OPEN_FILE,
    CMD_RENAME,
    CMD_EXPORT,
    CMD_DELETE,
    CMD_RESTORE,
    CMD_DELETE_FOREVER,
    CMD_FEEDBACK_WRITE,
    CMD_FEEDBACK_OPEN,
    CMD_COPY_DIAG
};

struct HitRegion {
    RectF rect;
    int id = HIT_NONE;
    int arg = 0;
    int arg2 = 0;
    bool isFolder = false;
};

enum class Tab { Files, Favorites, History, Trash };
enum class ViewMode { GridSmall, GridMedium, GridLarge, List };
enum class SortKey { Name, Size, Date, Type };
enum class ModalKind { None, Settings, Info, Language, SortMenu, ViewMenu, Legal };
enum class ToastKind { Success, Error, Warn, Info };
enum class ThemeFilter { All, Standard, Premium };

// The languages offered by the language picker.
struct LanguageDef {
    const char* code;
    const wchar_t* label;
    const wchar_t* flag;
};

const LanguageDef kLanguages[] = {
    {"en", L"English", L"\U0001F1EC\U0001F1E7"},
    {"es", L"Espa\u00F1ol", L"\U0001F1EA\U0001F1F8"},
    {"fr", L"Fran\u00E7ais", L"\U0001F1EB\U0001F1F7"},
    {"de", L"Deutsch", L"\U0001F1E9\U0001F1EA"},
    {"pt", L"Portugu\u00EAs", L"\U0001F1F5\U0001F1F9"},
    {"hi", L"\u0939\u093F\u0928\u094D\u0926\u0940", L"\U0001F1EE\U0001F1F3"},
    {"ar", L"\u0627\u0644\u0639\u0631\u0628\u064A\u0629", L"\U0001F1F8\U0001F1E6"},
    {"zh", L"\u4E2D\u6587", L"\U0001F1E8\U0001F1F3"},
};

// ------------------------------------------------------------------
// Animation state
// ------------------------------------------------------------------
struct ElementState {
    float hover = 0.0f;
    float hoverTarget = 0.0f;
    float press = 0.0f;
    float pressTarget = 0.0f;
};

std::map<int, ElementState> g_elements;

ElementState& element(int id) {
    auto it = g_elements.find(id);
    if (it == g_elements.end()) {
        it = g_elements.emplace(id, ElementState()).first;
    }
    return it->second;
}

// Hover state for one row of a menu or list, keyed by id + argument so that
// rows no longer light up together.
int cardKey(int base, int index);
ElementState& rowState(int id, int arg) {
    return element(cardKey(id, arg));
}

// Per-card ids live in a separate numeric range so they never clash
// with the fixed ids above.
int cardKey(int base, int index) {
    return 100000 + base * 4096 + (index & 0xFFF);
}

// ------------------------------------------------------------------
// Toast notifications
// ------------------------------------------------------------------
struct Toast {
    std::wstring message;
    ToastKind kind = ToastKind::Info;
    ULONGLONG born = 0;
    bool dying = false;
    float slide = 0.0f;
    float slideTarget = 0.0f;
};

// ------------------------------------------------------------------
// Application state
// ------------------------------------------------------------------
struct AppState {
    Tab tab = Tab::Files;
    ViewMode view = ViewMode::GridMedium;
    SortKey sortKey = SortKey::Name;
    bool sortAscending = true;

    std::vector<std::string> currentPath;
    std::vector<std::vector<std::string>> pathHistory;
    int historyIndex = -1;

    std::string searchQuery;
    bool searching = false;

    bool selectionMode = false;
    std::vector<int> selected;

    // Settings
    bool liquidGlass = true;
    bool animations = true;
    bool calmMotion = false;
    bool premiumFx = true;
    bool confirmDelete = true;
    bool showExtensions = true;
    bool openFoldersOnClick = true;
    bool rememberLastFolder = true;
    bool saveLogin = true;
    std::string themeName = "Dark Nebula";
    ThemeFilter themeFilter = ThemeFilter::All;
    std::string language = "en";

    // Modal
    ModalKind modal = ModalKind::None;
    float modalAnim = 0.0f;
    float modalScroll = 0.0f;
    float modalContentHeight = 0.0f;
    int legalDoc = 0;  // 0 privacy, 1 terms, 2 cookies, 3 feedback

    // Prompt (rename / new folder)
    bool prompting = false;
    std::wstring promptTitle;
    std::wstring promptValue;
    bool promptIsFolder = false;
    int promptTargetId = -1;

    // Data
    std::vector<FileRecord> folders;
    std::vector<FileRecord> files;
    std::vector<HistoryRecord> history;
    std::map<std::string, uint64_t> folderSizes;
    StorageStats stats;

    // Layout
    float scrollOffset = 0.0f;
    float maxScroll = 0.0f;
    float tabSliderX = 0.0f;
    float tabSliderW = 0.0f;
    float tabSliderTargetX = 0.0f;
    float tabSliderTargetW = 0.0f;
    bool sliderPrimed = false;

    // Card overflow menu
    bool cardMenuOpen = false;
    int cardMenuId = -1;
    bool cardMenuIsFolder = false;
    bool cardMenuInTrash = false;
    std::string cardMenuName;
    RectF cardMenuRect;

    // Bottom bar selection
    int bottomFileId = -1;

    // Profile menu
    bool profileOpen = false;
    std::string status = "Online";
    int statusIndex = 0;

    // Drop target
    bool dragOver = false;
    bool dropActive = false;
};

AppState g_app;
Database* gDatabase = nullptr;
StorageHandler* gStorage = nullptr;
HWND g_hwnd = nullptr;
HWND g_promptEdit = nullptr;
HWND g_searchEdit = nullptr;
float g_scale = 1.0f;
std::vector<HitRegion> g_hits;
size_t g_hitFloor = 0;  // hits below this index are covered by a modal
std::vector<Toast> g_toasts;
int g_hoverId = HIT_NONE;
int g_hoverArg = 0;

// ------------------------------------------------------------------
// Small helpers
// ------------------------------------------------------------------

// Starts the animation timer if it is not already running. The timer is
// stopped again once everything has settled, so an idle app costs nothing.
void wakeAnimations();

float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

float lerpf(float a, float b, float t) {
    return a + (b - a) * t;
}

std::string joinPath(const std::vector<std::string>& parts) {
    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) out += "/";
        out += parts[i];
    }
    return out;
}

std::string formatSizePlain(uint64_t bytes) {
    static const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);
    while (size >= 1024.0 && unit < 4) {
        size /= 1024.0;
        unit++;
    }
    char buf[64];
    if (unit == 0) {
        snprintf(buf, sizeof(buf), "%lld B", static_cast<long long>(bytes));
    } else {
        snprintf(buf, sizeof(buf), "%.2f %s", size, units[unit]);
    }
    return buf;
}

std::string lowerCase(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

std::string extensionOf(const std::string& name) {
    size_t dot = name.find_last_of('.');
    if (dot == std::string::npos || dot == 0) return "";
    return lowerCase(name.substr(dot + 1));
}

// Chooses the icon that best represents a file.
neb::Icon iconForFile(const FileRecord& file) {
    std::string ext = extensionOf(file.name);

    if (file.mimeType.rfind("video/", 0) == 0) return neb::Icon::Video;
    if (file.mimeType.rfind("audio/", 0) == 0) return neb::Icon::Music;
    if (file.mimeType.rfind("image/", 0) == 0) return neb::Icon::Image;

    if (ext == "mp4" || ext == "mov" || ext == "avi" || ext == "mkv" || ext == "webm" ||
        ext == "flv" || ext == "wmv" || ext == "m4v")
        return neb::Icon::Video;
    if (ext == "mp3" || ext == "wav" || ext == "ogg" || ext == "flac" || ext == "m4a" ||
        ext == "aac" || ext == "wma")
        return neb::Icon::Music;
    if (ext == "jpg" || ext == "jpeg" || ext == "png" || ext == "gif" || ext == "webp" ||
        ext == "bmp" || ext == "svg" || ext == "ico" || ext == "heic")
        return neb::Icon::Image;
    if (ext == "zip" || ext == "rar" || ext == "7z" || ext == "tar" || ext == "gz" ||
        ext == "bz2" || ext == "xz")
        return neb::Icon::Archive;
    return neb::Icon::Document;
}

// ------------------------------------------------------------------
// Toasts
// ------------------------------------------------------------------
void pushToast(const std::wstring& message, ToastKind kind) {
    Toast toast;
    toast.message = message;
    toast.kind = kind;
    toast.born = GetTickCount64();
    toast.slideTarget = 1.0f;
    g_toasts.push_back(toast);
    wakeAnimations();

    while (g_toasts.size() > 4) {
        g_toasts.erase(g_toasts.begin());
    }
}

// ------------------------------------------------------------------
// Settings persistence
// ------------------------------------------------------------------
void loadSettings() {
    if (!gDatabase) return;
    auto readBool = [&](const char* key, bool fallback) {
        std::string value = gDatabase->getSetting(key);
        if (value.empty()) return fallback;
        return value == "1";
    };

    g_app.liquidGlass = readBool("liquid_glass", true);
    g_app.animations = readBool("animations", true);
    g_app.calmMotion = readBool("calm_motion", false);
    g_app.premiumFx = readBool("premium_fx", true);
    g_app.confirmDelete = readBool("confirm_delete", true);
    g_app.showExtensions = readBool("show_extensions", true);
    g_app.openFoldersOnClick = readBool("open_folders_on_click", true);
    g_app.rememberLastFolder = readBool("remember_last_folder", true);
    g_app.saveLogin = readBool("save_login", true);

    std::string theme = gDatabase->getSetting("theme");
    if (!theme.empty() && neb::findTheme(theme)) {
        g_app.themeName = theme;
    }
    neb::applyTheme(g_app.themeName);

    std::string language = gDatabase->getSetting("language");
    if (!language.empty()) {
        g_app.language = language;
    }

    std::string filter = gDatabase->getSetting("theme_filter");
    if (filter == "standard") g_app.themeFilter = ThemeFilter::Standard;
    else if (filter == "premium") g_app.themeFilter = ThemeFilter::Premium;
    else g_app.themeFilter = ThemeFilter::All;

    neb::setGlassEnabled(g_app.liquidGlass);

    std::string view = gDatabase->getSetting("view_mode");
    if (view == "small") g_app.view = ViewMode::GridSmall;
    else if (view == "large") g_app.view = ViewMode::GridLarge;
    else if (view == "list") g_app.view = ViewMode::List;
    else g_app.view = ViewMode::GridMedium;

    std::string sort = gDatabase->getSetting("sort_key");
    if (sort == "size") g_app.sortKey = SortKey::Size;
    else if (sort == "date") g_app.sortKey = SortKey::Date;
    else if (sort == "type") g_app.sortKey = SortKey::Type;
    else g_app.sortKey = SortKey::Name;
    g_app.sortAscending = gDatabase->getSetting("sort_order") != "desc";
}

void saveSettings() {
    if (!gDatabase) return;
    gDatabase->setSetting("liquid_glass", g_app.liquidGlass ? "1" : "0");
    gDatabase->setSetting("animations", g_app.animations ? "1" : "0");
    gDatabase->setSetting("calm_motion", g_app.calmMotion ? "1" : "0");
    gDatabase->setSetting("premium_fx", g_app.premiumFx ? "1" : "0");
    gDatabase->setSetting("confirm_delete", g_app.confirmDelete ? "1" : "0");
    gDatabase->setSetting("show_extensions", g_app.showExtensions ? "1" : "0");
    gDatabase->setSetting("open_folders_on_click", g_app.openFoldersOnClick ? "1" : "0");
    gDatabase->setSetting("remember_last_folder", g_app.rememberLastFolder ? "1" : "0");
    gDatabase->setSetting("save_login", g_app.saveLogin ? "1" : "0");
    gDatabase->setSetting("language", g_app.language);
    gDatabase->setSetting("theme", g_app.themeName);
    gDatabase->setSetting("theme_filter",
                          g_app.themeFilter == ThemeFilter::Standard ? "standard"
                          : g_app.themeFilter == ThemeFilter::Premium ? "premium"
                                                                   : "all");

    const char* view = "medium";
    switch (g_app.view) {
        case ViewMode::GridSmall: view = "small"; break;
        case ViewMode::GridLarge: view = "large"; break;
        case ViewMode::List: view = "list"; break;
        default: break;
    }
    gDatabase->setSetting("view_mode", view);

    const char* sort = "name";
    switch (g_app.sortKey) {
        case SortKey::Size: sort = "size"; break;
        case SortKey::Date: sort = "date"; break;
        case SortKey::Type: sort = "type"; break;
        default: break;
    }
    gDatabase->setSetting("sort_key", sort);
    gDatabase->setSetting("sort_order", g_app.sortAscending ? "asc" : "desc");
}

// ------------------------------------------------------------------
// Data loading
// ------------------------------------------------------------------
void computeFolderSizes() {
    g_app.folderSizes.clear();
    if (!gDatabase) return;

    for (const auto& file : gDatabase->listFiles(false)) {
        g_app.folderSizes[file.parentFolder] += file.size;
        // Credit every ancestor folder too so nested folders show a total.
        std::string parent = file.parentFolder;
        while (!parent.empty()) {
            size_t slash = parent.find_last_of('/');
            if (slash == std::string::npos) break;
            parent = parent.substr(0, slash);
            g_app.folderSizes[parent] += file.size;
        }
    }
}

void sortItems() {
    const bool asc = g_app.sortAscending;
    auto cmp = [&](const FileRecord& a, const FileRecord& b) {
        int result = 0;
        switch (g_app.sortKey) {
            case SortKey::Size:
                result = a.size < b.size ? -1 : (a.size > b.size ? 1 : 0);
                break;
            case SortKey::Date:
                result = a.createdAt.compare(b.createdAt);
                break;
            case SortKey::Type:
                result = a.mimeType.compare(b.mimeType);
                break;
            default: {
                std::string na = lowerCase(a.name);
                std::string nb = lowerCase(b.name);
                result = na.compare(nb);
                break;
            }
        }
        if (result == 0) result = a.name.compare(b.name);
        return asc ? result < 0 : result > 0;
    };

    std::sort(g_app.folders.begin(), g_app.folders.end(), cmp);
    std::sort(g_app.files.begin(), g_app.files.end(), cmp);
}

void reloadData() {
    if (!gDatabase) return;

    // Any data change repaints the workspace.
    if (g_hwnd) {
        InvalidateRect(g_hwnd, nullptr, FALSE);
    }

    g_app.stats = gStorage->getStats();

    switch (g_app.tab) {
        case Tab::Files: {
            std::string parent = joinPath(g_app.currentPath);
            g_app.folders = gDatabase->listFoldersByParent(parent);
            if (g_app.searching && !g_app.searchQuery.empty()) {
                // Search across the whole library, not just this folder.
                g_app.files = gDatabase->searchFiles(g_app.searchQuery);
            } else {
                g_app.files = gDatabase->listFilesByFolder(parent, false);
            }
            computeFolderSizes();
            break;
        }
        case Tab::Favorites:
            g_app.folders.clear();
            g_app.files = gDatabase->listFavoriteFiles();
            break;
        case Tab::History:
            g_app.folders.clear();
            g_app.files.clear();
            g_app.history = gDatabase->getHistory(150);
            break;
        case Tab::Trash:
            g_app.folders.clear();
            g_app.files = gDatabase->listFiles(true);
            g_app.files.erase(
                std::remove_if(g_app.files.begin(), g_app.files.end(),
                               [](const FileRecord& f) { return !f.isDeleted; }),
                g_app.files.end());
            break;
    }

    sortItems();
    g_app.scrollOffset = clampf(g_app.scrollOffset, 0.0f, g_app.maxScroll);
}

void navigateTo(const std::vector<std::string>& path, bool recordHistory = true) {
    g_app.currentPath = path;
    g_app.searchQuery.clear();
    g_app.searching = false;
    g_app.scrollOffset = 0.0f;

    if (recordHistory) {
        if (g_app.historyIndex >= 0 &&
            g_app.historyIndex < static_cast<int>(g_app.pathHistory.size())) {
            g_app.pathHistory.resize(g_app.historyIndex + 1);
        }
        g_app.pathHistory.push_back(path);
        g_app.historyIndex = static_cast<int>(g_app.pathHistory.size()) - 1;
    }

    if (gDatabase && g_app.rememberLastFolder) {
        gDatabase->setSetting("last_folder", joinPath(path));
    }

    reloadData();
}

void switchTab(Tab tab) {
    if (g_app.tab == tab) return;
    g_app.tab = tab;
    g_app.scrollOffset = 0.0f;
    g_app.selectionMode = false;
    g_app.selected.clear();
    g_app.searchQuery.clear();
    g_app.searching = false;
    // The tab pill slides to its new position.
    wakeAnimations();
    reloadData();
}

// ------------------------------------------------------------------
// Hit regions
// ------------------------------------------------------------------
void addHit(const RectF& rect, int id, int arg = 0, int arg2 = 0, bool isFolder = false) {
    if (rect.Width <= 0.0f || rect.Height <= 0.0f) return;
    HitRegion region;
    region.rect = rect;
    region.id = id;
    region.arg = arg;
    region.arg2 = arg2;
    region.isFolder = isFolder;
    g_hits.push_back(region);
}

const HitRegion* hitTest(float x, float y) {
    for (size_t i = g_hits.size(); i > g_hitFloor; --i) {
        if (g_hits[i - 1].rect.Contains(x, y)) return &g_hits[i - 1];
    }
    return nullptr;
}

// ------------------------------------------------------------------
// Animation tick - returns true while something is still moving
// ------------------------------------------------------------------

// Motion is on unless the user turned animations off or asked for calm mode.
bool motionEnabled() {
    return g_app.animations && !g_app.calmMotion;
}

bool tickAnimations() {
    bool active = false;
    // Time based easing: identical speed at any frame rate.
    static LARGE_INTEGER s_freq = {};
    static LARGE_INTEGER s_last = {};
    if (!s_freq.QuadPart) QueryPerformanceFrequency(&s_freq);
    LARGE_INTEGER nowCounter;
    QueryPerformanceCounter(&nowCounter);
    float dt = s_last.QuadPart ? static_cast<float>(nowCounter.QuadPart - s_last.QuadPart) /
                                     static_cast<float>(s_freq.QuadPart)
                               : 0.016f;
    s_last = nowCounter;
    if (dt > 0.1f) dt = 0.016f;
    if (dt > 0.05f) dt = 0.05f;
    if (dt < 0.001f) dt = 0.001f;
    const float speed = 1.0f - std::exp(-dt * 13.0f);
    const bool smooth = motionEnabled();

    for (auto& kv : g_elements) {
        ElementState& state = kv.second;
        if (!smooth) {
            // Animations are off, so jump straight to the resting state.
            state.hover = state.hoverTarget;
            state.press = state.pressTarget;
            continue;
        }
        if (std::fabs(state.hover - state.hoverTarget) > 0.002f) {
            state.hover += (state.hoverTarget - state.hover) * speed;
            active = true;
        } else {
            state.hover = state.hoverTarget;
        }
        if (std::fabs(state.press - state.pressTarget) > 0.002f) {
            state.press += (state.pressTarget - state.press) * speed;
            active = true;
        } else {
            state.press = state.pressTarget;
        }
    }

    if (!smooth) {
        g_app.tabSliderX = g_app.tabSliderTargetX;
        g_app.tabSliderW = g_app.tabSliderTargetW;
        g_app.modalAnim = 1.0f;
        for (auto& toast : g_toasts) {
            toast.slide = toast.slideTarget;
        }
        return false;
    }

    if (std::fabs(g_app.tabSliderX - g_app.tabSliderTargetX) > 0.05f ||
        std::fabs(g_app.tabSliderW - g_app.tabSliderTargetW) > 0.05f) {
        g_app.tabSliderX = lerpf(g_app.tabSliderX, g_app.tabSliderTargetX, speed);
        g_app.tabSliderW = lerpf(g_app.tabSliderW, g_app.tabSliderTargetW, speed);
        active = true;
    }

    if (g_app.modal != ModalKind::None) {
        float next = lerpf(g_app.modalAnim, 1.0f, speed);
        if (std::fabs(next - g_app.modalAnim) > 0.002f) active = true;
        g_app.modalAnim = next;
    }

    for (auto& toast : g_toasts) {
        if (std::fabs(toast.slide - toast.slideTarget) > 0.002f) {
            toast.slide = lerpf(toast.slide, toast.slideTarget, speed);
            active = true;
        }
    }

    return active;
}

// ------------------------------------------------------------------
// Metrics (logical pixels at 96 dpi)
// ------------------------------------------------------------------
struct Metrics {
    float navHeight = 64.0f;
    float padX = 26.0f;
    float taglineHeight = 40.0f;
    float crumbHeight = 52.0f;
    float toolbarHeight = 44.0f;
    float dropHeight = 46.0f;
    float selectHeight = 38.0f;
    float footerHeight = 54.0f;
    float bottomHeight = 58.0f;
    float gap = 14.0f;
    float rowHeight = 58.0f;
};

const Metrics& metrics() {
    static Metrics m;
    return m;
}

void cardMetrics(ViewMode mode, float& width, float& height, float& iconSize) {
    switch (mode) {
        case ViewMode::GridSmall: width = 150.0f; height = 186.0f; iconSize = 46.0f; break;
        case ViewMode::GridLarge: width = 232.0f; height = 252.0f; iconSize = 72.0f; break;
        case ViewMode::List: width = 0.0f; height = metrics().rowHeight; iconSize = 38.0f; break;
        default: width = 184.0f; height = 210.0f; iconSize = 58.0f; break;
    }
}

} // namespace

// ============================================================
// Painting
// ============================================================

namespace {

// Draws text with a horizontal gradient. The gradient is applied as a text
// brush, so only the glyphs are painted.
void drawGradientText(Graphics& g, const std::wstring& text, const RectF& slot, float size, bool bold) {
    const neb::Theme& t = neb::currentTheme();

    FontFamily family(L"Segoe UI");
    Font font(&family, size, bold ? FontStyleBold : FontStyleRegular, UnitPixel);
    StringFormat format;
    format.SetAlignment(StringAlignment::StringAlignmentNear);
    format.SetLineAlignment(StringAlignment::StringAlignmentCenter);
    format.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);

    RectF bounds;
    g.MeasureString(text.c_str(), -1, &font, PointF(0, 0), &format, &bounds);

    LinearGradientBrush brush(PointF(slot.X, slot.Y), PointF(slot.X + bounds.Width, slot.Y), t.accentA,
                              t.accentB);
    g.DrawString(text.c_str(), -1, &font, slot, &format, &brush);
}

// A pill button with a hover fill and a press squash.
void drawPill(Graphics& g, const RectF& rect, float radius, bool hovered, float press,
              bool filled, bool accent) {
    const neb::Theme& t = neb::currentTheme();

    float squash = 1.0f - 0.04f * press;
    RectF r = rect;
    r.X += rect.Width * (1.0f - squash) * 0.5f;
    r.Width *= squash;

    if (filled) {
        if (accent) {
            neb::fillRoundAccent(g, r, radius);
        } else {
            fillRound(g, r, radius, t.glassStrong);
        }
    } else {
        Color fill = t.glass;
        if (hovered) {
            fill = rgba(t.glassHover.GetR(), t.glassHover.GetG(), t.glassHover.GetB(), 200);
        }
        fillRound(g, r, radius, fill);
    }
    strokeRound(g, r, radius, t.border, 1.0f);
}

// Draws the top navigation bar.
void drawNavbar(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();

    RectF bar(0, 0, area.Width, metrics().navHeight);
    fillRoundGradientV(g, bar, 0.0f, rgba(t.glass.GetR(), t.glass.GetG(), t.glass.GetB(), 205),
                       rgba(t.glass.GetR(), t.glass.GetG(), t.glass.GetB(), 165));
    strokeRound(g, RectF(0.5f, metrics().navHeight - 1.0f, area.Width - 1.0f, 1.0f), 0.0f, t.border, 1.0f);

    // Brand.
    neb::drawLogoMark(g, RectF(20.0f, 16.0f, 32.0f, 32.0f));
    neb::drawTextAt(g, L"NebulaDrive", 60.0f, 17.0f, 17.0f, true, t.text);

    RectF badge(154.0f, 20.0f, 42.0f, 18.0f);
    fillRound(g, badge, 9.0f, rgba(255, 200, 60, 40));
    strokeRound(g, badge, 9.0f, rgba(255, 200, 60, 120), 1.0f);
    neb::drawText(g, L"BETA", badge, 9.0f, true, Color(255, 210, 110));

    float right = area.Width - 18.0f;

    // Info.
    float infoW = 72.0f;
    RectF infoBtn(right - infoW, 14.0f, infoW, 36.0f);
    ElementState& infoState = element(HIT_INFO);
    drawPill(g, infoBtn, 12.0f, infoState.hover > 0.01f, infoState.press, false, false);
    neb::drawIcon(g, neb::Icon::Info, RectF(infoBtn.X + 11.0f, infoBtn.Y + 9.0f, 18.0f, 18.0f), t.textSecondary);
    neb::drawText(g, L"Info", RectF(infoBtn.X + 32.0f, infoBtn.Y, 36.0f, infoBtn.Height), 12.0f, false,
                  t.textSecondary);
    addHit(infoBtn, HIT_INFO);

    // Settings.
    float setW = 98.0f;
    RectF setBtn(right - infoW - setW - 8.0f, 14.0f, setW, 36.0f);
    ElementState& setState = element(HIT_SETTINGS);
    if (setState.hover > 0.01f) {
        LinearGradientBrush brush(
            PointF(setBtn.X, setBtn.Y), PointF(setBtn.X + setBtn.Width, setBtn.Y + setBtn.Height),
            rgba(t.accentA.GetR(), t.accentA.GetG(), t.accentA.GetB(), (BYTE)(255 * setState.hover)),
            rgba(t.accentB.GetR(), t.accentB.GetG(), t.accentB.GetB(), (BYTE)(255 * setState.hover)));
        GraphicsPath path; neb::addRoundedRect(path, setBtn.X, setBtn.Y, setBtn.Width, setBtn.Height, 12.0f);
        g.FillPath(&brush, &path);
    } else {
        neb::fillRoundAccent(g, setBtn, 12.0f);
    }
    neb::drawIcon(g, neb::Icon::Gear, RectF(setBtn.X + 11.0f, setBtn.Y + 9.0f, 18.0f, 18.0f), Color(255, 255, 255));
    neb::drawText(g, L"Settings", RectF(setBtn.X + 32.0f, setBtn.Y, 60.0f, setBtn.Height), 12.0f, false,
                  Color(255, 255, 255));
    addHit(setBtn, HIT_SETTINGS);

    // Language.
    float langW = 58.0f;
    RectF langBtn(right - infoW - setW - langW - 16.0f, 14.0f, langW, 36.0f);
    ElementState& langState = element(HIT_LANG);
    drawPill(g, langBtn, 12.0f, langState.hover > 0.01f, langState.press, false, false);
    RectF flag(langBtn.X + 9.0f, langBtn.Y + 11.0f, 18.0f, 13.0f);
    neb::drawFlag(g, g_app.language, flag);
    strokeRound(g, flag, 3.0f, rgba(255, 255, 255, 40), 1.0f);
    neb::drawText(g, L"EN", RectF(langBtn.X + 31.0f, langBtn.Y, 24.0f, langBtn.Height), 12.0f, true,
                  t.textSecondary);
    addHit(langBtn, HIT_LANG);

    // User chip, anchored just left of the language button so the nav tabs
    // always have clear space in the middle.
    float chipW = 176.0f;
    RectF chip(langBtn.X - chipW - 14.0f, 14.0f, chipW, 36.0f);
    RectF avatar(chip.X, chip.Y + 4.0f, 28.0f, 28.0f);
    LinearGradientBrush avatarBrush(PointF(avatar.X, avatar.Y), PointF(avatar.X + 28.0f, avatar.Y + 28.0f),
                                    t.accentA, t.accentB);
    GraphicsPath avatarPath; neb::addRoundedRect(avatarPath, avatar.X, avatar.Y, 28.0f, 28.0f, 14.0f);
    g.FillPath(&avatarBrush, &avatarPath);
    neb::drawIcon(g, neb::Icon::User, RectF(avatar.X + 7.0f, avatar.Y + 6.0f, 14.0f, 14.0f), Color(255, 255, 255), 1.8f);
    neb::drawText(g, L"Makerxm800", RectF(avatar.X + 36.0f, chip.Y + 1.0f, 130.0f, 18.0f), 12.5f, true, t.text,
                  StringAlignment::StringAlignmentNear);
    neb::drawText(g, L"\u2699 upgrading early app", RectF(avatar.X + 36.0f, chip.Y + 17.0f, 150.0f, 16.0f),
                  9.5f, false, Color(120, 220, 160), StringAlignment::StringAlignmentNear);

    // The chip opens the profile menu.
    addHit(chip, HIT_USER_CHIP);
}

// Draws the centre tab strip and its sliding pill.
void drawNavTabs(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();

    struct TabDef {
        Tab tab;
        const wchar_t* label;
        neb::Icon icon;
        int hit;
    };
    static const TabDef tabs[] = {
        {Tab::Files, L"Files", neb::Icon::Folder, HIT_TAB_FILES},
        {Tab::Favorites, L"Favorites", neb::Icon::Star, HIT_TAB_FAVORITES},
        {Tab::History, L"History", neb::Icon::Clock, HIT_TAB_HISTORY},
        {Tab::Trash, L"Trash", neb::Icon::Trash, HIT_TAB_TRASH},
    };

    const int count = 4;
    // Centre the tab strip between the brand and the account controls.
    float leftEdge = 210.0f;
    float rightEdge = area.Width - 470.0f;
    if (rightEdge < leftEdge + 260.0f) {
        rightEdge = leftEdge + 260.0f;
    }
    float groupW = std::min(440.0f, rightEdge - leftEdge);
    float groupX = leftEdge + (rightEdge - leftEdge - groupW) / 2.0f;
    float groupY = 11.0f;
    float groupH = 42.0f;

    RectF track(groupX, groupY, groupW, groupH);
    fillRound(g, track, 14.0f, rgba(255, 255, 255, 12));
    strokeRound(g, track, 14.0f, t.border, 1.0f);

    float tabW = groupW / count;
    float pad = 4.0f;
    float innerW = tabW - pad * 2.0f;

    for (int i = 0; i < count; ++i) {
        RectF tabRect(groupX + pad + i * tabW, groupY + pad, innerW, groupH - pad * 2.0f);
        bool active = (static_cast<int>(g_app.tab) == i);
        if (active) {
            g_app.tabSliderTargetX = tabRect.X;
            g_app.tabSliderTargetW = tabRect.Width;
            wakeAnimations();
        }
    }

    if (!g_app.sliderPrimed) {
        g_app.tabSliderX = g_app.tabSliderTargetX;
        g_app.tabSliderW = g_app.tabSliderTargetW;
        g_app.sliderPrimed = true;
    }

    // The pill sits behind the labels.
    RectF slider(g_app.tabSliderX, groupY + pad, g_app.tabSliderW, groupH - pad * 2.0f);
    neb::fillRoundAccent(g, slider, 11.0f);

    for (int i = 0; i < count; ++i) {
        RectF tabRect(groupX + pad + i * tabW, groupY + pad, innerW, groupH - pad * 2.0f);
        bool active = (static_cast<int>(g_app.tab) == i);

        ElementState& state = element(tabs[i].hit);
        if (state.hover > 0.01f && !active) {
            fillRound(g, tabRect, 11.0f, rgba(255, 255, 255, (BYTE)(24 * state.hover)));
        }

        Color fg = active ? Color(255, 255, 255) : t.textSecondary;
        float lift = active ? -1.0f : (-1.0f * state.hover);
        neb::drawIcon(g, tabs[i].icon, RectF(tabRect.X + 11.0f, tabRect.Y + 11.0f + lift, 16.0f, 16.0f), fg, 2.0f);
        neb::drawText(g, tabs[i].label, RectF(tabRect.X + 31.0f, tabRect.Y + lift, innerW - 36.0f, tabRect.Height),
                      12.5f, active, fg, StringAlignment::StringAlignmentNear);
        addHit(tabRect, tabs[i].hit);
    }
}

// The tagline strip under the navigation bar.
void drawTagline(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();
    RectF banner(area.X, area.Y, area.Width, metrics().taglineHeight);

    fillRound(g, banner, 12.0f, rgba(255, 255, 255, 5));
    strokeRound(g, banner, 12.0f, t.border, 1.0f);

    const std::wstring lead = L"Your private cloud";
    const std::wstring tail = L"  \u2014  always with you, no servers, no subscriptions.";

    FontFamily family(L"Segoe UI");
    Font font(&family, 12.0f, FontStyleRegular, UnitPixel);
    StringFormat format;
    format.SetAlignment(StringAlignment::StringAlignmentNear);
    format.SetLineAlignment(StringAlignment::StringAlignmentCenter);
    format.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);

    RectF leadBounds, tailBounds;
    g.MeasureString(lead.c_str(), -1, &font, PointF(0, 0), &format, &leadBounds);
    g.MeasureString(tail.c_str(), -1, &font, PointF(0, 0), &format, &tailBounds);

    float total = leadBounds.Width + tailBounds.Width + 36.0f;
    float startX = banner.X + (banner.Width - total) / 2.0f;
    float textY = banner.Y + (banner.Height - 16.0f) / 2.0f;

    neb::drawIcon(g, neb::Icon::Home, RectF(startX, textY + 1.0f, 14.0f, 14.0f), t.textSecondary, 1.7f);
    drawGradientText(g, lead, RectF(startX + 22.0f, textY, leadBounds.Width, 16.0f), 12.0f, false);

    SolidBrush tailBrush(t.textSecondary);
    g.DrawString(tail.c_str(), -1, &font, PointF(startX + 24.0f + leadBounds.Width, textY), &format,
                 &tailBrush);
}

// The breadcrumb bar: back / forward / refresh, the path, New Folder.
void drawBreadcrumbBar(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();

    neb::drawGlassPanel(g, area, 30.0f, false);

    float x = area.X + 12.0f;
    float cy = area.Y + area.Height / 2.0f;

    struct NavBtn {
        int id;
        neb::Icon icon;
        bool enabled;
    };
    NavBtn navs[] = {
        {HIT_BACK, neb::Icon::Back, g_app.historyIndex > 0},
        {HIT_FORWARD, neb::Icon::Forward,
         g_app.historyIndex >= 0 && g_app.historyIndex < static_cast<int>(g_app.pathHistory.size()) - 1},
        {HIT_REFRESH, neb::Icon::Refresh, true},
    };

    for (const NavBtn& nav : navs) {
        ElementState& state = element(nav.id);
        RectF btn(x, cy - 16.0f, 32.0f, 32.0f);
        bool hovered = nav.enabled && state.hover > 0.01f;
        if (hovered) {
            fillRound(g, btn, 16.0f, rgba(t.glassHover.GetR(), t.glassHover.GetG(),
                                           t.glassHover.GetB(), 190));
        }
        Color fg = nav.enabled ? t.textSecondary : rgba(t.textMuted.GetR(), t.textMuted.GetG(),
                                                         t.textMuted.GetB(), 90);
        neb::drawIcon(g, nav.icon, RectF(btn.X + 8.0f, btn.Y + 8.0f, 16.0f, 16.0f), fg, 2.0f);
        addHit(btn, nav.id);
        x += 34.0f;
    }

    // Divider.
    strokeRound(g, RectF(x + 2.0f, cy - 12.0f, 1.0f, 24.0f), 0.0f, t.border, 1.0f);
    x += 12.0f;

    // Breadcrumb items.
    RectF rootChip(x, cy - 15.0f, 68.0f, 30.0f);
    bool rootCurrent = g_app.currentPath.empty();
    if (!rootCurrent) {
        fillRound(g, rootChip, 15.0f, rgba(255, 255, 255, 10));
    }
    neb::drawIcon(g, neb::Icon::Home, RectF(rootChip.X + 9.0f, rootChip.Y + 8.0f, 14.0f, 14.0f),
                  rootCurrent ? t.text : t.textSecondary, 1.8f);
    neb::drawText(g, L"Root", RectF(rootChip.X + 26.0f, rootChip.Y, 38.0f, rootChip.Height), 12.0f, rootCurrent,
                  rootCurrent ? t.text : t.textSecondary, StringAlignment::StringAlignmentNear);
    addHit(rootChip, HIT_CARD, -2);

    x += 70.0f;

    for (size_t i = 0; i < g_app.currentPath.size(); ++i) {
        bool current = (i + 1 == g_app.currentPath.size());

        // Chevron separator.
        neb::drawIcon(g, neb::Icon::Forward, RectF(x, cy - 5.0f, 10.0f, 10.0f), t.textMuted, 1.6f);
        x += 14.0f;

        std::wstring name = neb::widen(g_app.currentPath[i]);
        SizeF measured = neb::measureText(g, name, 12.0f, current);
        RectF chip(x, cy - 15.0f, measured.Width + 24.0f, 30.0f);
        if (current) {
            neb::fillRoundAccent(g, chip, 15.0f);
        } else {
            fillRound(g, chip, 15.0f, rgba(255, 255, 255, 10));
        }
        neb::drawText(g, name, chip, 12.0f, current, current ? Color(255, 255, 255) : t.textSecondary);
        addHit(chip, HIT_CARD, -3, static_cast<int>(i));
        x += chip.Width + 2.0f;
    }

    // New Folder button.
    float btnW = 118.0f;
    RectF newBtn(area.X + area.Width - btnW - 12.0f, area.Y + (area.Height - 34.0f) / 2.0f, btnW, 34.0f);
    ElementState& newState = element(HIT_NEW_FOLDER);
    if (newState.hover > 0.01f) {
        LinearGradientBrush brush(
            PointF(newBtn.X, newBtn.Y), PointF(newBtn.X + newBtn.Width, newBtn.Y + newBtn.Height),
            rgba(t.accentA.GetR(), t.accentA.GetG(), t.accentA.GetB(), (BYTE)(255 * newState.hover)),
            rgba(t.accentB.GetR(), t.accentB.GetG(), t.accentB.GetB(), (BYTE)(255 * newState.hover)));
        GraphicsPath path; neb::addRoundedRect(path, newBtn.X, newBtn.Y, newBtn.Width, newBtn.Height, 17.0f);
        g.FillPath(&brush, &path);
    } else {
        neb::fillRoundAccent(g, newBtn, 17.0f);
    }
    neb::drawIcon(g, neb::Icon::FolderPlus, RectF(newBtn.X + 12.0f, newBtn.Y + 9.0f, 16.0f, 16.0f), Color(255, 255, 255));
    neb::drawText(g, L"New Folder", RectF(newBtn.X + 32.0f, newBtn.Y, btnW - 40.0f, newBtn.Height), 12.0f, true,
                  Color(255, 255, 255), StringAlignment::StringAlignmentNear);
    addHit(newBtn, HIT_NEW_FOLDER);
}

// The second toolbar: sort, view, favourites filter and the search field.
void drawToolbar(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();

    neb::drawGlassPanel(g, area, 30.0f, false);

    float x = area.X + 12.0f;
    float cy = area.Y + area.Height / 2.0f;

    struct SortOption {
        SortKey key;
        const wchar_t* label;
    };
    static const SortOption sorts[] = {
        {SortKey::Name, L"Sort: Name"}, {SortKey::Size, L"Sort: Size"},
        {SortKey::Date, L"Sort: Date"}, {SortKey::Type, L"Sort: Type"},
    };
    const wchar_t* sortLabel = L"Sort";
    for (const SortOption& option : sorts) {
        if (option.key == g_app.sortKey) sortLabel = option.label;
    }

    // Sort cycle button.
    RectF sortBtn(x, cy - 16.0f, 128.0f, 32.0f);
    ElementState& sortState = element(HIT_SORT);
    drawPill(g, sortBtn, 16.0f, sortState.hover > 0.01f, sortState.press, false, false);
    neb::drawText(g, sortLabel, RectF(sortBtn.X + 12.0f, sortBtn.Y, 96.0f, sortBtn.Height), 12.0f, false,
                  t.textSecondary, StringAlignment::StringAlignmentNear);
    neb::drawText(g, g_app.sortAscending ? L"\u25BE" : L"\u25BF",
                  RectF(sortBtn.X + sortBtn.Width - 26.0f, sortBtn.Y, 18.0f, sortBtn.Height), 11.0f, false,
                  t.textMuted);
    addHit(sortBtn, HIT_SORT);
    x += 136.0f;

    // View cycle button.
    static const wchar_t* viewLabels[] = {L"View: Small Grid", L"View: Medium Grid", L"View: Large Grid",
                                          L"View: List"};
    RectF viewBtn(x, cy - 16.0f, 148.0f, 32.0f);
    ElementState& viewState = element(HIT_VIEW);
    drawPill(g, viewBtn, 16.0f, viewState.hover > 0.01f, viewState.press, false, false);
    neb::drawText(g, viewLabels[static_cast<int>(g_app.view)], RectF(viewBtn.X + 12.0f, viewBtn.Y, 116.0f, viewBtn.Height),
                  12.0f, false, t.textSecondary, StringAlignment::StringAlignmentNear);
    neb::drawText(g, L"\u25BE", RectF(viewBtn.X + viewBtn.Width - 26.0f, viewBtn.Y, 18.0f, viewBtn.Height),
                  11.0f, false, t.textMuted);
    addHit(viewBtn, HIT_VIEW);
    x += 156.0f;

    // Favourites filter.
    RectF favBtn(x, cy - 16.0f, 104.0f, 32.0f);
    ElementState& favState = element(HIT_FAV_FILTER);
    drawPill(g, favBtn, 16.0f, favState.hover > 0.01f, favState.press, false, false);
    neb::drawIcon(g, neb::Icon::Star, RectF(favBtn.X + 11.0f, favBtn.Y + 8.0f, 16.0f, 16.0f), t.textSecondary, 2.0f);
    neb::drawText(g, L"Favorites", RectF(favBtn.X + 31.0f, favBtn.Y, 68.0f, favBtn.Height), 12.0f, false,
                  t.textSecondary, StringAlignment::StringAlignmentNear);
    addHit(favBtn, HIT_FAV_FILTER);

    // Search field on the right.
    float searchW = std::min(230.0f, area.Width * 0.22f);
    RectF searchRect(area.X + area.Width - searchW - 12.0f, cy - 17.0f, searchW, 34.0f);
    ElementState& searchState = element(HIT_SEARCH);
    Color searchFill = searchState.hover > 0.01f ? rgba(255, 255, 255, 18) : rgba(255, 255, 255, 10);
    fillRound(g, searchRect, 17.0f, searchFill);
    strokeRound(g, searchRect, 17.0f, searchState.hover > 0.01f ? t.accentSolid : t.border, 1.0f);
    neb::drawIcon(g, neb::Icon::Search, RectF(searchRect.X + 11.0f, searchRect.Y + 9.0f, 16.0f, 16.0f),
                  t.textMuted, 1.8f);

    std::wstring query = neb::widen(g_app.searchQuery);
    if (query.empty()) {
        neb::drawText(g, L"Search...", RectF(searchRect.X + 33.0f, searchRect.Y, searchRect.Width - 42.0f, searchRect.Height),
                      12.0f, false, t.textMuted, StringAlignment::StringAlignmentNear);
    } else {
        neb::drawText(g, query, RectF(searchRect.X + 33.0f, searchRect.Y, searchRect.Width - 42.0f, searchRect.Height),
                      12.0f, false, t.text, StringAlignment::StringAlignmentNear);
    }
    addHit(searchRect, HIT_SEARCH);
}

// The dashed drop zone.
void drawDropZone(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();

    bool over = g_app.dragOver;
    Color fill = over ? rgba(t.accentA.GetR(), t.accentA.GetG(), t.accentA.GetB(), 40) : rgba(255, 255, 255, 6);
    fillRound(g, area, 16.0f, fill);
    strokeRound(g, area, 16.0f, over ? t.accentSolid : t.border, over ? 2.0f : 1.0f);

    const std::wstring text =
        over ? L"Release to add these files"
              : L"Drop files anywhere, click to browse, or Ctrl+V to paste";

    neb::drawIcon(g, neb::Icon::Upload, RectF(area.X + area.Width / 2.0f - 100.0f, area.Y + 15.0f, 16.0f, 16.0f),
                  over ? t.accentSolid : t.textSecondary, 1.9f);
    neb::drawText(g, text, RectF(area.X + area.Width / 2.0f - 80.0f, area.Y, area.Width * 0.6f, area.Height),
                  12.0f, false, over ? t.text : t.textSecondary, StringAlignment::StringAlignmentNear);

    addHit(area, HIT_DROPZONE);
}

// The "Select" toggle row and the bulk action bar.
void drawSelectRow(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();

    RectF toggle(area.X, area.Y + 1.0f, 92.0f, 30.0f);
    ElementState& state = element(HIT_SELECT_TOGGLE);
    bool active = g_app.selectionMode;
    if (active) {
        neb::fillRoundAccent(g, toggle, 15.0f);
    } else {
        fillRound(g, toggle, 15.0f, rgba(255, 255, 255, 10));
        strokeRound(g, toggle, 15.0f, t.border, 1.0f);
    }
    RectF dot(toggle.X + 12.0f, toggle.Y + 12.0f, 6.0f, 6.0f);
    fillRound(g, dot, 3.0f, active ? Color(120, 230, 150) : rgba(t.textMuted.GetR(), t.textMuted.GetG(),
                                                                     t.textMuted.GetB(), 170));
    neb::drawText(g, L"Select", RectF(toggle.X + 24.0f, toggle.Y, 60.0f, toggle.Height), 12.0f, active,
                  active ? Color(255, 255, 255) : t.textSecondary, StringAlignment::StringAlignmentNear);
    addHit(toggle, HIT_SELECT_TOGGLE);

    neb::drawText(g, L"Select mode unlocks bulk Move / Export / Delete",
                  RectF(toggle.X + toggle.Width + 12.0f, toggle.Y, 460.0f, toggle.Height), 11.5f, false,
                  rgba(t.textSecondary.GetR(), t.textSecondary.GetG(), t.textSecondary.GetB(), 160),
                  StringAlignment::StringAlignmentNear);

    if (!g_app.selectionMode) return;

    // The bulk action bar slides in underneath.
    RectF bar(area.X, area.Y + 38.0f, area.Width, 42.0f);
    fillRound(g, bar, 21.0f, rgba(255, 255, 255, 10));
    strokeRound(g, bar, 21.0f, t.border, 1.0f);

    // Select all.
    RectF allBtn(bar.X + 10.0f, bar.Y + 6.0f, 104.0f, 30.0f);
    drawPill(g, allBtn, 15.0f, element(HIT_SELECT_ALL).hover > 0.01f, 0.0f, false, false);
    neb::drawIcon(g, g_app.selected.size() == g_app.files.size() && !g_app.files.empty()
                          ? neb::Icon::Check
                          : neb::Icon::Star,
                  RectF(allBtn.X + 10.0f, allBtn.Y + 7.0f, 16.0f, 16.0f), t.textSecondary, 1.9f);
    neb::drawText(g, L"Select All", RectF(allBtn.X + 30.0f, allBtn.Y, 70.0f, allBtn.Height), 11.5f, false,
                  t.textSecondary, StringAlignment::StringAlignmentNear);
    addHit(allBtn, HIT_SELECT_ALL);

    // Selected count.
    std::wstring count = std::to_wstring(g_app.selected.size()) + L" selected";
    neb::drawText(g, count, RectF(allBtn.X + allBtn.Width + 14.0f, bar.Y, 120.0f, bar.Height), 11.5f, true,
                  t.accentSolid, StringAlignment::StringAlignmentNear);

    // Actions, right aligned.
    struct BulkAction {
        int id;
        const wchar_t* label;
        bool danger;
    };
    BulkAction actions[] = {
        {HIT_SELECT_MOVE, L"Move", false},
        {HIT_SELECT_EXPORT, L"Export", false},
        {HIT_SELECT_DELETE, L"Delete", true},
        {HIT_SELECT_DESELECT, L"Deselect", false},
    };

    float x = bar.X + bar.Width - 10.0f;
    for (int i = 3; i >= 0; --i) {
        SizeF measured = neb::measureText(g, actions[i].label, 11.5f, true);
        float w = measured.Width + 30.0f;
        RectF btn(x - w, bar.Y + 6.0f, w, 30.0f);
        ElementState& actionState = element(actions[i].id);
        Color tone = actions[i].danger ? Color(248, 113, 113) : t.textSecondary;
        fillRound(g, btn, 15.0f, rgba(255, 255, 255, actionState.hover > 0.01f ? 16 : 8));
        if (actions[i].danger) {
            strokeRound(g, btn, 15.0f, rgba(248, 113, 113, 110), 1.0f);
        }
        neb::drawText(g, actions[i].label, btn, 11.5f, true, tone);
        addHit(btn, actions[i].id);
        x -= w + 8.0f;
    }
}

// Draws one file/folder card in the grid.
void drawCard(Graphics& g, const RectF& rect, const FileRecord& item, bool isFolder, int index,
              uint64_t size, bool selected) {
    const neb::Theme& t = neb::currentTheme();

    int hoverKey = cardKey(HIT_CARD, index);
    ElementState& state = element(hoverKey);
    float hover = state.hover;
    float press = state.press;

    // Hover lift and press squash.
    float lift = motionEnabled() ? -3.0f * hover : 0.0f;
    float squash = motionEnabled() ? 1.0f - 0.02f * press : 1.0f;
    RectF r(rect.X + rect.Width * (1.0f - squash) * 0.5f, rect.Y + lift, rect.Width * squash, rect.Height);

    // The card keeps the theme's own translucency so it reads as a subtle
    // panel rather than a solid block.
    Color fill = t.cardBg;
    if (selected) {
        fill = rgba(t.accentA.GetR(), t.accentA.GetG(), t.accentA.GetB(), 60);
    } else if (hover > 0.01f) {
        fill = rgba(t.cardBg.GetR(), t.cardBg.GetG(), t.cardBg.GetB(),
                    (BYTE)(t.cardBg.GetAlpha() + 26 * hover));
    }
    fillRound(g, r, 20.0f, fill);
    strokeRound(g, r, 20.0f, selected ? t.accentSolid : (hover > 0.01f ? t.borderStrong : t.border), 1.0f);

    if (hover > 0.01f && !t.light) {
        Color glow = t.accentSolid;
        glow = Color::MakeARGB((BYTE)(70 * hover), glow.GetR(), glow.GetG(), glow.GetB());
        strokeRound(g, RectF(r.X - 1.0f, r.Y - 1.0f, r.Width + 2.0f, r.Height + 2.0f), 21.0f, glow, 2.0f);
    }

    float cardW, cardH, iconSize;
    cardMetrics(g_app.view, cardW, cardH, iconSize);

    // In select mode a checkbox appears on the card.
    if (g_app.selectionMode) {
        RectF check(r.X + 10.0f, r.Y + 10.0f, 22.0f, 22.0f);
        fillRound(g, check, 11.0f, selected ? t.accentSolid : rgba(255, 255, 255, 26));
        strokeRound(g, check, 11.0f, selected ? t.accentSolid : t.borderStrong, 1.0f);
        if (selected) {
            neb::drawText(g, L"\u2713", check, 12.0f, true, Color(255, 255, 255));
        }
    }

    if (g_app.view == ViewMode::List) {
        // Row layout: icon, name, size, actions.
        // The card itself is registered first so its buttons take priority.
        addHit(r, HIT_CARD, item.id, index, isFolder);
        RectF iconRect(r.X + 12.0f, r.Y + (r.Height - iconSize) / 2.0f, iconSize, iconSize);
        fillRound(g, iconRect, iconSize / 3.0f, rgba(255, 255, 255, 16));
        neb::drawIcon(g, isFolder ? neb::Icon::Folder : iconForFile(item),
                      RectF(iconRect.X + 9.0f, iconRect.Y + 9.0f, iconSize - 18.0f, iconSize - 18.0f),
                      isFolder ? t.accentSolid : t.textSecondary, 1.9f);

        neb::drawText(g, neb::widen(item.name), RectF(r.X + iconSize + 30.0f, r.Y, r.Width * 0.42f, r.Height),
                      13.0f, false, t.text, StringAlignment::StringAlignmentNear);
        neb::drawText(g, neb::widen(formatSizePlain(size)), RectF(r.X + r.Width * 0.62f, r.Y, 90.0f, r.Height),
                      11.5f, false, t.textMuted, StringAlignment::StringAlignmentFar);

        // Star.
        RectF starBtn(r.X + r.Width - 116.0f, r.Y + (r.Height - 28.0f) / 2.0f, 28.0f, 28.0f);
        neb::drawIcon(g, item.isFavorite ? neb::Icon::StarFilled : neb::Icon::Star,
                      RectF(starBtn.X + 5.0f, starBtn.Y + 5.0f, 18.0f, 18.0f),
                      item.isFavorite ? Color(255, 200, 70) : t.textMuted, 1.9f);
        addHit(starBtn, HIT_CARD_STAR, item.id, index, isFolder);

        // More.
        RectF moreBtn(r.X + r.Width - 78.0f, r.Y + (r.Height - 28.0f) / 2.0f, 62.0f, 28.0f);
        neb::drawIcon(g, neb::Icon::More, RectF(moreBtn.X + 6.0f, moreBtn.Y + 6.0f, 16.0f, 16.0f), t.textMuted);
        neb::drawText(g, L"More", RectF(moreBtn.X + 24.0f, moreBtn.Y, 40.0f, moreBtn.Height), 11.5f, false,
                      t.textMuted, StringAlignment::StringAlignmentNear);
        addHit(moreBtn, HIT_CARD_MORE, item.id, index, isFolder);

        return;
    }

    // Grid layout. The card is registered before its buttons so the buttons
    // take priority when hit-testing.
    addHit(r, HIT_CARD, item.id, index, isFolder);

    float iconX = r.X + (r.Width - iconSize) / 2.0f;
    RectF iconDisc(iconX, r.Y + 18.0f, iconSize, iconSize);
    Color discFill = isFolder ? rgba(t.accentA.GetR(), t.accentA.GetG(), t.accentA.GetB(), 34)
                              : rgba(255, 255, 255, 16);
    fillRound(g, iconDisc, iconSize / 3.0f, discFill);
    if (hover > 0.01f) {
        strokeRound(g, iconDisc, iconSize / 3.0f, t.accentSolid, 1.0f);
    }
    neb::drawIcon(g, isFolder ? neb::Icon::Folder : iconForFile(item),
                  RectF(iconDisc.X + iconSize * 0.22f, iconDisc.Y + iconSize * 0.22f, iconSize * 0.56f,
                        iconSize * 0.56f),
                  isFolder ? t.accentSolid : t.textSecondary, 1.9f);

    // Name.
    RectF nameRect(r.X + 10.0f, iconDisc.Y + iconSize + 10.0f, r.Width - 20.0f, 34.0f);
    neb::drawText(g, neb::widen(item.name), nameRect, 12.5f, true, t.text);

    // Size pill.
    RectF sizePill(r.X + r.Width / 2.0f - 34.0f, nameRect.Y + nameRect.Height + 2.0f, 68.0f, 22.0f);
    fillRound(g, sizePill, 11.0f, rgba(255, 255, 255, 18));
    neb::drawText(g, neb::widen(formatSizePlain(size)), sizePill, 10.5f, true, t.textSecondary);

    // Action row.
    float actionY = r.Y + r.Height - 44.0f;
    RectF starBtn(r.X + 12.0f, actionY, 30.0f, 30.0f);
    neb::drawIcon(g, item.isFavorite ? neb::Icon::StarFilled : neb::Icon::Star,
                  RectF(starBtn.X + 6.0f, starBtn.Y + 6.0f, 18.0f, 18.0f),
                  item.isFavorite ? Color(255, 200, 70) : t.textMuted, 1.9f);
    addHit(starBtn, HIT_CARD_STAR, item.id, index, isFolder);

    RectF moreBtn(r.X + r.Width - 74.0f, actionY, 62.0f, 30.0f);
    neb::drawIcon(g, neb::Icon::More, RectF(moreBtn.X + 7.0f, moreBtn.Y + 7.0f, 16.0f, 16.0f), t.textMuted);
    neb::drawText(g, L"More", RectF(moreBtn.X + 25.0f, moreBtn.Y, 40.0f, moreBtn.Height), 11.0f, false,
                  t.textMuted, StringAlignment::StringAlignmentNear);
    addHit(moreBtn, HIT_CARD_MORE, item.id, index, isFolder);

}

// The footer strip with the legal links.
void drawFooter(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();

    neb::drawGlassPanel(g, area, 16.0f, false);

    neb::drawText(g, L"\u00A9 2026 NebulaDrive Inc. All rights reserved.",
                  RectF(area.X + 18.0f, area.Y, area.Width * 0.5f, area.Height), 11.0f, false, t.textMuted,
                  StringAlignment::StringAlignmentNear);

    struct FooterLink {
        const wchar_t* label;
        int id;
        Color color;
    };
    static const FooterLink links[] = {
        {L"Privacy Policy", HIT_FOOTER_PRIVACY, Color(120, 170, 255)},
        {L"Terms of Service", HIT_FOOTER_TERMS, Color(170, 150, 255)},
        {L"Cookie Policy", HIT_FOOTER_COOKIE, Color(90, 220, 190)},
        {L"Feedback", HIT_FOOTER_FEEDBACK, Color(255, 140, 150)},
    };

    float x = area.X + area.Width - 18.0f;
    for (int i = 3; i >= 0; --i) {
        SizeF measured = neb::measureText(g, links[i].label, 11.0f, false);
        float w = measured.Width + 30.0f;
        RectF btn(x - w, area.Y + (area.Height - 30.0f) / 2.0f, w, 30.0f);
        ElementState& state = element(links[i].id);
        if (state.hover > 0.01f) {
            fillRound(g, btn, 15.0f, rgba(links[i].color.GetR(), links[i].color.GetG(),
                                           links[i].color.GetB(), (BYTE)(60 * state.hover)));
        }
        strokeRound(g, btn, 15.0f, rgba(links[i].color.GetR(), links[i].color.GetG(), links[i].color.GetB(), 90), 1.0f);
        neb::drawText(g, links[i].label, btn, 11.0f, false, links[i].color);
        addHit(btn, links[i].id);
        x -= w + 8.0f;
    }
}

// The bottom bar shows the current selection and library stats.
void drawBottomBar(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();

    RectF bar(area.X + 10.0f, area.Y + 8.0f, area.Width - 20.0f, area.Height - 16.0f);
    neb::drawGlassPanel(g, bar, 18.0f, false);

    // Library icon.
    RectF artwork(bar.X + 12.0f, bar.Y + 9.0f, 40.0f, 40.0f);
    LinearGradientBrush artworkBrush(PointF(artwork.X, artwork.Y), PointF(artwork.X + 40.0f, artwork.Y + 40.0f),
                                     t.accentA, t.accentB);
    GraphicsPath artworkPath; neb::addRoundedRect(artworkPath, artwork.X, artwork.Y, 40.0f, 40.0f, 10.0f);
    g.FillPath(&artworkBrush, &artworkPath);
    neb::drawIcon(g, neb::Icon::Folder, RectF(artwork.X + 11.0f, artwork.Y + 11.0f, 18.0f, 18.0f), Color(255, 255, 255));

    std::wstring title = L"NebulaDrive";
    std::wstring subtitle = std::to_wstring(g_app.stats.totalFiles) + L" files  \u00B7  " +
                            neb::widen(formatSizePlain(g_app.stats.totalSize)) + L" used  \u00B7  " +
                            std::to_wstring(g_app.stats.totalFolders) + L" folders";

    if (g_app.bottomFileId > 0 && gDatabase) {
        auto record = gDatabase->getFile(g_app.bottomFileId);
        if (record) {
            title = neb::widen(record->name);
            subtitle = neb::widen(record->path);
        }
    }

    neb::drawText(g, title, RectF(artwork.X + 52.0f, bar.Y + 8.0f, bar.Width * 0.4f, 20.0f), 12.5f, true, t.text,
                  StringAlignment::StringAlignmentNear);
    neb::drawText(g, subtitle, RectF(artwork.X + 52.0f, bar.Y + 26.0f, bar.Width * 0.5f, 18.0f), 10.5f, false,
                  t.textMuted, StringAlignment::StringAlignmentNear);

    // Right side: open and clear.
    float right = bar.X + bar.Width - 12.0f;

    RectF clearBtn(right - 92.0f, bar.Y + 14.0f, 86.0f, 30.0f);
    ElementState& clearState = element(HIT_BOTTOM_CLEAR);
    drawPill(g, clearBtn, 15.0f, clearState.hover > 0.01f, clearState.press, false, false);
    neb::drawText(g, L"Clear", clearBtn, 11.5f, false, t.textMuted);
    addHit(clearBtn, HIT_BOTTOM_CLEAR);

    RectF openBtn(right - 92.0f - 96.0f, bar.Y + 14.0f, 88.0f, 30.0f);
    ElementState& openState = element(HIT_BOTTOM_OPEN);
    drawPill(g, openBtn, 15.0f, openState.hover > 0.01f, openState.press, false, false);
    neb::drawText(g, L"Open", openBtn, 11.5f, false, t.textSecondary);
    addHit(openBtn, HIT_BOTTOM_OPEN);
}

// Draws a toast notification in the bottom-right corner.
void drawToasts(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();

    const float cardW = 330.0f;
    const float cardH = 54.0f;
    const float gap = 10.0f;
    const float margin = 24.0f;

    // Toasts stack upward from just above the footer and the bottom bar, so
    // they never cover the legal links or the player controls.
    float y = area.Y + area.Height - metrics().footerHeight - metrics().bottomHeight - 20.0f - cardH;

    for (size_t i = g_toasts.size(); i > 0; --i) {
        Toast& toast = g_toasts[i - 1];

        Color accent = t.info;
        switch (toast.kind) {
            case ToastKind::Success: accent = Color(52, 211, 153); break;
            case ToastKind::Error: accent = Color(248, 113, 113); break;
            case ToastKind::Warn: accent = Color(251, 191, 36); break;
            default: break;
        }

        float slide = motionEnabled() ? clampf(toast.slide, 0.0f, 1.0f) : 1.0f;
        float offset = (1.0f - slide) * 40.0f;

        RectF card(area.X + area.Width - margin - cardW + offset, y, cardW, cardH);
        fillRound(g, card, 16.0f, rgba(22, 20, 46, 240));
        strokeRound(g, card, 16.0f, rgba(accent.GetR(), accent.GetG(), accent.GetB(), 110), 1.0f);

        // Accent rail.
        fillRound(g, RectF(card.X + 1.0f, card.Y + 12.0f, 3.0f, card.Height - 24.0f), 2.0f, accent);

        neb::drawIcon(g, toast.kind == ToastKind::Error   ? neb::Icon::Info
                       : toast.kind == ToastKind::Success ? neb::Icon::Check
                       : toast.kind == ToastKind::Warn    ? neb::Icon::Info
                                                          : neb::Icon::Info,
                      RectF(card.X + 14.0f, card.Y + 18.0f, 18.0f, 18.0f), accent, 2.0f);

        neb::drawText(g, toast.message, RectF(card.X + 40.0f, card.Y, cardW - 52.0f, card.Height), 12.0f, true,
                      Color(255, 255, 255), StringAlignment::StringAlignmentNear);

        y -= cardH + gap;
    }
}

// A small modal used for settings and info.
// ------------------------------------------------------------------
// Legal documents and feedback panel
// ------------------------------------------------------------------
struct LegalPara {
    const wchar_t* head;
    const wchar_t* body;
};

const LegalPara kPrivacyDoc[] = {
    {L"Your files stay on this PC",
     L"NebulaDrive copies the files you add into a local folder called NebulaDriveData next to the app. "
     L"Nothing is uploaded to a server, because this desktop build has no network features."},
    {L"What the app stores",
     L"A small local database keeps each file's name, location, size, type, checksum, favorite flag and the "
     L"history of actions (uploads, renames, deletes). It also keeps your settings such as theme and language."},
    {L"No tracking",
     L"There are no accounts to create, no analytics and no advertising. The profile shown in the top bar is "
     L"only a local label."},
    {L"Deleting data",
     L"Deleted items move to Trash first. Emptying the Trash removes them from disk for good. Deleting the "
     L"NebulaDriveData folder removes everything NebulaDrive has stored."},
    {L"Feedback you write",
     L"Feedback is saved to feedback.txt inside NebulaDriveData. It is never sent anywhere unless you choose to "
     L"share that file yourself."},
};

const LegalPara kTermsDoc[] = {
    {L"Using NebulaDrive",
     L"NebulaDrive is a personal file library for your own computer. By using it you agree to these terms."},
    {L"Beta software",
     L"This is a beta release. Features can change and bugs can happen, so keep a second copy of anything "
     L"important. The app is provided as is, without any warranty."},
    {L"Your content",
     L"You keep all rights to the files you store. You are responsible for having the right to store them and "
     L"for following the laws that apply to you."},
    {L"Backups",
     L"Use the backup tools in Settings to export your library. You are responsible for keeping your backups "
     L"safe."},
    {L"Changes",
     L"These terms may be updated with new versions of the app. Continuing to use NebulaDrive after an update "
     L"means you accept the new terms."},
};

const LegalPara kCookieDoc[] = {
    {L"Desktop app",
     L"NebulaDrive for Windows does not use cookies and does not load web pages that could set them."},
    {L"Local settings",
     L"Preferences such as theme, language, view mode and toggles are saved in the local database on this "
     L"PC, not in a browser."},
    {L"Browser version",
     L"The browser version of NebulaDrive can keep similar preferences in your browser's own storage. You can "
     L"clear that storage at any time from your browser settings."},
};

const wchar_t* legalTitle(int doc) {
    switch (doc) {
        case 0: return L"Privacy Policy";
        case 1: return L"Terms of Service";
        case 2: return L"Cookie Policy";
        default: return L"Feedback";
    }
}

const wchar_t* legalSubtitle(int doc) {
    switch (doc) {
        case 0: return L"How NebulaDrive handles your data.";
        case 1: return L"The rules for using the app.";
        case 2: return L"What the app stores, and where.";
        default: return L"Your ideas make NebulaDrive better.";
    }
}

// Greedy word wrap, cached so the text is only measured once per width.
const std::vector<std::wstring>& wrapLines(Graphics& g, const std::wstring& text, float width, float size) {
    static std::map<std::pair<std::wstring, int>, std::vector<std::wstring>> cache;
    auto key = std::make_pair(text, static_cast<int>(width));
    auto found = cache.find(key);
    if (found != cache.end()) return found->second;

    std::vector<std::wstring> lines;
    std::wstring line;
    size_t pos = 0;
    while (pos < text.size()) {
        size_t space = text.find(L' ', pos);
        if (space == std::wstring::npos) space = text.size();
        std::wstring word = text.substr(pos, space - pos);
        pos = space + 1;
        std::wstring candidate = line.empty() ? word : line + L" " + word;
        if (!line.empty() && neb::measureText(g, candidate, size, false).Width > width) {
            lines.push_back(line);
            line = word;
        } else {
            line = candidate;
        }
    }
    if (!line.empty()) lines.push_back(line);
    return cache.emplace(key, lines).first->second;
}

void drawModal(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();
    float anim = clampf(g_app.modalAnim, 0.0f, 1.0f);

    // Dim the workspace.
    fillRound(g, area, 0.0f, rgba(0, 0, 0, (BYTE)(150 * anim)));

    bool isSettings = g_app.modal == ModalKind::Settings;
    bool isLegal = g_app.modal == ModalKind::Legal;
    g_hitFloor = g_hits.size();
    float cardW = std::min(isSettings ? 660.0f : 600.0f, area.Width - 60.0f);
    float cardH = std::min(isSettings ? 660.0f : 560.0f, area.Height - 60.0f);

    float ease = 1.0f - std::pow(1.0f - anim, 3.0f);
    float scale = 0.95f + 0.05f * ease;
    float cx = area.X + area.Width / 2.0f;
    float cy = area.Y + area.Height / 2.0f;

    RectF card(cx - cardW * scale / 2.0f, cy - cardH * scale / 2.0f + (1.0f - ease) * 22.0f,
               cardW * scale, cardH * scale);

    fillRoundGradientV(g, card, 26.0f, rgba(30, 27, 58, (BYTE)(248 * anim)),
                        rgba(20, 18, 42, (BYTE)(248 * anim)));
    strokeRound(g, card, 26.0f, rgba(t.borderStrong.GetR(), t.borderStrong.GetG(), t.borderStrong.GetB(),
                                      (BYTE)(200 * anim)),
                1.0f);

    addHit(card, HIT_NONE);  // clicks on the card itself never fall through

    float x = card.X + 26.0f;
    float y = card.Y + 22.0f;
    float w = card.Width - 52.0f;

    // Header.
    neb::drawIcon(g, isSettings ? neb::Icon::Gear : neb::Icon::Info, RectF(x, y + 2.0f, 22.0f, 22.0f),
                  t.accentSolid, 2.0f);
    neb::drawText(g, isSettings ? L"Advanced Settings" : isLegal ? legalTitle(g_app.legalDoc) : L"About NebulaDrive", RectF(x + 32.0f, y, w - 100.0f, 26.0f),
                  16.0f, true, t.text, StringAlignment::StringAlignmentNear);

    // Close button.
    RectF closeBtn(card.X + card.Width - 48.0f, card.Y + 18.0f, 30.0f, 30.0f);
    ElementState& closeState = element(HIT_MODAL_CLOSE);
    if (closeState.hover > 0.01f) {
        fillRound(g, closeBtn, 15.0f, rgba(239, 68, 68, (BYTE)(120 * closeState.hover)));
    }
    strokeRound(g, closeBtn, 15.0f, t.border, 1.0f);
    neb::drawText(g, L"\u2715", closeBtn, 13.0f, false, t.textSecondary);
    addHit(closeBtn, HIT_MODAL_CLOSE);

    y += 34.0f;
    neb::drawText(g, isSettings ? L"Control storage, behaviour and appearance."
                                : isLegal ? legalSubtitle(g_app.legalDoc)
                                          : L"Your personal, self-hosted file library.",
                  RectF(x, y, w, 18.0f), 11.5f, false, t.textMuted, StringAlignment::StringAlignmentNear);
    y += 30.0f;

    // Everything below the header scrolls inside the card, so the settings
    // never spill past the bottom edge.
    RectF viewport(card.X + 6.0f, y - 6.0f, card.Width - 12.0f, card.Y + card.Height - y - 18.0f);
    GraphicsState viewportState = g.Save();
    GraphicsPath viewportPath;
    neb::addRoundedRect(viewportPath, card.X, card.Y, card.Width, card.Height, 26.0f);
    g.SetClip(&viewportPath);
    g.TranslateTransform(0.0f, -g_app.modalScroll);
    float contentStart = y;

    // Section helper.
    auto sectionLabel = [&](const wchar_t* text) {
        neb::drawText(g, text, RectF(x, y, w, 18.0f), 11.0f, true, t.accentSolid,
                      StringAlignment::StringAlignmentNear);
        y += 24.0f;
    };

    // A toggle row.
    auto toggleRow = [&](int id, const wchar_t* label, const wchar_t* hint, bool on, bool enabled = true) {
        RectF row(x, y, w, 52.0f);
        fillRound(g, row, 14.0f, rgba(255, 255, 255, enabled ? 8 : 4));
        strokeRound(g, row, 14.0f, t.border, 1.0f);

        neb::drawText(g, label, RectF(x + 16.0f, y + 8.0f, w - 90.0f, 20.0f), 12.5f, true,
                      enabled ? t.text : t.textMuted, StringAlignment::StringAlignmentNear);
        neb::drawText(g, hint, RectF(x + 16.0f, y + 27.0f, w - 90.0f, 18.0f), 10.5f, false, t.textMuted,
                      StringAlignment::StringAlignmentNear);

        RectF sw(x + w - 62.0f, y + 14.0f, 46.0f, 24.0f);
        if (on) {
            neb::fillRoundAccent(g, sw, 12.0f);
        } else {
            fillRound(g, sw, 12.0f, rgba(255, 255, 255, 22));
        }
        float knobX = on ? sw.X + sw.Width - 21.0f : sw.X + 3.0f;
        fillRound(g, RectF(knobX, sw.Y + 3.0f, 18.0f, 18.0f), 9.0f, Color(255, 255, 255));

        addHit(row, id);
        y += 60.0f;
    };

    // An action row.
    auto actionRow = [&](int id, const wchar_t* label, const wchar_t* hint, const wchar_t* action,
                         int command) {
        RectF row(x, y, w, 52.0f);
        fillRound(g, row, 14.0f, rgba(255, 255, 255, 8));
        strokeRound(g, row, 14.0f, t.border, 1.0f);

        neb::drawText(g, label, RectF(x + 16.0f, y + 8.0f, w - 150.0f, 20.0f), 12.5f, true, t.text,
                      StringAlignment::StringAlignmentNear);
        neb::drawText(g, hint, RectF(x + 16.0f, y + 27.0f, w - 150.0f, 18.0f), 10.5f, false, t.textMuted,
                      StringAlignment::StringAlignmentNear);

        RectF btn(x + w - 108.0f, y + 11.0f, 92.0f, 30.0f);
        ElementState& state = rowState(id, command);
        drawPill(g, btn, 15.0f, state.hover > 0.01f, state.press, false, false);
        neb::drawText(g, action, btn, 11.5f, false, t.textSecondary);
        addHit(btn, id, command);

        y += 60.0f;
    };

    if (isSettings) {
        // --- Storage overview ---
        sectionLabel(L"STORAGE OVERVIEW");
        {
            struct Tile {
                const wchar_t* value;
                const wchar_t* label;
                neb::Icon icon;
            };
            Tile tiles[] = {
                {std::to_wstring(g_app.stats.totalFolders).c_str(), L"FOLDERS", neb::Icon::Folder},
                {std::to_wstring(g_app.stats.totalFiles).c_str(), L"FILES", neb::Icon::Document},
                {neb::widen(formatSizePlain(g_app.stats.totalSize)).c_str(), L"USED", neb::Icon::Archive},
                {std::to_wstring(g_app.stats.videoFiles).c_str(), L"VIDEOS", neb::Icon::Video},
                {std::to_wstring(g_app.stats.imageFiles).c_str(), L"IMAGES", neb::Icon::Image},
                {neb::widen(formatSizePlain(g_app.stats.totalSize + g_app.stats.trashSize)).c_str(), L"ON DISK",
                 neb::Icon::Archive},
            };

            float tileW = (w - 16.0f) / 2.0f;
            float tileH = 74.0f;
            for (int i = 0; i < 6; ++i) {
                RectF tile(x + (i % 2) * (tileW + 16.0f), y + (i / 2) * (tileH + 12.0f), tileW, tileH);
                fillRound(g, tile, 14.0f, rgba(255, 255, 255, 7));
                strokeRound(g, tile, 14.0f, t.border, 1.0f);
                neb::drawIcon(g, tiles[i].icon, RectF(tile.X + 14.0f, tile.Y + 14.0f, 16.0f, 16.0f),
                              t.accentSolid, 1.9f);
                neb::drawText(g, tiles[i].value, RectF(tile.X + 14.0f, tile.Y + 32.0f, tileW - 28.0f, 24.0f),
                              15.0f, true, t.accentSolid, StringAlignment::StringAlignmentNear);
                neb::drawText(g, tiles[i].label, RectF(tile.X + 14.0f, tile.Y + 54.0f, tileW - 28.0f, 16.0f),
                              9.5f, true, t.textMuted, StringAlignment::StringAlignmentNear);
            }
            y += 3 * (tileH + 12.0f) + 6.0f;

            // The wide "Local & Secure" tile.
            RectF wide(x, y, w, 62.0f);
            fillRound(g, wide, 14.0f, rgba(255, 255, 255, 7));
            strokeRound(g, wide, 14.0f, t.border, 1.0f);
            neb::drawIcon(g, neb::Icon::User, RectF(wide.X + 14.0f, wide.Y + 14.0f, 16.0f, 16.0f), t.accentSolid,
                          1.9f);
            neb::drawText(g, L"Local & Secure", RectF(wide.X + 14.0f, wide.Y + 32.0f, w - 28.0f, 20.0f), 12.0f,
                          true, t.accentSolid, StringAlignment::StringAlignmentNear);
            neb::drawText(g, L"EVERYTHING IN YOUR LIBRARY", RectF(wide.X + 14.0f, wide.Y + 50.0f, w - 28.0f, 14.0f),
                          8.5f, true, t.textMuted, StringAlignment::StringAlignmentNear);
            y += 74.0f;

            RectF note(x, y, w, 30.0f);
            fillRound(g, note, 10.0f, rgba(255, 255, 255, 5));
            strokeRound(g, note, 10.0f, t.border, 1.0f);
            neb::drawIcon(g, neb::Icon::Info, RectF(note.X + 10.0f, note.Y + 8.0f, 14.0f, 14.0f), t.textMuted, 1.8f);
            neb::drawText(g, L"Everything is stored locally and securely on this device.",
                          RectF(note.X + 30.0f, note.Y, w - 40.0f, note.Height), 10.5f, false, t.textMuted,
                          StringAlignment::StringAlignmentNear);
            y += 42.0f;
        }

        // --- Account controls ---
        sectionLabel(L"ACCOUNT CONTROLS");
        actionRow(HIT_MENU_ITEM, L"Set Password", L"Protect this account", L"Set Password", CMD_SET_PASSWORD);
        toggleRow(HIT_TGL_SAVE_LOGIN, L"Save Login on This Device", L"Stay signed in on this PC",
                  g_app.saveLogin);
        actionRow(HIT_MENU_ITEM, L"Export Account Login", L"Encrypted login and profile only", L"Download",
                  CMD_EXPORT_LOGIN);

        // Saved login details.
        {
            RectF box(x, y, w, 118.0f);
            fillRound(g, box, 14.0f, rgba(255, 255, 255, 6));
            strokeRound(g, box, 14.0f, t.border, 1.0f);
            neb::drawText(g, L"SAVED LOGIN", RectF(box.X + 16.0f, box.Y + 10.0f, w - 32.0f, 16.0f), 9.5f, true,
                          t.textMuted, StringAlignment::StringAlignmentNear);

            const wchar_t* rows[] = {L"Account name", L"Display name", L"Password"};
            const wchar_t* values[] = {L"Makerxm800", L"Makerxm800", L"••••••••"};
            for (int i = 0; i < 3; ++i) {
                float ry = box.Y + 32.0f + i * 22.0f;
                neb::drawText(g, rows[i], RectF(box.X + 16.0f, ry, 120.0f, 18.0f), 11.0f, false, t.textSecondary,
                              StringAlignment::StringAlignmentNear);
                neb::drawText(g, values[i], RectF(box.X + 140.0f, ry, w - 220.0f, 18.0f), 11.0f, true, t.text,
                              StringAlignment::StringAlignmentNear);
            }

            RectF showBtn(box.X + w - 78.0f, box.Y + 76.0f, 62.0f, 26.0f);
            drawPill(g, showBtn, 13.0f, false, 0.0f, false, false);
            neb::drawText(g, L"Show", showBtn, 10.5f, false, t.textSecondary);
            y += 130.0f;
        }

        actionRow(HIT_MENU_ITEM, L"Log Out", L"Sign out of this device", L"Log Out", CMD_LOGOUT);
        actionRow(HIT_MENU_ITEM, L"Reset Cache", L"Clears cached files only", L"Reset", CMD_RESET_CACHE);

        // --- Backup ---
        sectionLabel(L"BACKUP (ZIP)");
        {
            RectF box(x, y, w, 76.0f);
            fillRound(g, box, 14.0f, rgba(t.accentA.GetR(), t.accentA.GetG(), t.accentA.GetB(), 26));
            strokeRound(g, box, 14.0f, t.border, 1.0f);
            neb::drawIcon(g, neb::Icon::Archive, RectF(box.X + 18.0f, box.Y + 24.0f, 26.0f, 26.0f), t.accentSolid,
                          2.0f);
            neb::drawText(g, L"Download All Files as ZIP", RectF(box.X + 58.0f, box.Y + 18.0f, w - 190.0f, 20.0f),
                          13.0f, true, t.text, StringAlignment::StringAlignmentNear);
            neb::drawText(g, L"Exports a real .zip with your actual files inside.",
                          RectF(box.X + 58.0f, box.Y + 40.0f, w - 190.0f, 18.0f), 10.5f, false, t.textMuted,
                          StringAlignment::StringAlignmentNear);

            RectF btn(box.X + w - 118.0f, box.Y + 22.0f, 100.0f, 32.0f);
            drawPill(g, btn, 16.0f, rowState(HIT_MENU_ITEM, CMD_ZIP_EXPORT).hover > 0.01f, 0.0f, true, true);
            neb::drawText(g, L"Download", btn, 11.5f, true, Color(255, 255, 255));
            addHit(btn, HIT_MENU_ITEM, CMD_ZIP_EXPORT);
            y += 88.0f;
        }
        actionRow(HIT_MENU_ITEM, L"Restore from ZIP Backup", L"Bring a backup into this library", L"Import",
                  CMD_ZIP_IMPORT);

        // --- Diagnostics ---
        sectionLabel(L"DIAGNOSTICS");
        {
            RectF box(x, y, w, 76.0f);
            fillRound(g, box, 14.0f, rgba(255, 255, 255, 7));
            strokeRound(g, box, 14.0f, t.border, 1.0f);
            neb::drawIcon(g, neb::Icon::Check, RectF(box.X + 18.0f, box.Y + 24.0f, 26.0f, 26.0f), t.accentSolid,
                          2.0f);
            neb::drawText(g, L"Check your library", RectF(box.X + 58.0f, box.Y + 18.0f, w - 190.0f, 20.0f), 13.0f,
                          true, t.text, StringAlignment::StringAlignmentNear);
            neb::drawText(g, L"Find missing file data, incomplete uploads, and records that no longer match.",
                          RectF(box.X + 58.0f, box.Y + 40.0f, w - 190.0f, 18.0f), 10.5f, false, t.textMuted,
                          StringAlignment::StringAlignmentNear);

            RectF btn(box.X + w - 96.0f, box.Y + 22.0f, 78.0f, 32.0f);
            drawPill(g, btn, 16.0f, rowState(HIT_MENU_ITEM, CMD_SCAN).hover > 0.01f, 0.0f, false, false);
            neb::drawText(g, L"Scan", btn, 11.5f, false, t.textSecondary);
            addHit(btn, HIT_MENU_ITEM, CMD_SCAN);
            y += 88.0f;
        }

        // --- Themes ---
        sectionLabel(L"THEMES");
        toggleRow(HIT_TGL_LIQUID, L"Liquid Glass", L"site-wide glass effect", g_app.liquidGlass);
        toggleRow(HIT_TGL_ANIM, L"Animations", L"hover, slide and transition effects", g_app.animations);
        toggleRow(HIT_TGL_CALM, L"Reduce Motion", L"calm UI, kinder battery", g_app.calmMotion);
        toggleRow(HIT_TGL_PREMIUM_FX, L"Premium FX", L"PC-only wild animations", g_app.premiumFx);

        // All / Standard / Premium segmented control.
        {
            const wchar_t* labels[] = {L"All themes", L"Standard", L"Premium"};
            int commands[] = {CMD_THEME_ALL, CMD_THEME_STANDARD, CMD_THEME_PREMIUM};
            ThemeFilter filters[] = {ThemeFilter::All, ThemeFilter::Standard, ThemeFilter::Premium};

            float segW = (w - 8.0f) / 3.0f;
            RectF seg(x, y, w, 38.0f);
            fillRound(g, seg, 12.0f, rgba(255, 255, 255, 8));
            strokeRound(g, seg, 12.0f, t.border, 1.0f);

            for (int i = 0; i < 3; ++i) {
                RectF part(x + 4.0f + i * segW, y + 4.0f, segW - 2.0f, 30.0f);
                if (g_app.themeFilter == filters[i]) {
                    neb::fillRoundAccent(g, part, 10.0f);
                }
                neb::drawText(g, labels[i], part, 11.5f, g_app.themeFilter == filters[i],
                              g_app.themeFilter == filters[i] ? Color(255, 255, 255) : t.textSecondary);
                addHit(part, HIT_THEME_SEGMENT, commands[i]);
            }
            y += 48.0f;
            neb::drawText(g, L"Choose which theme styles you want to see below.",
                          RectF(x + 4.0f, y, w - 8.0f, 18.0f), 10.5f, false, t.textMuted,
                          StringAlignment::StringAlignmentNear);
            y += 26.0f;
        }

        // The theme grid.
        {
            const auto& names = neb::themeNames();
            float cellW = (w - 24.0f) / 3.0f;
            float cellH = 74.0f;
            int column = 0;
            float gridTop = y;

            for (size_t i = 0; i < names.size(); ++i) {
                bool premium = neb::themeIsPremium(i);
                if (g_app.themeFilter == ThemeFilter::Standard && premium) continue;
                if (g_app.themeFilter == ThemeFilter::Premium && !premium) continue;

                if (column == 0) {
                    if (i > 0 && !premium) {
                        // Section label for the premium group.
                        bool firstPremium = true;
                        for (size_t j = 0; j < i; ++j) {
                            if (neb::themeIsPremium(j)) {
                                firstPremium = false;
                                break;
                            }
                        }
                        if (firstPremium) {
                            y += 26.0f;
                            neb::drawText(g, L"PREMIUM", RectF(x, y, w, 18.0f), 10.0f, true, t.textMuted,
                                          StringAlignment::StringAlignmentNear);
                            y += 22.0f;
                        }
                    }
                }

                const neb::Theme* theme = neb::findTheme(names[i]);
                bool active = names[i] == g_app.themeName;

                RectF cell(x + column * (cellW + 12.0f), y, cellW, cellH);
                fillRound(g, cell, 14.0f, rgba(255, 255, 255, 7));
                strokeRound(g, cell, 14.0f, active ? t.accentSolid : t.border, active ? 2.0f : 1.0f);

                RectF swatch(cell.X + 10.0f, cell.Y + 10.0f, cellW - 20.0f, 30.0f);
                LinearGradientBrush brush(PointF(swatch.X, swatch.Y), PointF(swatch.X + swatch.Width, swatch.Y),
                                         theme ? theme->accentA : t.accentA,
                                         theme ? theme->accentB : t.accentB);
                GraphicsPath path; neb::addRoundedRect(path, swatch.X, swatch.Y, swatch.Width, swatch.Height, 10.0f);
                g.FillPath(&brush, &path);

                neb::drawText(g, neb::widen(names[i]), RectF(cell.X, cell.Y + 42.0f, cellW, 16.0f), 10.0f,
                              active, active ? t.text : t.textSecondary);

                if (premium) {
                    // The little gold premium badge.
                    RectF badge(cell.X + cellW - 30.0f, cell.Y + 48.0f, 22.0f, 14.0f);
                    fillRound(g, badge, 7.0f, rgba(245, 200, 90, 235));
                    neb::drawText(g, L"*", badge, 9.0f, true, Color(60, 44, 10));
                }

                addHit(cell, HIT_THEME, static_cast<int>(i));

                if (++column == 3) {
                    column = 0;
                    y += cellH + 10.0f;
                }
            }
            if (column != 0) {
                y += cellH + 10.0f;
            }
            y = gridTop + (y - gridTop) + 14.0f;
        }

        // --- Fonts ---
        sectionLabel(L"FONTS");
        {
            const wchar_t* fonts[] = {L"Nebula Sans", L"Aurora Serif", L"Comet Mono"};
            float cellW = (w - 24.0f) / 3.0f;
            for (int i = 0; i < 3; ++i) {
                RectF cell(x + i * (cellW + 12.0f), y, cellW, 72.0f);
                fillRound(g, cell, 14.0f, rgba(255, 255, 255, 7));
                strokeRound(g, cell, 14.0f, t.border, 1.0f);
                neb::drawText(g, L"Aa Bb 123", RectF(cell.X + 12.0f, cell.Y + 10.0f, cellW - 24.0f, 26.0f), 17.0f,
                              true, t.text, StringAlignment::StringAlignmentNear);
                neb::drawText(g, fonts[i], RectF(cell.X + 12.0f, cell.Y + 40.0f, cellW - 24.0f, 16.0f), 10.0f,
                              true, t.textSecondary, StringAlignment::StringAlignmentNear);

                RectF badge(cell.X + cellW - 74.0f, cell.Y + 8.0f, 62.0f, 15.0f);
                fillRound(g, badge, 7.0f, rgba(167, 139, 250, 220));
                neb::drawText(g, L"COMING SOON", badge, 7.0f, true, Color(255, 255, 255));
            }
            y += 86.0f;
        }

        // --- Feedback ---
        sectionLabel(L"FEEDBACK");
        {
            RectF box(x, y, w, 66.0f);
            fillRound(g, box, 14.0f, rgba(255, 255, 255, 7));
            strokeRound(g, box, 14.0f, t.border, 1.0f);
            neb::drawIcon(g, neb::Icon::Info, RectF(box.X + 16.0f, box.Y + 12.0f, 16.0f, 16.0f), t.textMuted, 1.8f);
            neb::drawText(g, L"Rate the app, report a bug, or customise the questions people are asked.",
                          RectF(box.X + 38.0f, box.Y + 12.0f, w - 54.0f, 18.0f), 10.5f, false, t.textSecondary,
                          StringAlignment::StringAlignmentNear);

            RectF give(x + 16.0f, box.Y + 36.0f, 118.0f, 24.0f);
            neb::fillRoundAccent(g, give, 12.0f);
            neb::drawText(g, L"Give feedback", give, 10.5f, true, Color(255, 255, 255));
            addHit(give, HIT_MENU_ITEM, CMD_FEEDBACK);

            RectF custom(x + 142.0f, box.Y + 36.0f, 152.0f, 24.0f);
            fillRound(g, custom, 12.0f, rgba(255, 255, 255, 12));
            strokeRound(g, custom, 12.0f, t.border, 1.0f);
            neb::drawText(g, L"Customize questions", custom, 10.5f, true, t.textSecondary);
            addHit(custom, HIT_MENU_ITEM, CMD_FEEDBACK);
            y += 78.0f;
        }

        // --- Danger zone ---
        sectionLabel(L"DANGER ZONE");
        {
            RectF box(x, y, w, 150.0f);
            fillRound(g, box, 14.0f, rgba(239, 68, 68, 12));
            strokeRound(g, box, 14.0f, rgba(239, 68, 68, 70), 1.0f);

            struct DangerRow {
                const wchar_t* label;
                int command;
                bool fatal;
            };
            DangerRow dangers[] = {
                {L"Clear Cache (Delete All Files)", CMD_DANGER_CLEAR, false},
                {L"Delete Account", CMD_DANGER_DELETE, false},
                {L"Terminate Account (Permanent)", CMD_DANGER_TERMINATE, true},
            };

            float dy = box.Y + 34.0f;
            for (const DangerRow& danger : dangers) {
                RectF row(box.X + 16.0f, dy, w - 32.0f, 34.0f);
                Color tone = danger.fatal ? Color(239, 68, 68) : Color(251, 191, 36);
                fillRound(g, row, 10.0f, rgba(tone.GetR(), tone.GetG(), tone.GetB(), danger.fatal ? 46 : 26));
                strokeRound(g, row, 10.0f, rgba(tone.GetR(), tone.GetG(), tone.GetB(), 110), 1.0f);
                neb::drawIcon(g, danger.fatal ? neb::Icon::Trash : neb::Icon::Info,
                              RectF(row.X + 10.0f, row.Y + 9.0f, 16.0f, 16.0f), tone, 1.9f);
                neb::drawText(g, danger.label, RectF(row.X + 34.0f, row.Y, row.Width - 44.0f, row.Height), 11.5f,
                              true, tone, StringAlignment::StringAlignmentNear);
                addHit(row, HIT_MENU_ITEM, danger.command);
                dy += 40.0f;
            }
            y += 162.0f;
        }

        // Close row.
        {
            RectF btn(x, y, w, 36.0f);
            fillRound(g, btn, 12.0f, rgba(255, 255, 255, 10));
            strokeRound(g, btn, 12.0f, t.border, 1.0f);
            neb::drawText(g, L"Close", btn, 12.0f, true, t.textSecondary);
            addHit(btn, HIT_MODAL_CLOSE);
            y += 48.0f;
        }
    } else if (isLegal) {
        int doc = g_app.legalDoc;
        if (doc == 3) {
            sectionLabel(L"SHARE YOUR THOUGHTS");
            auto feedbackRow = [&](const wchar_t* label, const wchar_t* hint, const wchar_t* action, int command,
                                   bool primary) {
                RectF row(x, y, w, 52.0f);
                fillRound(g, row, 14.0f, rgba(255, 255, 255, 8));
                strokeRound(g, row, 14.0f, t.border, 1.0f);
                neb::drawText(g, label, RectF(x + 16.0f, y + 8.0f, w - 150.0f, 20.0f), 12.5f, true, t.text,
                              StringAlignment::StringAlignmentNear);
                neb::drawText(g, hint, RectF(x + 16.0f, y + 27.0f, w - 150.0f, 18.0f), 10.5f, false, t.textMuted,
                              StringAlignment::StringAlignmentNear);
                RectF btn(x + w - 112.0f, y + 11.0f, 96.0f, 30.0f);
                ElementState& state = rowState(HIT_MENU_ITEM, command);
                drawPill(g, btn, 15.0f, state.hover > 0.01f, state.press, primary, primary);
                neb::drawText(g, action, btn, 11.5f, primary, primary ? Color(255, 255, 255) : t.textSecondary);
                addHit(btn, HIT_MENU_ITEM, command);
                y += 60.0f;
            };
            feedbackRow(L"Write feedback", L"Saved on this PC in feedback.txt", L"Write", CMD_FEEDBACK_WRITE, true);
            feedbackRow(L"Open feedback file", L"Read everything you have written", L"Open", CMD_FEEDBACK_OPEN,
                        false);
            feedbackRow(L"Copy diagnostics", L"App version and library stats for bug reports", L"Copy",
                        CMD_COPY_DIAG, false);
            y += 6.0f;
            RectF box(x, y, w, 62.0f);
            fillRound(g, box, 16.0f, rgba(52, 211, 153, 18));
            strokeRound(g, box, 16.0f, rgba(52, 211, 153, 70), 1.0f);
            neb::drawIcon(g, neb::Icon::Check, RectF(box.X + 16.0f, box.Y + 20.0f, 20.0f, 20.0f),
                          Color(52, 211, 153), 2.0f);
            neb::drawText(g, L"Private by default", RectF(box.X + 48.0f, box.Y + 12.0f, w - 64.0f, 20.0f), 12.0f,
                          true, t.text, StringAlignment::StringAlignmentNear);
            neb::drawText(g, L"Feedback stays on this PC unless you decide to share the file.",
                          RectF(box.X + 48.0f, box.Y + 32.0f, w - 64.0f, 20.0f), 10.5f, false, t.textSecondary,
                          StringAlignment::StringAlignmentNear);
            y += 82.0f;
        } else {
            const LegalPara* paras = kPrivacyDoc;
            size_t count = sizeof(kPrivacyDoc) / sizeof(kPrivacyDoc[0]);
            if (doc == 1) {
                paras = kTermsDoc;
                count = sizeof(kTermsDoc) / sizeof(kTermsDoc[0]);
            } else if (doc == 2) {
                paras = kCookieDoc;
                count = sizeof(kCookieDoc) / sizeof(kCookieDoc[0]);
            }
            for (size_t i = 0; i < count; ++i) {
                neb::drawText(g, paras[i].head, RectF(x, y, w, 20.0f), 12.5f, true, t.text,
                              StringAlignment::StringAlignmentNear);
                y += 24.0f;
                const auto& lines = wrapLines(g, paras[i].body, w, 11.5f);
                for (const auto& line : lines) {
                    neb::drawText(g, line, RectF(x, y, w, 18.0f), 11.5f, false, t.textSecondary,
                                  StringAlignment::StringAlignmentNear);
                    y += 18.0f;
                }
                y += 14.0f;
            }
            neb::drawText(g, L"Last updated November 2026", RectF(x, y, w, 18.0f), 10.0f, false, t.textMuted,
                          StringAlignment::StringAlignmentNear);
            y += 30.0f;
        }
    } else {
        // --- Hero ---
        RectF logoBox(x, y, 46.0f, 46.0f);
        neb::drawLogoMark(g, logoBox);
        neb::drawText(g, L"PERSONAL FILE LIBRARY", RectF(x + 58.0f, y + 2.0f, w - 58.0f, 16.0f), 9.5f, true,
                      t.accentSolid, StringAlignment::StringAlignmentNear);
        neb::drawText(g, L"NebulaDrive", RectF(x + 58.0f, y + 16.0f, 220.0f, 24.0f), 19.0f, true, t.text,
                      StringAlignment::StringAlignmentNear);
        RectF betaBadge(x + 178.0f, y + 20.0f, 46.0f, 18.0f);
        fillRound(g, betaBadge, 9.0f, rgba(255, 200, 60, 45));
        strokeRound(g, betaBadge, 9.0f, rgba(255, 200, 60, 120), 1.0f);
        neb::drawText(g, L"BETA", betaBadge, 9.0f, true, Color(255, 210, 110));
        neb::drawText(g, L"Store, organize and play your files in one place, right on your PC.",
                      RectF(x + 58.0f, y + 40.0f, w - 58.0f, 18.0f), 11.0f, false, t.textSecondary,
                      StringAlignment::StringAlignmentNear);
        y += 62.0f;

        // --- Facts ---
        {
            struct Fact {
                const wchar_t* value;
                const wchar_t* label;
            };
            Fact facts[] = {
                {L"Local", L"STORAGE"}, {L"30", L"THEMES"}, {L"8", L"LANGUAGES"}, {L"0", L"CLOUD SERVERS"},
            };
            float cellW = (w - 24.0f) / 4.0f;
            for (int i = 0; i < 4; ++i) {
                RectF cell(x + i * (cellW + 8.0f), y, cellW, 62.0f);
                fillRound(g, cell, 14.0f, rgba(255, 255, 255, 7));
                strokeRound(g, cell, 14.0f, t.border, 1.0f);
                neb::drawText(g, facts[i].value, RectF(cell.X, cell.Y + 12.0f, cellW, 24.0f), 15.0f, true,
                              t.accentSolid);
                neb::drawText(g, facts[i].label, RectF(cell.X, cell.Y + 38.0f, cellW, 16.0f), 9.0f, true,
                              t.textMuted);
            }
            y += 76.0f;
        }

        // --- Get started ---
        sectionLabel(L"GET STARTED");
        {
            struct Step {
                const wchar_t* number;
                const wchar_t* title;
                const wchar_t* body;
            };
            Step steps[] = {
                {L"1", L"Add your files", L"Drag and drop them in, paste, or use Upload."},
                {L"2", L"Organize", L"Create nested folders and give them custom icons."},
                {L"3", L"Play or edit", L"Open audio and video, or cut a clip and download it."},
            };
            float cellW = (w - 24.0f) / 3.0f;
            for (int i = 0; i < 3; ++i) {
                RectF cell(x + i * (cellW + 12.0f), y, cellW, 118.0f);
                fillRound(g, cell, 16.0f, rgba(255, 255, 255, 7));
                strokeRound(g, cell, 16.0f, t.border, 1.0f);

                RectF num(x + i * (cellW + 12.0f) + 16.0f, y + 14.0f, 26.0f, 26.0f);
                neb::fillRoundAccent(g, num, 13.0f);
                neb::drawText(g, steps[i].number, num, 12.0f, true, Color(255, 255, 255));

                neb::drawText(g, steps[i].title, RectF(cell.X + 16.0f, y + 46.0f, cellW - 32.0f, 20.0f), 12.5f,
                              true, t.text, StringAlignment::StringAlignmentNear);
                neb::drawText(g, steps[i].body, RectF(cell.X + 16.0f, y + 66.0f, cellW - 32.0f, 40.0f), 10.5f,
                              false, t.textSecondary, StringAlignment::StringAlignmentNear);
            }
            y += 132.0f;
        }

        // --- Built-in tools ---
        sectionLabel(L"BUILT-IN TOOLS");
        {
            struct Tool {
                const wchar_t* title;
                const wchar_t* body;
                neb::Icon icon;
            };
            Tool tools[] = {
                {L"Files", L"Browse, search, rename and move. Select several items at once.", neb::Icon::Folder},
                {L"Media player", L"Play audio and video with speed, volume, repeat and a mini-player.", neb::Icon::Music},
                {L"Cinema & fullscreen", L"Watch videos in a focused view with transport controls.", neb::Icon::Video},
                {L"Cut & export", L"Trim a video or audio clip to a range you choose and download it.", neb::Icon::Archive},
                {L"Themes & motion", L"Pick from 30 palettes, tune the glass effect, or reduce motion.", neb::Icon::Star},
                {L"Backups & recovery", L"Restore from Trash, review History, export ZIP backups.", neb::Icon::Check},
            };
            float cellW = (w - 12.0f) / 2.0f;
            for (int i = 0; i < 6; ++i) {
                RectF cell(x + (i % 2) * (cellW + 12.0f), y + (i / 2) * 96.0f, cellW, 86.0f);
                fillRound(g, cell, 16.0f, rgba(255, 255, 255, 7));
                strokeRound(g, cell, 16.0f, t.border, 1.0f);

                RectF iconBox(cell.X + 16.0f, cell.Y + 16.0f, 32.0f, 32.0f);
                fillRound(g, iconBox, 11.0f, rgba(t.accentA.GetR(), t.accentA.GetG(), t.accentA.GetB(), 34));
                neb::drawIcon(g, tools[i].icon, RectF(iconBox.X + 8.0f, iconBox.Y + 8.0f, 16.0f, 16.0f),
                              t.accentSolid, 1.9f);
                neb::drawText(g, tools[i].title, RectF(cell.X + 58.0f, cell.Y + 16.0f, cellW - 74.0f, 20.0f),
                              12.5f, true, t.text, StringAlignment::StringAlignmentNear);
                neb::drawText(g, tools[i].body, RectF(cell.X + 58.0f, cell.Y + 36.0f, cellW - 74.0f, 40.0f),
                              10.5f, false, t.textSecondary, StringAlignment::StringAlignmentNear);
            }
            y += 3 * 96.0f + 10.0f;
        }

        // --- Keyboard shortcuts ---
        sectionLabel(L"KEYBOARD SHORTCUTS");
        {
            struct Shortcut {
                const wchar_t* keys;
                const wchar_t* action;
            };
            Shortcut shortcuts[] = {
                {L"Ctrl + F", L"Search"},          {L"F5", L"Refresh"},
                {L"Left / Right", L"Back / forward"}, {L"Ctrl + N", L"New folder"},
                {L"Ctrl + I", L"Import files"},    {L"Delete", L"Move to Trash"},
                {L"Esc", L"Close dialogs"},       {L"I", L"Open this panel"},
            };
            float cellW = (w - 12.0f) / 2.0f;
            for (int i = 0; i < 6; ++i) {
                RectF cell(x + (i % 2) * (cellW + 12.0f), y + (i / 2) * 34.0f, cellW, 28.0f);
                fillRound(g, cell, 9.0f, rgba(255, 255, 255, 6));
                strokeRound(g, cell, 9.0f, t.border, 1.0f);
                neb::drawText(g, shortcuts[i].keys, RectF(cell.X + 12.0f, cell.Y, 150.0f, cell.Height), 10.0f,
                              true, t.text, StringAlignment::StringAlignmentNear);
                neb::drawText(g, shortcuts[i].action, RectF(cell.X + 150.0f, cell.Y, cellW - 162.0f, cell.Height),
                              10.0f, false, t.textSecondary, StringAlignment::StringAlignmentFar);
            }
            y += 3 * 34.0f + 14.0f;
        }

        // --- Privacy ---
        {
            RectF box(x, y, w, 62.0f);
            fillRound(g, box, 16.0f, rgba(52, 211, 153, 18));
            strokeRound(g, box, 16.0f, rgba(52, 211, 153, 70), 1.0f);
            neb::drawIcon(g, neb::Icon::Check, RectF(box.X + 16.0f, box.Y + 20.0f, 20.0f, 20.0f),
                          Color(52, 211, 153), 2.0f);
            neb::drawText(g, L"Your files stay on this device", RectF(box.X + 48.0f, box.Y + 12.0f, w - 64.0f, 20.0f),
                          12.0f, true, t.text, StringAlignment::StringAlignmentNear);
            neb::drawText(g, L"NebulaDrive keeps everything in a local folder and never uploads it anywhere.",
                          RectF(box.X + 48.0f, box.Y + 32.0f, w - 64.0f, 20.0f), 10.5f, false, t.textSecondary,
                          StringAlignment::StringAlignmentNear);
            y += 76.0f;
        }

        // --- Actions ---
        {
            RectF tour(x, y, w / 2.0f - 6.0f, 46.0f);
            fillRound(g, tour, 14.0f, rgba(255, 255, 255, 8));
            strokeRound(g, tour, 14.0f, t.border, 1.0f);
            neb::drawIcon(g, neb::Icon::Star, RectF(tour.X + 16.0f, tour.Y + 15.0f, 16.0f, 16.0f), t.accentSolid,
                          1.9f);
            neb::drawText(g, L"Take the Tour", RectF(tour.X + 42.0f, tour.Y + 8.0f, tour.Width - 60.0f, 18.0f),
                          12.0f, true, t.text, StringAlignment::StringAlignmentNear);
            neb::drawText(g, L"Walk through the workspace", RectF(tour.X + 42.0f, tour.Y + 26.0f, tour.Width - 60.0f, 16.0f),
                          9.5f, false, t.textMuted, StringAlignment::StringAlignmentNear);

            RectF settings(x + w / 2.0f + 6.0f, y, w / 2.0f - 6.0f, 46.0f);
            fillRound(g, settings, 14.0f, rgba(255, 255, 255, 8));
            strokeRound(g, settings, 14.0f, t.border, 1.0f);
            neb::drawIcon(g, neb::Icon::Gear, RectF(settings.X + 16.0f, settings.Y + 15.0f, 16.0f, 16.0f),
                          t.accentSolid, 1.9f);
            neb::drawText(g, L"Open settings", RectF(settings.X + 42.0f, settings.Y + 8.0f, settings.Width - 60.0f, 18.0f),
                          12.0f, true, t.text, StringAlignment::StringAlignmentNear);
            neb::drawText(g, L"Storage, themes and accounts", RectF(settings.X + 42.0f, settings.Y + 26.0f, settings.Width - 60.0f, 16.0f),
                          9.5f, false, t.textMuted, StringAlignment::StringAlignmentNear);
            addHit(settings, HIT_SETTINGS);
            y += 60.0f;
        }

        neb::drawText(g, L"NebulaDrive BETA \u00B7 Press I anytime to open this panel",
                      RectF(x, y, w, 18.0f), 10.0f, false, t.textMuted, StringAlignment::StringAlignmentCenter);
        y += 30.0f;
    }

    g.Restore(viewportState);

    // Remember how tall the content is so the scroll can be clamped.
    g_app.modalContentHeight = (y - contentStart);
    float maxScroll = std::max(0.0f, g_app.modalContentHeight - viewport.Height);
    if (g_app.modalScroll > maxScroll) {
        g_app.modalScroll = maxScroll;
    }
}

// The profile menu that drops down from the account chip.
void drawProfileMenu(Graphics& g, const RectF& anchor) {
    if (!g_app.profileOpen) return;
    const neb::Theme& t = neb::currentTheme();

    static const wchar_t* statuses[] = {L"Online", L"Organizing", L"Uploading", L"Backing up", L"Away",
                                        L"Offline"};
    static const Color statusColors[] = {Color(76, 175, 80), Color(33, 150, 243), Color(255, 152, 0),
                                         Color(156, 39, 176), Color(255, 193, 7), Color(158, 158, 158)};

    const float cardW = 320.0f;
    float cardH = 430.0f;

    RectF card(anchor.X, anchor.Y, cardW, cardH);
    fillRoundGradientV(g, card, 20.0f, rgba(30, 27, 58, 252), rgba(20, 18, 42, 252));
    strokeRound(g, card, 20.0f, t.borderStrong, 1.0f);

    float x = card.X + 18.0f;
    float y = card.Y + 16.0f;
    float w = cardW - 36.0f;

    // Avatar and name.
    RectF avatar(x + w / 2.0f - 28.0f, y, 56.0f, 56.0f);
    LinearGradientBrush avatarBrush(PointF(avatar.X, avatar.Y), PointF(avatar.X + 56.0f, avatar.Y + 56.0f),
                                    t.accentA, t.accentB);
    GraphicsPath avatarPath; neb::addRoundedRect(avatarPath, avatar.X, avatar.Y, 56.0f, 56.0f, 28.0f);
    g.FillPath(&avatarBrush, &avatarPath);
    neb::drawIcon(g, neb::Icon::User, RectF(avatar.X + 17.0f, avatar.Y + 17.0f, 22.0f, 22.0f), Color(255, 255, 255),
                  1.9f);

    neb::drawText(g, L"Makerxm800", RectF(x, y + 64.0f, w, 22.0f), 15.0f, true, t.text,
                  StringAlignment::StringAlignmentCenter);
    neb::drawText(g, L"@Makerxm800 \u00B7 Local account", RectF(x, y + 86.0f, w, 18.0f), 10.5f, false,
                  t.textMuted, StringAlignment::StringAlignmentCenter);

    // Current status.
    RectF statusLine(x + w / 2.0f - 60.0f, y + 108.0f, 120.0f, 20.0f);
    SolidBrush dot(statusColors[g_app.statusIndex]);
    g.FillEllipse(&dot, statusLine.X, statusLine.Y + 7.0f, 7.0f, 7.0f);
    neb::drawText(g, statuses[g_app.statusIndex], RectF(statusLine.X + 12.0f, statusLine.Y, 108.0f, 20.0f),
                  10.5f, false, t.textSecondary, StringAlignment::StringAlignmentNear);
    y += 136.0f;

    // The status grid.
    neb::drawText(g, L"STATUS \u2014 WHAT ARE YOU UP TO?", RectF(x, y, w, 16.0f), 9.0f, true, t.textMuted,
                  StringAlignment::StringAlignmentCenter);
    y += 22.0f;

    float pillW = (w - 8.0f) / 3.0f;
    for (int i = 0; i < 6; ++i) {
        RectF pill(x + (i % 3) * (pillW + 8.0f), y + (i / 3) * 30.0f, pillW, 26.0f);
        bool active = i == g_app.statusIndex;
        fillRound(g, pill, 13.0f, rgba(255, 255, 255, active ? 12 : 6));
        if (active) {
            strokeRound(g, pill, 13.0f, statusColors[i], 1.0f);
        }
        SolidBrush pip(statusColors[i]);
        g.FillEllipse(&pip, pill.X + 9.0f, pill.Y + 10.0f, 6.0f, 6.0f);
        neb::drawText(g, statuses[i], RectF(pill.X + 20.0f, pill.Y, pillW - 26.0f, pill.Height), 9.5f, active,
                      t.textSecondary, StringAlignment::StringAlignmentNear);
        addHit(pill, HIT_MENU_ITEM, CMD_NONE);
    }
    y += 70.0f;

    // Custom status field.
    RectF custom(x, y, w - 44.0f, 32.0f);
    fillRound(g, custom, 16.0f, rgba(255, 255, 255, 8));
    strokeRound(g, custom, 16.0f, t.border, 1.0f);
    neb::drawText(g, L"Custom status... (e.g. Sorting photos)", RectF(custom.X + 12.0f, custom.Y, custom.Width - 20.0f, custom.Height),
                  10.5f, false, t.textMuted, StringAlignment::StringAlignmentNear);

    RectF setBtn(x + w - 40.0f, y, 40.0f, 32.0f);
    neb::fillRoundAccent(g, setBtn, 16.0f);
    neb::drawText(g, L"Set", setBtn, 11.0f, true, Color(255, 255, 255));
    y += 42.0f;

    // Actions.
    struct ActionRow {
        const wchar_t* label;
        neb::Icon icon;
    };
    ActionRow actions[] = {
        {L"Switch Account", neb::Icon::Refresh},
        {L"Profile themes", neb::Icon::Star},
    };
    for (const ActionRow& action : actions) {
        RectF row(x, y, w, 38.0f);
        fillRound(g, row, 12.0f, rgba(255, 255, 255, 6));
        strokeRound(g, row, 12.0f, t.border, 1.0f);
        neb::drawIcon(g, action.icon, RectF(row.X + 12.0f, row.Y + 11.0f, 16.0f, 16.0f), t.textSecondary, 1.9f);
        neb::drawText(g, action.label, RectF(row.X + 36.0f, row.Y, w - 60.0f, row.Height), 11.5f, true, t.text,
                      StringAlignment::StringAlignmentNear);
        addHit(row, HIT_MENU_ITEM, CMD_NONE);
        y += 44.0f;
    }
}

// A dropdown menu anchored under a toolbar button.
struct MenuRow {
    int command = 0;
    const wchar_t* label = nullptr;
    const wchar_t* glyph = nullptr;   // the letter chip shown on the left
    Color chip;
    bool checked = false;
    bool separatorBefore = false;
    bool enabled = true;
};

// Draws a floating dropdown panel and registers its rows for hit testing.
void drawDropdown(Graphics& g, const RectF& anchor, const std::vector<MenuRow>& rows, float rowH = 34.0f) {
    const neb::Theme& t = neb::currentTheme();

    float width = 224.0f;
    float height = 10.0f + rowH * static_cast<float>(rows.size());

    RectF panel(anchor.X, anchor.Y, width, height);
    fillRound(g, panel, 14.0f, rgba(22, 20, 46, 248));
    strokeRound(g, panel, 14.0f, t.borderStrong, 1.0f);

    float y = panel.Y + 5.0f;
    for (const MenuRow& row : rows) {
        if (row.separatorBefore) {
            strokeRound(g, RectF(panel.X + 12.0f, y + 2.0f, width - 24.0f, 1.0f), 0.0f, t.border, 1.0f);
            y += 8.0f;
        }

        RectF item(panel.X + 6.0f, y, width - 12.0f, rowH - 2.0f);
        ElementState& state = rowState(HIT_MENU_ITEM, row.command);
        if (state.hover > 0.01f && row.enabled) {
            fillRound(g, item, 10.0f, rgba(255, 255, 255, 12));
        }

        float textX = item.X + 10.0f;
        if (row.glyph) {
            // The coloured letter chip used for the sort entries.
            RectF chip(item.X + 8.0f, item.Y + (rowH - 22.0f) / 2.0f, 22.0f, 22.0f);
            fillRound(g, chip, 7.0f, row.chip);
            neb::drawText(g, row.glyph, chip, 10.0f, true, Color(255, 255, 255));
            textX = chip.X + chip.Width + 10.0f;
        }

        Color fg = row.enabled ? t.text : t.textMuted;
        neb::drawText(g, row.label, RectF(textX, item.Y, item.X + item.Width - textX - 24.0f, item.Height),
                      12.0f, row.checked, fg, StringAlignment::StringAlignmentNear);
        if (row.checked) {
            neb::drawText(g, L"\u2713", RectF(item.X + item.Width - 24.0f, item.Y, 18.0f, item.Height), 12.0f,
                          true, t.accentSolid, StringAlignment::StringAlignmentCenter);
        }

        addHit(item, HIT_MENU_ITEM, row.command);
        y += rowH;
    }
}

// The language picker.
void drawLanguageModal(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();
    float anim = clampf(g_app.modalAnim, 0.0f, 1.0f);

    fillRound(g, area, 0.0f, rgba(0, 0, 0, (BYTE)(150 * anim)));

    float cardW = std::min(560.0f, area.Width - 60.0f);
    float cardH = std::min(520.0f, area.Height - 60.0f);
    float ease = 1.0f - std::pow(1.0f - anim, 3.0f);
    float scale = 0.96f + 0.04f * ease;

    RectF card(area.X + (area.Width - cardW * scale) / 2.0f,
               area.Y + (area.Height - cardH * scale) / 2.0f + (1.0f - ease) * 16.0f, cardW * scale,
               cardH * scale);
    g_hitFloor = g_hits.size();
    addHit(card, HIT_NONE);

    fillRoundGradientV(g, card, 24.0f, rgba(30, 27, 58, (BYTE)(250 * anim)),
                        rgba(20, 18, 42, (BYTE)(250 * anim)));
    strokeRound(g, card, 24.0f, rgba(t.borderStrong.GetR(), t.borderStrong.GetG(), t.borderStrong.GetB(),
                                     (BYTE)(210 * anim)),
                1.0f);

    // Header.
    RectF iconBox(card.X + 24.0f, card.Y + 20.0f, 34.0f, 34.0f);
    fillRound(g, iconBox, 11.0f, rgba(t.accentA.GetR(), t.accentA.GetG(), t.accentA.GetB(), 40));
    neb::drawIcon(g, neb::Icon::Globe, RectF(iconBox.X + 8.0f, iconBox.Y + 8.0f, 18.0f, 18.0f),
                  t.accentSolid, 2.0f);
    neb::drawText(g, L"Language", RectF(iconBox.X + 46.0f, card.Y + 20.0f, 200.0f, 34.0f), 16.0f, true,
                  t.text, StringAlignment::StringAlignmentNear);

    RectF closeBtn(card.X + card.Width - 52.0f, card.Y + 20.0f, 30.0f, 30.0f);
    ElementState& closeState = element(HIT_MODAL_CLOSE);
    if (closeState.hover > 0.01f) {
        fillRound(g, closeBtn, 15.0f, rgba(239, 68, 68, (BYTE)(120 * closeState.hover)));
    }
    strokeRound(g, closeBtn, 15.0f, t.border, 1.0f);
    neb::drawText(g, L"\u2715", closeBtn, 13.0f, false, t.textSecondary);
    addHit(closeBtn, HIT_MODAL_CLOSE);

    // Language rows.
    float rowH = 40.0f;
    float y = card.Y + 66.0f;
    float x = card.X + 18.0f;
    float w = card.Width - 36.0f;

    for (size_t i = 0; i < sizeof(kLanguages) / sizeof(kLanguages[0]); ++i) {
        const LanguageDef& language = kLanguages[i];
        bool selected = g_app.language == language.code;

        RectF row(x, y, w, rowH);
        if (selected) {
            fillRound(g, row, 11.0f, rgba(t.accentA.GetR(), t.accentA.GetG(), t.accentA.GetB(), 46));
            strokeRound(g, row, 11.0f, t.accentSolid, 1.5f);
        } else {
            ElementState& state = rowState(HIT_LANGUAGE_ITEM, static_cast<int>(CMD_LANG_EN) + static_cast<int>(i));
            if (state.hover > 0.01f) {
                fillRound(g, row, 11.0f, rgba(255, 255, 255, 10));
            }
        }

        RectF flagRect(row.X + 12.0f, row.Y + (row.Height - 16.0f) / 2.0f, 22.0f, 15.0f);
        neb::drawFlag(g, language.code, flagRect);
        strokeRound(g, flagRect, 3.0f, rgba(255, 255, 255, 40), 1.0f);
        neb::drawText(g, language.label, RectF(row.X + 48.0f, row.Y, w - 90.0f, row.Height), 13.0f, selected,
                      selected ? t.text : t.textSecondary, StringAlignment::StringAlignmentNear);
        if (selected) {
            neb::drawText(g, L"\u2713", RectF(row.X + w - 30.0f, row.Y, 20.0f, row.Height), 13.0f, true,
                          t.accentSolid, StringAlignment::StringAlignmentCenter);
        }

        addHit(row, HIT_LANGUAGE_ITEM, static_cast<int>(CMD_LANG_EN) + static_cast<int>(i));
        y += rowH + 2.0f;
    }
}

// The overflow menu that opens from a card's "More" button.
void drawCardMenu(Graphics& g) {
    if (!g_app.cardMenuOpen) return;
    const neb::Theme& t = neb::currentTheme();

    struct MenuItem {
        int command;
        const wchar_t* label;
        const wchar_t* desc;
        Color tone;
        neb::Icon icon;
    };

    std::vector<MenuItem> items;
    if (g_app.cardMenuInTrash) {
        items.push_back({CMD_RESTORE, L"Restore", L"Move back into the library", Color(129, 140, 248),
                         neb::Icon::Refresh});
        items.push_back({CMD_DELETE_FOREVER, L"Delete Forever", L"Erase this file for good", Color(248, 113, 113),
                         neb::Icon::Trash});
    } else if (g_app.cardMenuIsFolder) {
        items.push_back({CMD_OPEN_FILE, L"Open", L"Browse this folder", Color(129, 140, 248), neb::Icon::Folder});
        items.push_back({CMD_RENAME, L"Rename", L"Change the folder name", Color(56, 189, 248), neb::Icon::Document});
        items.push_back({CMD_MOVE, L"Move", L"Move to another folder", Color(245, 158, 11), neb::Icon::Folder});
        items.push_back({CMD_DELETE, L"Delete", L"Move folder to Trash", Color(248, 113, 113), neb::Icon::Trash});
    } else {
        items.push_back({CMD_CUT, L"Cut Audio", L"Trim start and end", Color(167, 139, 250), neb::Icon::Music});
        items.push_back({CMD_RENAME, L"Rename", L"Change file name", Color(56, 189, 248), neb::Icon::Document});
        items.push_back({CMD_MOVE, L"Move", L"Move to another folder", Color(245, 158, 11), neb::Icon::Folder});
        items.push_back({CMD_DELETE, L"Delete", L"Move file to Trash", Color(248, 113, 113), neb::Icon::Trash});
    }

    const float rowH = 46.0f;
    const float headH = 46.0f;
    float w = 246.0f;
    float h = headH + rowH * static_cast<float>(items.size()) + 8.0f;

    RectF panel(g_app.cardMenuRect.X, g_app.cardMenuRect.Y, w, h);
    fillRound(g, panel, 16.0f, rgba(26, 24, 50, 250));
    strokeRound(g, panel, 16.0f, t.borderStrong, 1.0f);

    // Header: "FILE OPTIONS" plus the item name.
    neb::drawText(g, g_app.cardMenuIsFolder ? L"FOLDER OPTIONS" : L"FILE OPTIONS",
                  RectF(panel.X + 14.0f, panel.Y + 9.0f, w - 28.0f, 15.0f), 9.5f, true, t.textMuted,
                  StringAlignment::StringAlignmentNear);
    std::string label = g_app.cardMenuName;
    neb::drawText(g, neb::widen(label), RectF(panel.X + 14.0f, panel.Y + 24.0f, w - 28.0f, 18.0f), 11.5f, true,
                  t.text, StringAlignment::StringAlignmentNear);
    strokeRound(g, RectF(panel.X + 10.0f, panel.Y + headH - 6.0f, w - 20.0f, 1.0f), 0.0f, t.border, 1.0f);

    float y = panel.Y + headH - 2.0f;
    for (size_t i = 0; i < items.size(); ++i) {
        RectF row(panel.X + 8.0f, y, w - 16.0f, rowH - 2.0f);
        ElementState& state = element(HIT_CARD_MORE);
        if (state.hover > 0.01f) {
            fillRound(g, row, 11.0f, rgba(255, 255, 255, 8));
        }

        // The coloured icon chip on the left.
        RectF chip(row.X + 8.0f, row.Y + 9.0f, 28.0f, 28.0f);
        fillRound(g, chip, 9.0f, rgba(items[i].tone.GetR(), items[i].tone.GetG(), items[i].tone.GetB(), 46));
        strokeRound(g, chip, 9.0f, rgba(items[i].tone.GetR(), items[i].tone.GetG(), items[i].tone.GetB(), 110), 1.0f);
        neb::drawIcon(g, items[i].icon, RectF(chip.X + 7.0f, chip.Y + 7.0f, 14.0f, 14.0f), items[i].tone, 2.0f);

        neb::drawText(g, items[i].label, RectF(chip.X + chip.Width + 10.0f, row.Y + 7.0f, w - 76.0f, 18.0f),
                      12.0f, true, t.text, StringAlignment::StringAlignmentNear);
        neb::drawText(g, items[i].desc, RectF(chip.X + chip.Width + 10.0f, row.Y + 24.0f, w - 76.0f, 16.0f),
                      9.5f, false, t.textMuted, StringAlignment::StringAlignmentNear);
        neb::drawText(g, L"\u203A", RectF(row.X + row.Width - 22.0f, row.Y, 16.0f, row.Height), 13.0f, false,
                      t.textMuted, StringAlignment::StringAlignmentCenter);

        addHit(row, HIT_CARD_MORE, g_app.cardMenuId, 1000 + items[i].command, g_app.cardMenuIsFolder);
        y += rowH;
    }
}

// The in-app text prompt used for rename / new folder.
void drawPrompt(Graphics& g, const RectF& area) {
    if (!g_app.prompting) return;
    const neb::Theme& t = neb::currentTheme();

    g_hitFloor = g_hits.size();
    fillRound(g, area, 0.0f, rgba(0, 0, 0, 160));

    const float cardW = 420.0f;
    const float cardH = 210.0f;
    RectF card(area.X + (area.Width - cardW) / 2.0f, area.Y + (area.Height - cardH) / 2.0f, cardW, cardH);

    fillRoundGradientV(g, card, 22.0f, rgba(30, 27, 58, 252), rgba(20, 18, 42, 252));
    strokeRound(g, card, 22.0f, t.borderStrong, 1.0f);

    // The icon tile above the title.
    RectF iconBox(card.X + cardW / 2.0f - 24.0f, card.Y + 22.0f, 48.0f, 48.0f);
    fillRound(g, iconBox, 15.0f, rgba(245, 158, 11, 40));
    strokeRound(g, iconBox, 15.0f, rgba(245, 158, 11, 120), 1.0f);
    neb::drawIcon(g, g_app.promptIsFolder ? neb::Icon::FolderPlus : neb::Icon::Document,
                  RectF(iconBox.X + 12.0f, iconBox.Y + 12.0f, 24.0f, 24.0f), Color(245, 158, 11), 2.0f);

    neb::drawText(g, g_app.promptTitle, RectF(card.X + 24.0f, card.Y + 80.0f, cardW - 48.0f, 24.0f), 16.0f, true,
                  t.text, StringAlignment::StringAlignmentCenter);
    neb::drawText(g, g_app.promptTargetId == -100 ? L"Tell us what you think:" : g_app.promptIsFolder ? L"Give your folder name:" : L"Give your file a new name:",
                  RectF(card.X + 24.0f, card.Y + 104.0f, cardW - 48.0f, 18.0f), 11.0f, false, t.textSecondary,
                  StringAlignment::StringAlignmentCenter);

    // The text field is a real edit control placed over this slot.
    RectF field(card.X + 24.0f, card.Y + 128.0f, cardW - 48.0f, 38.0f);
    fillRound(g, field, 12.0f, rgba(255, 255, 255, 12));
    strokeRound(g, field, 12.0f, t.accentSolid, 1.5f);

    // Buttons.
    RectF cancelBtn(card.X + 24.0f, card.Y + 174.0f, 168.0f, 30.0f);
    drawPill(g, cancelBtn, 12.0f, element(HIT_MODAL_CLOSE).hover > 0.01f, 0.0f, false, false);
    neb::drawText(g, L"Cancel", cancelBtn, 12.0f, false, t.textSecondary);
    addHit(cancelBtn, HIT_NEW_FOLDER_CANCEL);

    RectF okBtn(card.X + 200.0f, card.Y + 174.0f, 196.0f, 30.0f);
    drawPill(g, okBtn, 12.0f, element(HIT_NEW_FOLDER_OK).hover > 0.01f, 0.0f, true, true);
    neb::drawText(g, g_app.promptTargetId == -100 ? L"Save feedback" : g_app.promptIsFolder ? L"Create" : L"Save", okBtn, 12.0f, true, Color(255, 255, 255));
    addHit(okBtn, HIT_NEW_FOLDER_OK);
}

// The full window paint.

// ------------------------------------------------------------------
// Tooltips
// ------------------------------------------------------------------
bool g_tipVisible = false;
std::wstring g_tipText;
int g_tipKey = -1;
float g_mouseLX = 0.0f;
float g_mouseLY = 0.0f;
float g_tipX = 0.0f;
float g_tipY = 0.0f;

std::wstring tooltipFor(const HitRegion& h) {
    switch (h.id) {
        case HIT_TAB_FILES: return L"Browse your files";
        case HIT_TAB_FAVORITES: return L"Starred files and folders";
        case HIT_TAB_HISTORY: return L"Recent activity";
        case HIT_TAB_TRASH: return L"Deleted items - restore or empty";
        case HIT_LANG: return L"Change language";
        case HIT_SETTINGS: return L"Settings and themes";
        case HIT_INFO: return L"About NebulaDrive";
        case HIT_USER_CHIP: return L"Account and profile";
        case HIT_BACK: return L"Back";
        case HIT_FORWARD: return L"Forward";
        case HIT_REFRESH: return L"Refresh";
        case HIT_NEW_FOLDER: return L"Create a new folder";
        case HIT_SORT: return L"Sort your files";
        case HIT_VIEW: return L"Change how files are shown";
        case HIT_FAV_FILTER: return L"Show only favorites";
        case HIT_SEARCH: return L"Search your files";
        case HIT_DROPZONE: return L"Add files to this folder";
        case HIT_SELECT_TOGGLE: return L"Select several items at once";
        case HIT_CARD_STAR: return L"Add or remove favorite";
        case HIT_CARD_MORE: return L"More actions";
        case HIT_FOOTER_PRIVACY: return L"How NebulaDrive handles your data";
        case HIT_FOOTER_TERMS: return L"Terms of using NebulaDrive";
        case HIT_FOOTER_COOKIE: return L"Cookie information";
        case HIT_FOOTER_FEEDBACK: return L"Tell us what you think";
        case HIT_BOTTOM_OPEN: return L"Open the selected file";
        case HIT_BOTTOM_CLEAR: return L"Clear the selection";
        case HIT_TOAST_CLOSE: return L"Dismiss";
        case HIT_CARD:
            if (h.arg > 0) {
                const auto& list = h.isFolder ? g_app.folders : g_app.files;
                for (const auto& item : list) {
                    if (item.id == h.arg) return neb::widen(item.name);
                }
            }
            return L"";
        default: return L"";
    }
}

void drawTooltip(Graphics& g, const RectF& area) {
    if (!g_tipVisible || g_tipText.empty()) return;
    const neb::Theme& t = neb::currentTheme();
    SizeF size = neb::measureText(g, g_tipText, 11.0f, false);
    float w = std::min(size.Width + 24.0f, 420.0f);
    float h = 28.0f;
    float tx = g_tipX + 14.0f;
    float ty = g_tipY + 24.0f;
    if (tx + w > area.Width - 8.0f) tx = area.Width - 8.0f - w;
    if (tx < 8.0f) tx = 8.0f;
    if (ty + h > area.Height - 8.0f) ty = g_tipY - h - 10.0f;
    RectF r(tx, ty, w, h);
    fillRound(g, RectF(r.X, r.Y + 2.0f, r.Width, r.Height), 10.0f, rgba(0, 0, 0, 60));
    fillRound(g, r, 10.0f, rgba(18, 16, 40, 246));
    strokeRound(g, r, 10.0f, t.borderStrong, 1.0f);
    neb::drawText(g, g_tipText, r, 11.0f, false, t.text);
}

void paintWindow(Graphics& g, const RectF& area) {
    const neb::Theme& t = neb::currentTheme();

    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.SetInterpolationMode(InterpolationModeBilinear);
    g.SetCompositingQuality(CompositingQualityHighSpeed);

    neb::drawBackground(g, area, t);

    g_hits.clear();
    g_hitFloor = 0;

    const Metrics& m = metrics();
    float y = m.navHeight;

    drawNavbar(g, area);
    drawNavTabs(g, area);

    // Tagline.
    float taglineY = y + 14.0f;
    drawTagline(g, RectF(m.padX, taglineY, area.Width - m.padX * 2.0f, m.taglineHeight));
    y = taglineY + m.taglineHeight;

    // Breadcrumb bar.
    float crumbY = y + 14.0f;
    drawBreadcrumbBar(g, RectF(m.padX, crumbY, area.Width - m.padX * 2.0f, m.crumbHeight));
    y = crumbY + m.crumbHeight;

    // Toolbar.
    float toolbarY = y + 12.0f;
    drawToolbar(g, RectF(m.padX, toolbarY, area.Width - m.padX * 2.0f, m.toolbarHeight));
    y = toolbarY + m.toolbarHeight;

    // Drop zone.
    float dropY = y + 12.0f;
    drawDropZone(g, RectF(m.padX, dropY, area.Width - m.padX * 2.0f, m.dropHeight));
    y = dropY + m.dropHeight;

    // Select row. It grows to fit the bulk action bar.
    float selectH = g_app.selectionMode ? 80.0f : m.selectHeight;
    float selectY = y + 12.0f;
    drawSelectRow(g, RectF(m.padX, selectY, area.Width - m.padX * 2.0f, selectH));
    y = selectY + selectH;

    // Content area.
    float footerY = area.Height - m.footerHeight - m.bottomHeight - 20.0f;
    float gridY = y + 12.0f;
    float gridH = footerY - gridY - 12.0f;
    if (gridH < 60.0f) gridH = 60.0f;

    RectF gridArea(m.padX, gridY, area.Width - m.padX * 2.0f, gridH);

    GraphicsState clipState = g.Save();
    g.SetClip(gridArea);

    // Build the combined item list: folders first, then files.
    std::vector<FileRecord> items;
    if (g_app.tab != Tab::History) {
        items = g_app.folders;
        items.insert(items.end(), g_app.files.begin(), g_app.files.end());
    }

    if (g_app.tab == Tab::History && g_app.history.empty()) {
        neb::drawIcon(g, neb::Icon::Clock, RectF(gridArea.X + gridArea.Width / 2.0f - 24.0f, gridArea.Y + 40.0f,
                                                 48.0f, 48.0f),
                      rgba(t.textMuted.GetR(), t.textMuted.GetG(), t.textMuted.GetB(), 110), 1.6f);
        neb::drawText(g, L"No activity yet", RectF(gridArea.X, gridArea.Y + 100.0f, gridArea.Width, 24.0f), 14.0f,
                      true, t.textSecondary);
        neb::drawText(g, L"Your actions will show up here.", RectF(gridArea.X, gridArea.Y + 126.0f, gridArea.Width, 20.0f),
                      11.5f, false, t.textMuted);
    } else if (items.empty()) {
        neb::drawIcon(g, neb::Icon::Folder, RectF(gridArea.X + gridArea.Width / 2.0f - 24.0f, gridArea.Y + 40.0f,
                                                 48.0f, 48.0f),
                      rgba(t.textMuted.GetR(), t.textMuted.GetG(), t.textMuted.GetB(), 110), 1.6f);
        const wchar_t* emptyTitle = g_app.tab == Tab::Trash      ? L"Trash is empty"
                                    : g_app.tab == Tab::Favorites ? L"No favorites yet"
                                    : g_app.tab == Tab::History   ? L"No activity yet"
                                                                : L"This folder is empty";
        const wchar_t* emptyHint = g_app.tab == Tab::Trash      ? L"Deleted files will appear here."
                                  : g_app.tab == Tab::Favorites ? L"Star a file to keep it close."
                                  : g_app.tab == Tab::History   ? L"Your actions will show up here."
                                                              : L"Drop files here or create a folder.";
        neb::drawText(g, emptyTitle, RectF(gridArea.X, gridArea.Y + 100.0f, gridArea.Width, 24.0f), 14.0f, true,
                      t.textSecondary);
        neb::drawText(g, emptyHint, RectF(gridArea.X, gridArea.Y + 126.0f, gridArea.Width, 20.0f), 11.5f, false,
                      t.textMuted);
    } else if (g_app.tab == Tab::History) {
        // History is a simple list of actions.
        float rowY = gridArea.Y - g_app.scrollOffset;
        for (size_t i = 0; i < g_app.history.size(); ++i) {
            const HistoryRecord& entry = g_app.history[i];
            RectF row(gridArea.X, rowY, gridArea.Width, 46.0f);
            if (i % 2 == 0) {
                fillRound(g, row, 12.0f, rgba(255, 255, 255, 5));
            }
            neb::drawIcon(g, neb::Icon::Clock, RectF(row.X + 12.0f, row.Y + 13.0f, 18.0f, 18.0f), t.textMuted, 1.9f);
            neb::drawText(g, neb::widen(entry.action), RectF(row.X + 40.0f, row.Y, 110.0f, row.Height), 12.0f, true,
                          t.text, StringAlignment::StringAlignmentNear);
            neb::drawText(g, neb::widen(entry.details), RectF(row.X + 155.0f, row.Y, gridArea.Width * 0.42f, row.Height),
                          11.5f, false, t.textSecondary, StringAlignment::StringAlignmentNear);
            neb::drawText(g, neb::widen(entry.timestamp), RectF(row.X, row.Y, gridArea.Width - 14.0f, row.Height),
                          10.5f, false, t.textMuted, StringAlignment::StringAlignmentFar);
            rowY += 50.0f;
        }
        g_app.maxScroll = std::max(0.0f, rowY - (gridArea.Y + gridArea.Height));
    } else if (g_app.view == ViewMode::List) {
        float rowY = gridArea.Y - g_app.scrollOffset;
        for (size_t i = 0; i < items.size(); ++i) {
            bool isFolder = i < g_app.folders.size();
            const FileRecord& item = items[i];
            uint64_t size = isFolder ? (g_app.folderSizes.count(item.path) ? g_app.folderSizes[item.path] : 0)
                                     : item.size;
            bool isSelected = std::find(g_app.selected.begin(), g_app.selected.end(), item.id) !=
                              g_app.selected.end();
            drawCard(g, RectF(gridArea.X, rowY, gridArea.Width, m.rowHeight), item, isFolder,
                     static_cast<int>(i), size, isSelected);
            rowY += m.rowHeight + 4.0f;
        }
        g_app.maxScroll = std::max(0.0f, rowY - (gridArea.Y + gridArea.Height));
    } else {
        float cardW, cardH, iconSize;
        cardMetrics(g_app.view, cardW, cardH, iconSize);

        float gap = m.gap;
        int columns = std::max(1, static_cast<int>((gridArea.Width + gap) / (cardW + gap)));
        float actualW = (gridArea.Width - gap * (columns - 1)) / columns;
        float cardHeight = cardH;

        float startY = gridArea.Y - g_app.scrollOffset;
        float y2 = startY;
        for (size_t i = 0; i < items.size(); ++i) {
            bool isFolder = i < g_app.folders.size();
            const FileRecord& item = items[i];
            uint64_t size = isFolder ? (g_app.folderSizes.count(item.path) ? g_app.folderSizes[item.path] : 0)
                                     : item.size;

            int col = static_cast<int>(i) % columns;
            int row = static_cast<int>(i) / columns;
            float x2 = gridArea.X + col * (actualW + gap);
            float yy = startY + row * (cardHeight + gap);

            bool isSelected = std::find(g_app.selected.begin(), g_app.selected.end(), item.id) !=
                              g_app.selected.end();
            drawCard(g, RectF(x2, yy, actualW, cardHeight), item, isFolder, static_cast<int>(i), size, isSelected);
        }
        float rows = std::ceil(static_cast<float>(items.size()) / columns);
        g_app.maxScroll = std::max(0.0f, startY + rows * (cardHeight + gap) - (gridArea.Y + gridArea.Height));
    }

    g.Restore(clipState);

    // Footer.
    float footerTop = area.Height - m.footerHeight - m.bottomHeight - 20.0f;
    drawFooter(g, RectF(m.padX, footerTop, area.Width - m.padX * 2.0f, m.footerHeight));

    // Bottom bar.
    drawBottomBar(g, RectF(0.0f, area.Height - m.bottomHeight, area.Width, m.bottomHeight));

    // Overlays last.
    drawCardMenu(g);
    drawProfileMenu(g, RectF(area.Width - 350.0f, metrics().navHeight + 4.0f, 320.0f, 430.0f));
    drawToasts(g, area);

    if (g_app.modal == ModalKind::SortMenu) {
        // Sort dropdown, anchored under the Sort button.
        std::vector<MenuRow> rows = {
            {CMD_SORT_NAME, L"Name", L"#", rgba(255, 107, 107, 60), g_app.sortKey == SortKey::Name, false, true},
            {CMD_SORT_SIZE, L"Size", L"S", rgba(255, 165, 2, 60), g_app.sortKey == SortKey::Size, false, true},
            {CMD_SORT_DATE, L"Date", L"D", rgba(30, 144, 255, 60), g_app.sortKey == SortKey::Date, false, true},
            {CMD_SORT_TYPE, L"Type", L"T", rgba(46, 213, 115, 60), g_app.sortKey == SortKey::Type, false, true},
            {CMD_SORT_EXTENSION, L"Extension", L"E", rgba(83, 82, 237, 60), false, false, true},
            {CMD_SORT_ASC, L"Ascending", L"\u2191", rgba(255, 99, 72, 60), g_app.sortAscending, true, true},
            {CMD_SORT_DESC, L"Descending", L"\u2193", rgba(255, 99, 72, 60), !g_app.sortAscending, false, true},
        };
        drawDropdown(g, RectF(m.padX + 6.0f, toolbarY + m.toolbarHeight + 4.0f, 224.0f, 0.0f), rows);
    } else if (g_app.modal == ModalKind::ViewMenu) {
        std::vector<MenuRow> rows = {
            {CMD_VIEW_MEDIUM, L"Medium Grid", nullptr, Color(0, 0, 0, 0), g_app.view == ViewMode::GridMedium, false, true},
            {CMD_VIEW_SMALL, L"Small Grid", nullptr, Color(0, 0, 0, 0), g_app.view == ViewMode::GridSmall, false, true},
            {CMD_VIEW_LARGE, L"Large Grid", nullptr, Color(0, 0, 0, 0), g_app.view == ViewMode::GridLarge, false, true},
            {CMD_VIEW_LIST, L"List", nullptr, Color(0, 0, 0, 0), g_app.view == ViewMode::List, true, true},
        };
        drawDropdown(g, RectF(m.padX + 142.0f, toolbarY + m.toolbarHeight + 4.0f, 224.0f, 0.0f), rows);
    } else if (g_app.modal == ModalKind::Language) {
        drawLanguageModal(g, area);
    } else if (g_app.modal != ModalKind::None) {
        drawModal(g, area);
    }

    if (g_app.prompting) {
        drawPrompt(g, area);
    }

    drawTooltip(g, area);
}

} // namespace

// ============================================================
// Interaction
// ============================================================

namespace {

// Starts the animation timer if it is not already running.
void wakeAnimations() {
    if (g_hwnd) {
        SetTimer(g_hwnd, 1, 16, nullptr);
    }
}

// The height of the client area in logical pixels.
float areaHeight() {
    RECT rc;
    if (g_hwnd && GetClientRect(g_hwnd, &rc)) {
        return rc.bottom / g_scale;
    }
    return 840.0f;
}

void setPromptField(const std::wstring& value) {
    g_app.promptValue = value;
    if (g_promptEdit) {
        SetWindowTextW(g_promptEdit, value.c_str());
    }
}

// Reads the current text out of the prompt's edit control.
std::wstring readPromptValue() {
    if (g_promptEdit) {
        int length = GetWindowTextLengthW(g_promptEdit);
        if (length > 0) {
            std::wstring value(static_cast<size_t>(length) + 1, 0);
            GetWindowTextW(g_promptEdit, &value[0], length + 1);
            value.resize(static_cast<size_t>(length));
            return value;
        }
        return L"";
    }
    return g_app.promptValue;
}

void closePrompt() {
    g_app.prompting = false;
    if (g_promptEdit) {
        DestroyWindow(g_promptEdit);
        g_promptEdit = nullptr;
    }
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

// Hides the search field when the user clicks away from it.
void closeSearch() {
    if (!g_searchEdit) return;
    DestroyWindow(g_searchEdit);
    g_searchEdit = nullptr;
    g_app.searching = false;
    g_app.searchQuery.clear();
    reloadData();
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

// Creates (or focuses) the inline search field over the toolbar slot.
void openSearch() {
    if (g_searchEdit) {
        SetFocus(g_searchEdit);
        return;
    }

    RECT rc;
    GetClientRect(g_hwnd, &rc);
    float areaW = rc.right;
    float areaH = rc.bottom;

    const Metrics& m = metrics();
    float y = m.navHeight;
    float taglineY = y + 14.0f;
    y = taglineY + m.taglineHeight;
    float crumbY = y + 14.0f;
    y = crumbY + m.crumbHeight;
    float toolbarY = y + 12.0f;

    float searchW = std::min(230.0f, areaW * 0.22f);
    float fieldX = m.padX + (areaW - m.padX * 2.0f) - searchW - 12.0f;
    float fieldY = toolbarY + (m.toolbarHeight - 34.0f) / 2.0f;

    g_searchEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                                    WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                                    static_cast<int>(fieldX * g_scale),
                                    static_cast<int>(fieldY * g_scale),
                                    static_cast<int>(searchW * g_scale),
                                    static_cast<int>(34.0f * g_scale), g_hwnd,
                                    reinterpret_cast<HMENU>(1001), GetModuleHandle(nullptr), nullptr);
    if (g_searchEdit) {
        SendMessageW(g_searchEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        SetFocus(g_searchEdit);
    }
    g_app.searching = true;
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

// Keeps the search field positioned over its painted slot.
void layoutSearchEdit() {
    if (!g_searchEdit) return;

    RECT rc;
    GetClientRect(g_hwnd, &rc);
    float areaW = rc.right;
    float areaH = rc.bottom;

    const Metrics& m = metrics();
    float y = m.navHeight;
    float taglineY = y + 14.0f;
    y = taglineY + m.taglineHeight;
    float crumbY = y + 14.0f;
    y = crumbY + m.crumbHeight;
    float toolbarY = y + 12.0f;

    float searchW = std::min(230.0f, areaW * 0.22f);
    float fieldX = m.padX + (areaW - m.padX * 2.0f) - searchW - 12.0f;
    float fieldY = toolbarY + (m.toolbarHeight - 34.0f) / 2.0f;

    SetWindowPos(g_searchEdit, nullptr, static_cast<int>(fieldX * g_scale),
                 static_cast<int>(fieldY * g_scale), static_cast<int>(searchW * g_scale),
                 static_cast<int>(34.0f * g_scale), SWP_NOZORDER | SWP_NOACTIVATE);
}

void openPrompt(const std::wstring& title, const std::wstring& initial, bool isFolder, int targetId) {
    g_app.prompting = true;
    g_app.promptTitle = title;
    g_app.promptValue = initial;
    g_app.promptIsFolder = isFolder;
    g_app.promptTargetId = targetId;

    if (g_promptEdit) {
        DestroyWindow(g_promptEdit);
        g_promptEdit = nullptr;
    }

    // The field sits inside the modal card; position it in screen pixels.
    RECT rc;
    GetClientRect(g_hwnd, &rc);
    float areaW = rc.right;
    float areaH = rc.bottom;

    const float cardW = 420.0f;
    const float cardH = 210.0f;
    float cardX = (areaW - cardW) / 2.0f;
    float cardY = (areaH - cardH) / 2.0f;
    float fieldX = cardX + 24.0f;
    float fieldY = cardY + 128.0f;
    float fieldW = cardW - 48.0f;
    float fieldH = 38.0f;

    g_promptEdit = CreateWindowExW(0, L"EDIT", initial.c_str(),
                                   WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                                   static_cast<int>(fieldX * g_scale), static_cast<int>(fieldY * g_scale),
                                   static_cast<int>(fieldW * g_scale), static_cast<int>(fieldH * g_scale),
                                   g_hwnd, nullptr, GetModuleHandle(nullptr), nullptr);
    if (g_promptEdit) {
        SendMessageW(g_promptEdit, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
        SendMessageW(g_promptEdit, EM_SETSEL, 0, -1);
        SetFocus(g_promptEdit);
    }

    InvalidateRect(g_hwnd, nullptr, FALSE);
}

void closeModal() {
    g_app.modal = ModalKind::None;
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

void openModal(ModalKind kind) {
    g_app.modal = kind;
    g_app.modalAnim = 0.0f;
    g_app.modalScroll = 0.0f;
    g_app.cardMenuOpen = false;
    wakeAnimations();
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

// The card "More" menu stores the command in arg2 above 1000.
void handleCardMenuAction(int action, int id, bool isFolder) {
    g_app.cardMenuOpen = false;

    if (!gDatabase) return;

    if (g_app.cardMenuInTrash) {
        if (action == CMD_RESTORE) {
            if (gStorage->restoreFile(id)) {
                pushToast(L"Restored to your library", ToastKind::Success);
            } else {
                pushToast(L"Could not restore that file", ToastKind::Error);
            }
        } else if (action == CMD_DELETE_FOREVER) {
            if (gStorage->permanentlyDeleteFile(id)) {
                pushToast(L"File deleted forever", ToastKind::Warn);
            } else {
                pushToast(L"Could not delete that file", ToastKind::Error);
            }
        }
        reloadData();
        InvalidateRect(g_hwnd, nullptr, FALSE);
        return;
    }

    if (isFolder) {
        auto folder = gDatabase->getFolderById(id);
        if (!folder) return;

        switch (action) {
            case CMD_OPEN_FILE: {  // Open
                std::vector<std::string> path;
                std::string current = folder->path;
                size_t start = 0;
                while (start <= current.size()) {
                    size_t slash = current.find('/', start);
                    std::string part = current.substr(
                        start, slash == std::string::npos ? std::string::npos : slash - start);
                    if (!part.empty()) path.push_back(part);
                    if (slash == std::string::npos) break;
                    start = slash + 1;
                }
                navigateTo(path);
                break;
            }
            case CMD_RENAME: {  // Rename
                openPrompt(L"Rename folder", neb::widen(folder->name), true, id);
                break;
            }
            case CMD_MOVE: {  // Move
                pushToast(L"Moving folders is not available in the desktop build yet", ToastKind::Info);
                break;
            }
            case CMD_DELETE: {  // Delete
                if (g_app.confirmDelete) {
                    if (MessageBoxW(g_hwnd, L"Move this folder and its contents to Trash?", L"Confirm",
                                    MB_YESNO | MB_ICONQUESTION) != IDYES) {
                        return;
                    }
                }
                if (gStorage->deleteFolder(id)) {
                    pushToast(L"Folder moved to Trash", ToastKind::Success);
                } else {
                    pushToast(L"Could not delete that folder", ToastKind::Error);
                }
                reloadData();
                break;
            }
            default: break;
        }
        InvalidateRect(g_hwnd, nullptr, FALSE);
        return;
    }

    auto file = gDatabase->getFile(id);
    if (!file) return;

    switch (action) {
        case CMD_CUT: {  // Cut / trim a clip
            pushToast(L"Trim and export is not available in the desktop build yet", ToastKind::Info);
            break;
        }
        case CMD_RENAME: {  // Rename
            openPrompt(L"Rename file", neb::widen(file->name), false, id);
            break;
        }
        case CMD_MOVE: {  // Move
            BROWSEINFOW bi = {};
            bi.hwndOwner = g_hwnd;
            bi.lpszTitle = L"Choose a destination folder";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
            if (pidl) {
                wchar_t path[MAX_PATH] = {};
                if (SHGetPathFromIDListW(pidl, path)) {
                    std::string dest = neb::narrow(path);
                    std::string target = g_app.currentPath.empty() ? dest : dest + "/" + joinPath(g_app.currentPath);
                    if (gStorage->moveFile(id, target)) {
                        pushToast(L"Moved to " + neb::widen(dest), ToastKind::Success);
                        reloadData();
                    } else {
                        pushToast(L"Could not move that file", ToastKind::Error);
                    }
                }
                CoTaskMemFree(pidl);
            }
            break;
        }
        case CMD_DELETE: {  // Delete
            if (g_app.confirmDelete) {
                if (MessageBoxW(g_hwnd, L"Move this file to Trash?", L"Confirm", MB_YESNO | MB_ICONQUESTION) != IDYES) {
                    return;
                }
            }
            if (gStorage->deleteFile(id)) {
                pushToast(L"Moved to Trash", ToastKind::Success);
            } else {
                pushToast(L"Could not delete that file", ToastKind::Error);
            }
            reloadData();
            break;
        }
        case CMD_OPEN_FILE: {  // Open with the default app
            std::string full = gStorage->getFilesPath() + "/" + file->path;
            if (std::filesystem::exists(toPath(full))) {
                HINSTANCE result = ShellExecuteW(g_hwnd, L"open", neb::widen(full).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                if (reinterpret_cast<INT_PTR>(result) <= 32) {
                    pushToast(L"No app is registered for that file type", ToastKind::Warn);
                }
            } else {
                pushToast(L"That file is missing from disk", ToastKind::Error);
            }
            break;
        }
        default: break;
    }
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

void saveFeedbackText(const std::wstring& text) {
    if (!gStorage) return;
    if (text.empty()) {
        pushToast(L"Write something first", ToastKind::Warn);
        return;
    }
    std::string path = gStorage->getStoragePath() + "/feedback.txt";
    std::ofstream file(toPath(path), std::ios::app | std::ios::binary);
    if (!file) {
        pushToast(L"Could not save your feedback", ToastKind::Error);
        return;
    }
    char stamp[32] = {};
    std::time_t now = std::time(nullptr);
    std::tm* local = std::localtime(&now);
    if (local) std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M", local);
    file << "[" << stamp << "] " << neb::narrow(text) << "\r\n";
    pushToast(L"Feedback saved - thank you!", ToastKind::Success);
}

bool copyTextToClipboard(const std::wstring& text) {
    if (!OpenClipboard(g_hwnd)) return false;
    EmptyClipboard();
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
    if (!mem) {
        CloseClipboard();
        return false;
    }
    void* dest = GlobalLock(mem);
    memcpy(dest, text.c_str(), (text.size() + 1) * sizeof(wchar_t));
    GlobalUnlock(mem);
    SetClipboardData(CF_UNICODETEXT, mem);
    CloseClipboard();
    return true;
}

// Finds the library folder once. It is made absolute right away because file
// dialogs change the process working directory, which used to make stored
// files look "missing from disk".
std::string resolveDataDir() {
    namespace fs = std::filesystem;
    std::error_code ec;

    // Where the library lives. A modern Windows app must not write next to the
    // exe (it is often inside Program Files and not writable), so the default
    // is the per-user Roaming folder — %APPDATA%\\NebulaDrive. Portable
    // installs (an existing NebulaDriveData next to the exe) keep using that
    // folder so they never lose their library.
    fs::path chosen;
    wchar_t exe[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    fs::path exeDir = exe[0] ? fs::path(exe).parent_path() : fs::current_path(ec);

    if (fs::exists(exeDir / L"NebulaDriveData", ec)) {
        chosen = exeDir / L"NebulaDriveData";
    } else {
        wchar_t appData[MAX_PATH] = {};
        if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData)) &&
            appData[0] != 0) {
            chosen = fs::path(appData) / L"NebulaDrive";
        } else {
            chosen = exeDir / L"NebulaDrive";
        }
    }

    fs::create_directories(chosen, ec);
    std::string result = neb::narrow(chosen.wstring());
    std::replace(result.begin(), result.end(), '\\', '/');
    return result;
}

void applyPromptResult() {
    // Always take the live text from the field, not the stored default.
    std::wstring value = readPromptValue();
    // Trim whitespace.
    size_t start = value.find_first_not_of(L" \t");
    size_t end = value.find_last_not_of(L" \t");
    value = (start == std::wstring::npos) ? L"" : value.substr(start, end - start + 1);

    bool isFolder = g_app.promptIsFolder;
    int targetId = g_app.promptTargetId;

    closePrompt();

    if (targetId == -100) {
        saveFeedbackText(value);
        return;
    }

    if (value.empty()) {
        pushToast(L"Name cannot be empty", ToastKind::Warn);
        return;
    }

    std::string name = neb::narrow(value);

    if (isFolder) {
        if (targetId < 0) {
            // Creating a new folder inside the current location.
            if (gStorage->createFolder(name, joinPath(g_app.currentPath))) {
                pushToast(L"Folder created: " + neb::widen(name), ToastKind::Success);
            } else {
                pushToast(L"Could not create that folder", ToastKind::Error);
            }
        } else {
            if (gStorage->renameFolder(targetId, name)) {
                pushToast(L"Folder renamed to " + neb::widen(name), ToastKind::Success);
            } else {
                pushToast(L"Could not rename that folder", ToastKind::Error);
            }
        }
        reloadData();
        InvalidateRect(g_hwnd, nullptr, FALSE);
        return;
    }

    if (gStorage->renameFile(targetId, name)) {
        pushToast(L"Renamed to " + neb::widen(name), ToastKind::Success);
    } else {
        pushToast(L"Could not rename that file", ToastKind::Error);
    }
    reloadData();
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

void toggleTheme(int index) {
    const auto& names = neb::themeNames();
    if (index < 0 || index >= static_cast<int>(names.size())) return;
    g_app.themeName = names[index];
    neb::applyTheme(g_app.themeName);
    saveSettings();
    pushToast(L"Theme: " + neb::widen(g_app.themeName), ToastKind::Info);
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

// Bulk actions on the current selection.
void moveSelected() {
    if (g_app.selected.empty() || !gStorage) {
        pushToast(L"Select some files first", ToastKind::Warn);
        return;
    }

    BROWSEINFOW bi = {};
    bi.hwndOwner = g_hwnd;
    bi.lpszTitle = L"Choose a destination folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;

    wchar_t path[MAX_PATH] = {};
    if (SHGetPathFromIDListW(pidl, path)) {
        std::string dest = neb::narrow(path);
        std::string target = g_app.currentPath.empty() ? dest : dest + "/" + joinPath(g_app.currentPath);
        int moved = 0;
        for (int id : g_app.selected) {
            if (gStorage->moveFile(id, target)) ++moved;
        }
        pushToast(L"Moved " + std::to_wstring(moved) + L" file(s)", ToastKind::Success);
        g_app.selected.clear();
        reloadData();
    }
    CoTaskMemFree(pidl);
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

void exportSelected() {
    if (g_app.selected.empty() || !gStorage) {
        pushToast(L"Select some files first", ToastKind::Warn);
        return;
    }

    BROWSEINFOW bi = {};
    bi.hwndOwner = g_hwnd;
    bi.lpszTitle = L"Choose a destination folder";
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
    if (!pidl) return;

    wchar_t path[MAX_PATH] = {};
    if (SHGetPathFromIDListW(pidl, path)) {
        std::string dest = neb::narrow(path);
        std::string target = g_app.currentPath.empty() ? dest : dest + "/" + joinPath(g_app.currentPath);
        int copied = 0;
        for (int id : g_app.selected) {
            if (gStorage->copyFile(id, target)) ++copied;
        }
        pushToast(L"Copied " + std::to_wstring(copied) + L" file(s)", ToastKind::Success);
    }
    CoTaskMemFree(pidl);
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

void deleteSelected() {
    if (g_app.selected.empty() || !gStorage) {
        pushToast(L"Select some files first", ToastKind::Warn);
        return;
    }

    if (g_app.confirmDelete) {
        if (MessageBoxW(g_hwnd, L"Move the selected files to Trash?", L"Confirm", MB_YESNO | MB_ICONQUESTION) !=
            IDYES) {
            return;
        }
    }

    int removed = 0;
    for (int id : g_app.selected) {
        if (gStorage->deleteFile(id)) ++removed;
    }
    pushToast(L"Moved " + std::to_wstring(removed) + L" file(s) to Trash", ToastKind::Success);
    g_app.selected.clear();
    reloadData();
    InvalidateRect(g_hwnd, nullptr, FALSE);
}

// Handles a command coming from a dropdown row or a settings action.
void handleCommand(int command) {
    switch (command) {
        case CMD_SORT_NAME: g_app.sortKey = SortKey::Name; break;
        case CMD_SORT_SIZE: g_app.sortKey = SortKey::Size; break;
        case CMD_SORT_DATE: g_app.sortKey = SortKey::Date; break;
        case CMD_SORT_TYPE: g_app.sortKey = SortKey::Type; break;
        case CMD_SORT_EXTENSION: g_app.sortKey = SortKey::Name; break;
        case CMD_SORT_ASC: g_app.sortAscending = true; break;
        case CMD_SORT_DESC: g_app.sortAscending = false; break;
        case CMD_VIEW_MEDIUM: g_app.view = ViewMode::GridMedium; break;
        case CMD_VIEW_SMALL: g_app.view = ViewMode::GridSmall; break;
        case CMD_VIEW_LARGE: g_app.view = ViewMode::GridLarge; break;
        case CMD_VIEW_LIST: g_app.view = ViewMode::List; break;
        default: break;
    }

    if (command >= CMD_SORT_NAME && command <= CMD_SORT_DESC) {
        sortItems();
        saveSettings();
        g_app.modal = ModalKind::None;
        InvalidateRect(g_hwnd, nullptr, FALSE);
        return;
    }

    if (command >= CMD_VIEW_MEDIUM && command <= CMD_VIEW_LIST) {
        saveSettings();
        g_app.modal = ModalKind::None;
        InvalidateRect(g_hwnd, nullptr, FALSE);
        return;
    }

    switch (command) {
        case CMD_SCAN: {
            // Look for records whose file is no longer on disk.
            int missing = 0;
            if (gDatabase && gStorage) {
                for (const auto& file : gDatabase->listFiles(true)) {
                    std::string full = gStorage->getFilesPath() + "/" + file.path;
                    if (!std::filesystem::exists(toPath(full))) {
                        ++missing;
                    }
                }
            }
            pushToast(missing == 0 ? L"Library looks healthy \u2014 nothing missing"
                                   : std::to_wstring(missing) + L" file(s) missing from disk",
                      missing == 0 ? ToastKind::Success : ToastKind::Warn);
            break;
        }

        case CMD_ZIP_EXPORT: {
            if (gStorage->createBackup()) {
                pushToast(L"Backup created", ToastKind::Success);
            } else {
                pushToast(L"Could not create the backup", ToastKind::Error);
            }
            reloadData();
            break;
        }

        case CMD_ZIP_IMPORT:
            pushToast(L"ZIP restore is not available in the desktop build yet", ToastKind::Info);
            break;

        case CMD_SET_PASSWORD:
            pushToast(L"Set a password from your account menu", ToastKind::Info);
            break;

        case CMD_EXPORT_LOGIN: {
            // Write a small profile file the user can keep.
            if (gDatabase) {
                std::string path = gStorage->getStoragePath() + "/nebuladrive-profile.json";
                std::string contents =
                    "{\n  \"account\": \"Makerxm800\",\n  \"version\": \"1.0.0\",\n  \"note\": "
                    "\"NebulaDrive local profile export\"\n}\n";
                std::ofstream file(path);
                if (file) {
                    file << contents;
                    pushToast(L"Profile exported", ToastKind::Success);
                } else {
                    pushToast(L"Could not write the profile", ToastKind::Error);
                }
            }
            break;
        }

        case CMD_LOGOUT:
            pushToast(L"Signed out of this device", ToastKind::Info);
            break;

        case CMD_RESET_CACHE:
            if (gDatabase) {
                gDatabase->vacuum();
                pushToast(L"Cache reset", ToastKind::Success);
            }
            break;

        case CMD_FEEDBACK:
            g_app.legalDoc = 3;
            openModal(ModalKind::Legal);
            break;

        case CMD_FEEDBACK_WRITE:
            openPrompt(L"Send feedback", L"", false, -100);
            break;

        case CMD_FEEDBACK_OPEN: {
            std::string path = gStorage->getStoragePath() + "/feedback.txt";
            if (std::filesystem::exists(toPath(path))) {
                ShellExecuteW(g_hwnd, L"open", toPath(path).wstring().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
            } else {
                pushToast(L"You have not written any feedback yet", ToastKind::Info);
            }
            break;
        }

        case CMD_COPY_DIAG: {
            std::wstring diag = L"NebulaDrive 1.0.0 (Beta)\r\n";
            diag += L"Architecture: " + std::to_wstring(sizeof(void*) * 8) + L"-bit\r\n";
            diag += L"Files: " + std::to_wstring(g_app.stats.totalFiles) + L"\r\n";
            diag += L"Folders: " + std::to_wstring(g_app.stats.totalFolders) + L"\r\n";
            diag += L"Used: " + neb::widen(formatSizePlain(g_app.stats.totalSize)) + L"\r\n";
            diag += L"Display scale: " + std::to_wstring(static_cast<int>(g_scale * 100.0f)) + L"%\r\n";
            diag += L"Theme: " + neb::widen(g_app.themeName) + L"\r\n";
            if (copyTextToClipboard(diag)) {
                pushToast(L"Diagnostics copied", ToastKind::Success);
            } else {
                pushToast(L"Could not reach the clipboard", ToastKind::Error);
            }
            break;
        }

        case CMD_DANGER_CLEAR: {
            if (MessageBoxW(g_hwnd, L"Delete every file in your library? This cannot be undone.", L"Confirm",
                            MB_YESNO | MB_ICONWARNING) == IDYES) {
                int removed = 0;
                if (gDatabase && gStorage) {
                    for (const auto& file : gDatabase->listFiles(false)) {
                        std::string full = gStorage->getFilesPath() + "/" + file.path;
                        std::error_code ec;
                        std::filesystem::remove(full, ec);
                        gDatabase->hardDelete(file.id);
                        ++removed;
                    }
                }
                pushToast(L"Cleared " + std::to_wstring(removed) + L" file(s)", ToastKind::Warn);
                reloadData();
            }
            break;
        }

        case CMD_DANGER_DELETE:
            pushToast(L"Delete Account is not available in the desktop build yet", ToastKind::Info);
            break;

        case CMD_DANGER_TERMINATE:
            pushToast(L"Terminate Account is not available in the desktop build yet", ToastKind::Info);
            break;

        default:
            break;
    }

    InvalidateRect(g_hwnd, nullptr, FALSE);
}

} // namespace

// ============================================================
// Window procedure
// ============================================================

namespace {

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            g_hwnd = hwnd;

            gDatabase = &Database::instance();
            if (!gDatabase->initialize(resolveDataDir() + "/nebuladrive.db")) {
                MessageBoxW(hwnd, L"Could not open the NebulaDrive database.", L"NebulaDrive",
                            MB_OK | MB_ICONERROR);
                return -1;
            }

            gStorage = new StorageHandler(resolveDataDir(), *gDatabase);
            if (!gStorage->initialize()) {
                MessageBoxW(hwnd, L"Could not prepare the storage folder.", L"NebulaDrive", MB_OK | MB_ICONERROR);
                return -1;
            }

            loadSettings();

            // Restore the last location.
            std::string last = gDatabase->getSetting("last_folder");
            std::vector<std::string> path;
            if (g_app.rememberLastFolder && !last.empty()) {
                size_t start = 0;
                while (start <= last.size()) {
                    size_t slash = last.find('/', start);
                    std::string part = last.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
                    if (!part.empty()) path.push_back(part);
                    if (slash == std::string::npos) break;
                    start = slash + 1;
                }
            }
            navigateTo(path, true);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);

            RECT rc;
            GetClientRect(hwnd, &rc);
            int width = rc.right;
            int height = rc.bottom;
            if (width <= 0 || height <= 0) {
                EndPaint(hwnd, &ps);
                return 0;
            }

            // Persistent off-screen buffer: created once and only rebuilt when the
            // window size changes (the old code allocated a new bitmap every frame).
            static HDC bufDc = nullptr;
            static HBITMAP bufBmp = nullptr;
            static HGDIOBJ bufOld = nullptr;
            static int bufW = 0, bufH = 0;
            if (!bufDc || bufW != width || bufH != height) {
                if (bufDc) {
                    SelectObject(bufDc, bufOld);
                    DeleteObject(bufBmp);
                    DeleteDC(bufDc);
                }
                bufDc = CreateCompatibleDC(hdc);
                BITMAPINFO bi = {};
                bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bi.bmiHeader.biWidth = width;
                bi.bmiHeader.biHeight = -height;
                bi.bmiHeader.biPlanes = 1;
                bi.bmiHeader.biBitCount = 32;
                bi.bmiHeader.biCompression = BI_RGB;
                void* bits = nullptr;
                bufBmp = CreateDIBSection(hdc, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
                bufOld = SelectObject(bufDc, bufBmp);
                bufW = width;
                bufH = height;
            }

            {
                Graphics graphics(bufDc);
                // Only rasterize the part of the window that actually needs repainting.
                RECT dirty = ps.rcPaint;
                graphics.SetClip(Rect(dirty.left, dirty.top, dirty.right - dirty.left,
                                      dirty.bottom - dirty.top));
                graphics.SetPageUnit(UnitPixel);
                graphics.SetPageScale(g_scale);
                paintWindow(graphics, RectF(0.0f, 0.0f, width / g_scale, height / g_scale));
            }

            BitBlt(hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left,
                   ps.rcPaint.bottom - ps.rcPaint.top, bufDc, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_ENTERSIZEMOVE:
            // While dragging the window edge, stretch the cached background instead
            // of rebuilding it on every size change.
            neb::setBackgroundDeferred(true);
            return 0;

        case WM_EXITSIZEMOVE:
            neb::setBackgroundDeferred(false);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_ERASEBKGND:
            return 1;

        case WM_TIMER: {
            if (wParam == 2) {
                KillTimer(hwnd, 2);
                if (g_tipKey != -1) {
                    g_tipVisible = true;
                    g_tipX = g_mouseLX;
                    g_tipY = g_mouseLY;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }
            bool active = tickAnimations();

            // Expire toasts.
            ULONGLONG now = GetTickCount64();
            bool changed = false;
            for (size_t i = 0; i < g_toasts.size();) {
                if (now - g_toasts[i].born > 3400 && !g_toasts[i].dying) {
                    g_toasts[i].dying = true;
                    g_toasts[i].slideTarget = 0.0f;
                    changed = true;
                }
                if (g_toasts[i].dying && g_toasts[i].slide <= 0.001f) {
                    g_toasts.erase(g_toasts.begin() + i);
                    changed = true;
                } else {
                    ++i;
                }
            }

            if (active || changed) {
                InvalidateRect(hwnd, nullptr, FALSE);
            } else {
                // Nothing is moving, so stop the timer until something does.
                // This keeps the app idle instead of waking 60 times a second.
                KillTimer(hwnd, 1);
            }
            return 0;
        }

        case WM_MOUSEMOVE: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            float lx = x / g_scale;
            float ly = y / g_scale;

            const HitRegion* hit = hitTest(lx, ly);
            int newId = hit ? hit->id : HIT_NONE;
            int newArg = hit ? hit->arg : 0;

            if (newId != g_hoverId || newArg != g_hoverArg) {
                // Reset the old hover target.
                if (g_hoverId != HIT_NONE) {
                    element(g_hoverId).hoverTarget = 0.0f;
                }
                g_hoverId = newId;
                g_hoverArg = newArg;
                if (newId != HIT_NONE) {
                    element(newId).hoverTarget = 1.0f;
                }
                wakeAnimations();
                // Card ids carry an index; reset the matching per-card state too.
                SetCursor(LoadCursor(nullptr, newId == HIT_NONE ? IDC_ARROW : IDC_HAND));
            }

            // Per-card hover needs the index, so refresh those targets as well.
            for (auto& kv : g_elements) {
                if (kv.first >= 100000) {
                    kv.second.hoverTarget = 0.0f;
                }
            }
            if (hit && (hit->id == HIT_CARD || hit->id == HIT_CARD_STAR || hit->id == HIT_CARD_MORE)) {
                element(cardKey(HIT_CARD, hit->arg2)).hoverTarget = 1.0f;
            }
            if (hit && (hit->id == HIT_MENU_ITEM || hit->id == HIT_LANGUAGE_ITEM)) {
                rowState(hit->id, hit->arg).hoverTarget = 1.0f;
                wakeAnimations();
            }

            // Tooltips appear after the pointer rests on a control for a moment.
            g_mouseLX = lx;
            g_mouseLY = ly;
            {
                std::wstring tip = hit ? tooltipFor(*hit) : L"";
                if (g_app.modal != ModalKind::None || g_app.prompting || g_app.cardMenuOpen || g_app.profileOpen) {
                    tip.clear();
                }
                int tipKey = (hit && !tip.empty()) ? (hit->id * 1000003 + hit->arg * 31 + hit->arg2) : -1;
                if (tipKey != g_tipKey) {
                    g_tipKey = tipKey;
                    KillTimer(hwnd, 2);
                    if (g_tipVisible) {
                        g_tipVisible = false;
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    if (tipKey != -1) {
                        g_tipText = tip;
                        SetTimer(hwnd, 2, 420, nullptr);
                    }
                }
            }

            if (g_app.dragOver) {
                g_app.dragOver = false;
                InvalidateRect(hwnd, nullptr, FALSE);
            }

            TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, hwnd, 0};
            TrackMouseEvent(&tme);
            return 0;
        }

        case WM_MOUSELEAVE: {
            KillTimer(hwnd, 2);
            g_tipKey = -1;
            g_tipVisible = false;
            if (g_hoverId != HIT_NONE) {
                element(g_hoverId).hoverTarget = 0.0f;
                g_hoverId = HIT_NONE;
                g_hoverArg = 0;
            }
            for (auto& kv : g_elements) {
                if (kv.first >= 100000) kv.second.hoverTarget = 0.0f;
            }
            wakeAnimations();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            float lx = x / g_scale;
            float ly = y / g_scale;

            KillTimer(hwnd, 2);
            g_tipKey = -1;
            g_tipVisible = false;

            const HitRegion* hit = hitTest(lx, ly);
            if (!hit) {
                // Clicking away closes the card menu.
                if (g_app.cardMenuOpen) {
                    g_app.cardMenuOpen = false;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }

            // While the prompt is open only its own buttons respond.
            if (g_app.prompting && hit->id != HIT_NEW_FOLDER_OK && hit->id != HIT_NEW_FOLDER_CANCEL &&
                hit->id != HIT_MODAL_CLOSE) {
                return 0;
            }

            element(hit->id).pressTarget = 1.0f;
            wakeAnimations();
            SetCapture(hwnd);
            return 0;
        }

        case WM_LBUTTONUP: {
            int x = GET_X_LPARAM(lParam);
            int y = GET_Y_LPARAM(lParam);
            float lx = x / g_scale;
            float ly = y / g_scale;

            ReleaseCapture();

            const HitRegion* hit = hitTest(lx, ly);
            int id = hit ? hit->id : HIT_NONE;
            int arg = hit ? hit->arg : 0;
            int arg2 = hit ? hit->arg2 : 0;
            bool isFolder = hit ? hit->isFolder : false;

            if (id != HIT_NONE) {
                element(id).pressTarget = 0.0f;
                wakeAnimations();
            }

            if (!hit) {
                bool fullModal = g_app.modal == ModalKind::Settings || g_app.modal == ModalKind::Info ||
                                 g_app.modal == ModalKind::Language || g_app.modal == ModalKind::Legal;
                if (fullModal && !g_app.prompting) closeModal();
                return 0;
            }

            // Card overflow menu items carry their action in arg2.
            if (id == HIT_CARD_MORE && arg2 >= 1000) {
                handleCardMenuAction(arg2 - 1000, arg, isFolder);
                return 0;
            }

            // Rows in the dropdown menus and the settings panel.
            if (id == HIT_MENU_ITEM) {
                handleCommand(arg);
                return 0;
            }

            if (id == HIT_LANGUAGE_ITEM) {
                size_t index = static_cast<size_t>(arg - CMD_LANG_EN);
                if (index < sizeof(kLanguages) / sizeof(kLanguages[0])) {
                    g_app.language = kLanguages[index].code;
                    saveSettings();
                    closeModal();
                    pushToast(std::wstring(L"Language: ") + kLanguages[index].label, ToastKind::Success);
                }
                return 0;
            }

            if (id == HIT_THEME_SEGMENT) {
                if (arg == CMD_THEME_ALL) g_app.themeFilter = ThemeFilter::All;
                else if (arg == CMD_THEME_STANDARD) g_app.themeFilter = ThemeFilter::Standard;
                else g_app.themeFilter = ThemeFilter::Premium;
                saveSettings();
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            switch (id) {
                case HIT_TAB_FILES: switchTab(Tab::Files); break;
                case HIT_TAB_FAVORITES: switchTab(Tab::Favorites); break;
                case HIT_TAB_HISTORY: switchTab(Tab::History); break;
                case HIT_TAB_TRASH: switchTab(Tab::Trash); break;

                case HIT_LANG:
                    openModal(ModalKind::Language);
                    break;

                case HIT_SETTINGS: openModal(ModalKind::Settings); break;
                case HIT_INFO: openModal(ModalKind::Info); break;

                case HIT_USER_CHIP:
                    g_app.profileOpen = !g_app.profileOpen;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_BACK:
                    if (g_app.historyIndex > 0) {
                        g_app.historyIndex--;
                        navigateTo(g_app.pathHistory[g_app.historyIndex], false);
                    }
                    break;

                case HIT_FORWARD:
                    if (g_app.historyIndex >= 0 &&
                        g_app.historyIndex < static_cast<int>(g_app.pathHistory.size()) - 1) {
                        g_app.historyIndex++;
                        navigateTo(g_app.pathHistory[g_app.historyIndex], false);
                    }
                    break;

                case HIT_REFRESH:
                    reloadData();
                    pushToast(L"Refreshed", ToastKind::Info);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_NEW_FOLDER:
                    if (g_app.prompting) {
                        applyPromptResult();
                    } else {
                        openPrompt(L"New folder", L"New folder", true, -1);
                    }
                    break;

                case HIT_NEW_FOLDER_OK:
                    if (g_app.prompting) {
                        applyPromptResult();
                    }
                    break;

                case HIT_NEW_FOLDER_CANCEL:
                    closePrompt();
                    break;

                case HIT_SORT:
                    g_app.modal = (g_app.modal == ModalKind::SortMenu) ? ModalKind::None : ModalKind::SortMenu;
                    g_app.modalAnim = 1.0f;
                    wakeAnimations();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_VIEW:
                    g_app.modal = (g_app.modal == ModalKind::ViewMenu) ? ModalKind::None : ModalKind::ViewMenu;
                    g_app.modalAnim = 1.0f;
                    wakeAnimations();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_FAV_FILTER:
                    switchTab(g_app.tab == Tab::Favorites ? Tab::Files : Tab::Favorites);
                    break;

                case HIT_SEARCH:
                    openSearch();
                    break;

                case HIT_DROPZONE: {
                    OPENFILENAMEW ofn = {};
                    wchar_t file[512] = {};
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hwnd;
                    ofn.lpstrFile = file;
                    ofn.nMaxFile = _countof(file);
                    ofn.lpstrFilter = L"All Files\0*.*\0";
                    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY;
                    if (GetOpenFileNameW(&ofn)) {
                        // Collect every selected file.
                        std::vector<std::string> paths;
                        wchar_t* current = file;
                        while (current && *current) {
                            paths.push_back(neb::narrow(current));
                            size_t len = wcslen(current);
                            if (len + 1 < _countof(file)) {
                                current += len + 1;
                            } else {
                                break;
                            }
                        }
                        int added = gStorage->importFiles(paths, joinPath(g_app.currentPath));
                        pushToast(L"Imported " + std::to_wstring(added) + L" file" +
                                      (added == 1 ? L"" : L"s"),
                                  ToastKind::Success);
                        reloadData();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    break;
                }

                case HIT_SELECT_TOGGLE:
                    g_app.selectionMode = !g_app.selectionMode;
                    g_app.selected.clear();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_SELECT_ALL: {
                    if (g_app.selected.size() == g_app.files.size() && !g_app.files.empty()) {
                        g_app.selected.clear();
                    } else {
                        g_app.selected.clear();
                        for (const auto& file : g_app.files) {
                            g_app.selected.push_back(file.id);
                        }
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;
                }

                case HIT_SELECT_DESELECT:
                    g_app.selected.clear();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_SELECT_MOVE:
                    moveSelected();
                    break;

                case HIT_SELECT_EXPORT:
                    exportSelected();
                    break;

                case HIT_SELECT_DELETE:
                    deleteSelected();
                    break;

                case HIT_CARD: {
                    if (arg == -2) {  // Root
                        navigateTo({});
                        break;
                    }
                    if (arg == -3) {  // Breadcrumb segment
                        std::vector<std::string> path;
                        for (int i = 0; i <= arg2 && i < static_cast<int>(g_app.currentPath.size()); ++i) {
                            path.push_back(g_app.currentPath[i]);
                        }
                        navigateTo(path);
                        break;
                    }
                    if (isFolder) {
                        // Open the folder by splitting its stored path.
                        auto folder = gDatabase->getFolderById(arg);
                        if (!folder) break;
                        std::vector<std::string> path;
                        std::string current = folder->path;
                        size_t start = 0;
                        while (start <= current.size()) {
                            size_t slash = current.find('/', start);
                            std::string part = current.substr(
                                start, slash == std::string::npos ? std::string::npos : slash - start);
                            if (!part.empty()) path.push_back(part);
                            if (slash == std::string::npos) break;
                            start = slash + 1;
                        }
                        navigateTo(path);
                        break;
                    }

                    // In select mode a click toggles the selection.
                    if (g_app.selectionMode) {
                        auto it = std::find(g_app.selected.begin(), g_app.selected.end(), arg);
                        if (it != g_app.selected.end()) {
                            g_app.selected.erase(it);
                        } else {
                            g_app.selected.push_back(arg);
                        }
                        InvalidateRect(hwnd, nullptr, FALSE);
                        break;
                    }

                    g_app.bottomFileId = arg;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;
                }

                case HIT_CARD_STAR: {
                    if (gDatabase->toggleFavorite(arg)) {
                        reloadData();
                        InvalidateRect(hwnd, nullptr, FALSE);
                    }
                    break;
                }

                case HIT_CARD_MORE: {
                    g_app.cardMenuOpen = true;
                    g_app.cardMenuId = arg;
                    g_app.cardMenuIsFolder = isFolder;
                    g_app.cardMenuInTrash = g_app.tab == Tab::Trash;
                    g_app.cardMenuName.clear();
                    if (isFolder) {
                        auto folder = gDatabase->getFolderById(arg);
                        if (folder) g_app.cardMenuName = folder->name;
                    } else {
                        auto file = gDatabase->getFile(arg);
                        if (file) g_app.cardMenuName = file->name;
                    }
                    // Anchor the menu under the button, kept inside the window.
                    float menuW = 246.0f;
                    float menuH = 46.0f + 46.0f * 4.0f + 8.0f;
                    float menuX = hit->rect.X + hit->rect.Width - menuW;
                    float menuY = hit->rect.Y + hit->rect.Height + 4.0f;
                    if (menuX < 12.0f) menuX = 12.0f;
                    if (menuY + menuH > areaHeight() - 70.0f) {
                        menuY = hit->rect.Y - menuH - 4.0f;
                    }
                    g_app.cardMenuRect = RectF(menuX, menuY, menuW, menuH);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;
                }

                case HIT_BOTTOM_OPEN:
                    if (g_app.bottomFileId > 0 && gDatabase) {
                        auto file = gDatabase->getFile(g_app.bottomFileId);
                        if (file) {
                            std::string full = gStorage->getFilesPath() + "/" + file->path;
                            if (std::filesystem::exists(toPath(full))) {
                                ShellExecuteW(hwnd, L"open", neb::widen(full).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
                            } else {
                                pushToast(L"That file is missing from disk", ToastKind::Error);
                            }
                        }
                    }
                    break;

                case HIT_BOTTOM_CLEAR:
                    g_app.bottomFileId = -1;
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_MODAL_CLOSE:
                    if (g_app.prompting) {
                        closePrompt();
                    } else {
                        closeModal();
                    }
                    break;

                case HIT_TGL_LIQUID:
                    g_app.liquidGlass = !g_app.liquidGlass;
                    neb::setGlassEnabled(g_app.liquidGlass);
                    saveSettings();
                    pushToast(g_app.liquidGlass ? L"Liquid Glass on" : L"Liquid Glass off", ToastKind::Info);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_TGL_ANIM:
                    g_app.animations = !g_app.animations;
                    saveSettings();
                    pushToast(g_app.animations ? L"Animations on" : L"Animations off", ToastKind::Info);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_TGL_CALM:
                    g_app.calmMotion = !g_app.calmMotion;
                    saveSettings();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_TGL_PREMIUM_FX:
                    g_app.premiumFx = !g_app.premiumFx;
                    saveSettings();
                    pushToast(g_app.premiumFx ? L"Premium FX on" : L"Premium FX off", ToastKind::Info);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_TGL_SAVE_LOGIN:
                    g_app.saveLogin = !g_app.saveLogin;
                    saveSettings();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_TGL_CONFIRM:
                    g_app.confirmDelete = !g_app.confirmDelete;
                    saveSettings();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_TGL_EXTENSIONS:
                    g_app.showExtensions = !g_app.showExtensions;
                    saveSettings();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_TGL_OPENFOLDER:
                    g_app.openFoldersOnClick = !g_app.openFoldersOnClick;
                    saveSettings();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_TGL_REMEMBER:
                    g_app.rememberLastFolder = !g_app.rememberLastFolder;
                    saveSettings();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_THEME: toggleTheme(arg); break;

                case HIT_MODAL_SCAN:
                    reloadData();
                    pushToast(L"Library looks healthy", ToastKind::Success);
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_MODEL_BACKUP:
                    if (gStorage->createBackup()) {
                        pushToast(L"Backup created", ToastKind::Success);
                    } else {
                        pushToast(L"Could not create the backup", ToastKind::Error);
                    }
                    reloadData();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;

                case HIT_MODAL_VACUUM:
                    if (gDatabase->vacuum()) {
                        pushToast(L"Database optimised", ToastKind::Success);
                    } else {
                        pushToast(L"Could not optimise the database", ToastKind::Error);
                    }
                    break;

                case HIT_MODAL_EMPTY_TRASH:
                    if (MessageBoxW(hwnd, L"Permanently delete everything in Trash?", L"Confirm",
                                    MB_YESNO | MB_ICONWARNING) == IDYES) {
                        int removed = 0;
                        if (gDatabase && gStorage) {
                            // Removes the bytes on disk as well as the records.
                            removed = gStorage->emptyTrash();
                            reloadData();
                            InvalidateRect(hwnd, nullptr, FALSE);
                        }
                        pushToast(L"Emptied trash (" + std::to_wstring(removed) + L" items)", ToastKind::Warn);
                    }
                    break;

                case HIT_FOOTER_PRIVACY: g_app.legalDoc = 0; openModal(ModalKind::Legal); break;
                case HIT_FOOTER_TERMS: g_app.legalDoc = 1; openModal(ModalKind::Legal); break;
                case HIT_FOOTER_COOKIE: g_app.legalDoc = 2; openModal(ModalKind::Legal); break;
                case HIT_FOOTER_FEEDBACK: g_app.legalDoc = 3; openModal(ModalKind::Legal); break;

                default:
                    break;
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            int delta = GET_WHEEL_DELTA_WPARAM(wParam);
            float step = delta > 0 ? -60.0f : 60.0f;

            if (g_app.modal != ModalKind::None) {
                // Scroll inside the open modal.
                float next = g_app.modalScroll + step;
                if (next < 0.0f) next = 0.0f;
                float maxScroll = std::max(0.0f, g_app.modalContentHeight - 400.0f);
                if (next > maxScroll) next = maxScroll;
                if (next != g_app.modalScroll) {
                    g_app.modalScroll = next;
                    InvalidateRect(hwnd, nullptr, FALSE);
                }
                return 0;
            }

            float scroll = g_app.scrollOffset + step;
            if (scroll < 0.0f) scroll = 0.0f;
            if (scroll > g_app.maxScroll) scroll = g_app.maxScroll;
            if (scroll != g_app.scrollOffset) {
                g_app.scrollOffset = scroll;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_KEYDOWN: {
            if (g_app.prompting) {
                if (wParam == VK_ESCAPE) {
                    closePrompt();
                } else if (wParam == VK_RETURN) {
                    applyPromptResult();
                }
                return 0;
            }

            switch (wParam) {
                case VK_ESCAPE:
                    if (g_app.cardMenuOpen) {
                        g_app.cardMenuOpen = false;
                    } else if (g_app.modal != ModalKind::None) {
                        closeModal();
                    }
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;
                case VK_F5:
                    reloadData();
                    InvalidateRect(hwnd, nullptr, FALSE);
                    break;
                case VK_LEFT:
                case VK_BACK:
                    if (g_app.historyIndex > 0) {
                        g_app.historyIndex--;
                        navigateTo(g_app.pathHistory[g_app.historyIndex], false);
                    }
                    break;
                case VK_RIGHT:
                    if (g_app.historyIndex >= 0 &&
                        g_app.historyIndex < static_cast<int>(g_app.pathHistory.size()) - 1) {
                        g_app.historyIndex++;
                        navigateTo(g_app.pathHistory[g_app.historyIndex], false);
                    }
                    break;
                default:
                    break;
            }
            return 0;
        }

        case WM_COMMAND: {
            if (LOWORD(wParam) == 1001 && HIWORD(wParam) == EN_CHANGE && g_searchEdit) {
                int len = GetWindowTextLengthW(g_searchEdit);
                std::wstring value(static_cast<size_t>(len) + 1, 0);
                GetWindowTextW(g_searchEdit, &value[0], len + 1);
                value.resize(static_cast<size_t>(len));
                g_app.searchQuery = neb::narrow(value);
                g_app.searching = true;
                reloadData();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }

        case WM_CTLCOLOREDIT: {
            // Keep the inline text fields in the dark theme.
            const neb::Theme& theme = neb::currentTheme();
            SetTextColor((HDC)wParam, theme.text.GetR() | (theme.text.GetG() << 8) | (theme.text.GetB() << 16));
            SetBkColor((HDC)wParam, theme.light ? RGB(240, 242, 245) : RGB(26, 24, 48));
            static HBRUSH darkBrush = CreateSolidBrush(RGB(26, 24, 48));
            static HBRUSH lightBrush = CreateSolidBrush(RGB(240, 242, 245));
            return (LRESULT)(theme.light ? lightBrush : darkBrush);
        }

        case WM_CTLCOLORSTATIC: {
            const neb::Theme& theme = neb::currentTheme();
            SetTextColor((HDC)wParam, theme.text.GetR() | (theme.text.GetG() << 8) | (theme.text.GetB() << 16));
            SetBkMode((HDC)wParam, TRANSPARENT);
            return (LRESULT)GetStockObject(NULL_BRUSH);
        }

        case WM_SIZE: {
            layoutSearchEdit();
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_DPICHANGED: {
            g_scale = HIWORD(wParam) / 96.0f;
            RECT* suggested = reinterpret_cast<RECT*>(lParam);
            SetWindowPos(hwnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left,
                         suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_DESTROY: {
            if (gStorage) {
                delete gStorage;
                gStorage = nullptr;
            }
            if (gDatabase) {
                gDatabase->close();
            }
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace

// ============================================================
// Entry point
// ============================================================

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show) {
    UNREFERENCED_PARAMETER(hPrev);
    UNREFERENCED_PARAMETER(cmd);

    // Without this Windows ticks timers every 15.6 ms, so a 16 ms animation timer
    // really fires every 31 ms (about 32 frames per second).
    timeBeginPeriod(1);

    INITCOMMONCONTROLSEX icx = {};
    icx.dwSize = sizeof(icx);
    icx.dwICC = ICC_LISTVIEW_CLASSES | ICC_BAR_CLASSES;
    InitCommonControlsEx(&icx);

    // High-DPI awareness so the app is crisp on scaled displays.
    SetProcessDPIAware();
    HDC screen = GetDC(nullptr);
    if (screen) {
        g_scale = GetDeviceCaps(screen, LOGPIXELSX) / 96.0f;
        ReleaseDC(nullptr, screen);
    }
    if (g_scale < 1.0f) g_scale = 1.0f;

    neb::uiStartup();

    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = L"NebulaDriveWindow";
    wc.hIcon = static_cast<HICON>(LoadImageW(hInst, MAKEINTRESOURCEW(101), IMAGE_ICON,
                                             GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
                                             LR_DEFAULTCOLOR));
    wc.hIconSm = static_cast<HICON>(LoadImageW(hInst, MAKEINTRESOURCEW(101), IMAGE_ICON,
                                               GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                               LR_DEFAULTCOLOR));
    if (!wc.hIcon) wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    if (!wc.hIconSm) wc.hIconSm = wc.hIcon;
    RegisterClassExW(&wc);

    int width = static_cast<int>(1280 * g_scale);
    int height = static_cast<int>(840 * g_scale);

    HWND hwnd = CreateWindowExW(0, L"NebulaDriveWindow", L"NebulaDrive (Beta)",
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
                                nullptr, nullptr, hInst, nullptr);
    if (!hwnd) {
        neb::uiShutdown();
        return 0;
    }

    // Center on the primary display.
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;
    SetWindowPos(hwnd, nullptr, (GetSystemMetrics(SM_CXSCREEN) - winW) / 2,
                 (GetSystemMetrics(SM_CYSCREEN) - winH) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    ShowWindow(hwnd, show);
    UpdateWindow(hwnd);

    // The animation loop runs continuously but only repaints when needed.
    SetTimer(hwnd, 1, 16, nullptr);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        if (g_promptEdit && IsDialogMessageW(g_promptEdit, &msg)) {
            continue;
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    neb::uiShutdown();
    timeEndPeriod(1);
    return static_cast<int>(msg.wParam);
}
