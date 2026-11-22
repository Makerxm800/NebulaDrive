#ifndef NEBULADRIVE_NEBULA_UI_H
#define NEBULADRIVE_NEBULA_UI_H

#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>

// ============================================================
// NebulaDrive UI toolkit - GDI+ based rendering primitives
// Shared by the desktop shell so the window matches the web app.
// ============================================================

namespace neb {

// Builds a colour from red/green/blue plus an alpha channel.
// GDI+'s four argument Color constructor takes (alpha, r, g, b), which is
// easy to get wrong, so every colour goes through this helper instead.
Gdiplus::Color rgba(BYTE r, BYTE g, BYTE b, BYTE a);

// --- Icons (vector, drawn from normalized 24x24 paths) ---
enum class Icon {
    None,
    Folder,
    Star,
    StarFilled,
    Clock,
    Trash,
    Gear,
    Info,
    Globe,
    Search,
    Back,
    Forward,
    Refresh,
    Plus,
    Download,
    Eye,
    More,
    Home,
    Upload,
    Check,
    User,
    Music,
    Video,
    Document,
    Image,
    Archive,
    FolderPlus
};

// --- Theme ---
struct Theme {
    std::string name;
    bool light = false;
    bool premium = false;   // premium themes carry the gold badge

    Gdiplus::Color bgTop{15, 12, 41};
    Gdiplus::Color bgMid{48, 43, 99};
    Gdiplus::Color bgBottom{36, 36, 62};

    Gdiplus::Color glass = rgba(255, 255, 255, 18);          // rgba(255,255,255,.07)
    Gdiplus::Color glassStrong = rgba(255, 255, 255, 26);    // rgba(255,255,255,.10)
    Gdiplus::Color glassHover = rgba(255, 255, 255, 46);     // rgba(255,255,255,.18)
    Gdiplus::Color border = rgba(255, 255, 255, 31);         // rgba(255,255,255,.12)
    Gdiplus::Color borderStrong = rgba(255, 255, 255, 46);
    Gdiplus::Color cardBg = rgba(255, 255, 255, 13);         // rgba(255,255,255,.05)

    Gdiplus::Color text{255, 255, 255};
    Gdiplus::Color textSecondary = rgba(255, 255, 255, 199); // rgba(255,255,255,.78)
    Gdiplus::Color textMuted = rgba(255, 255, 255, 128);     // rgba(255,255,255,.50)

    Gdiplus::Color accentA{102, 126, 234};            // #667eea
    Gdiplus::Color accentB{118, 75, 162};             // #764ba2
    Gdiplus::Color accentSolid{102, 126, 234};

    Gdiplus::Color danger{239, 68, 68};
    Gdiplus::Color success{52, 211, 153};
    Gdiplus::Color warning{251, 191, 36};
    Gdiplus::Color info{129, 140, 248};
};

// Theme registry -----------------------------------------------------------
const std::vector<std::string>& themeNames();
const Theme* findTheme(const std::string& name);
const Theme& currentTheme();
void applyTheme(const std::string& name);

// While true, the cached background is stretched instead of rebuilt (live window resize).
void setBackgroundDeferred(bool deferred);

// Number of standard (non-premium) themes.
size_t standardThemeCount();
// Whether the theme at this index is a premium one.
bool themeIsPremium(size_t index);

// --- GDI+ lifecycle --------------------------------------------------------
void uiStartup();
void uiShutdown();

// Whether the frosted glass effect is enabled.
bool glassEnabled();
void setGlassEnabled(bool enabled);

// --- Drawing helpers -------------------------------------------------------

// Rounded rectangle path. GDI+ on MinGW has a private copy constructor for
// GraphicsPath, so the path is filled in place rather than returned by value.
void addRoundedRect(Gdiplus::GraphicsPath& path, float x, float y, float w, float h, float radius);

// Fills a rounded rect with a solid colour.
void fillRound(Gdiplus::Graphics& g, const Gdiplus::RectF& r, float radius, Gdiplus::Color c);

// Fills a rounded rect with a vertical two-stop gradient.
void fillRoundGradientV(Gdiplus::Graphics& g, const Gdiplus::RectF& r, float radius,
                        Gdiplus::Color top, Gdiplus::Color bottom);

// Fills a rounded rect with the accent gradient.
void fillRoundAccent(Gdiplus::Graphics& g, const Gdiplus::RectF& r, float radius);

// Strokes a rounded rect outline.
void strokeRound(Gdiplus::Graphics& g, const Gdiplus::RectF& r, float radius, Gdiplus::Color c,
                 float width = 1.0f);

// Glass panel: translucent fill, hairline border and a soft drop shadow.
void drawGlassPanel(Gdiplus::Graphics& g, const Gdiplus::RectF& r, float radius, bool hovered = false);

// Draws text inside a rect with alignment and clipping.
void drawText(Gdiplus::Graphics& g, const std::wstring& text, const Gdiplus::RectF& r,
              float fontSize, bool bold, Gdiplus::Color color,
              Gdiplus::StringAlignment hAlign = Gdiplus::StringAlignmentCenter,
              Gdiplus::StringAlignment vAlign = Gdiplus::StringAlignmentCenter,
              bool italic = false);

// Draws a single line of text at a point (left baseline aligned).
void drawTextAt(Gdiplus::Graphics& g, const std::wstring& text, float x, float y, float fontSize,
                bool bold, Gdiplus::Color color, bool italic = false);

// Measures a string.
Gdiplus::SizeF measureText(Gdiplus::Graphics& g, const std::wstring& text, float fontSize, bool bold,
                           bool italic = false);

// Draws a vector icon scaled into the given rect.
void drawIcon(Gdiplus::Graphics& g, Icon icon, const Gdiplus::RectF& r, Gdiplus::Color color,
              float strokeWidth = 2.0f);

// Draws text using the colour emoji font, so flag and emoji glyphs render.
void drawEmojiText(Gdiplus::Graphics& g, const std::wstring& text, const Gdiplus::RectF& r, float fontSize);

// Draws a small national flag as coloured shapes.
void drawFlag(Gdiplus::Graphics& g, const std::string& code, const Gdiplus::RectF& r);

// Draws the NebulaDrive app mark (rounded square + cloud arrow).
void drawLogoMark(Gdiplus::Graphics& g, const Gdiplus::RectF& r);

// Draws the full-page background gradient plus the soft accent glows.
void drawBackground(Gdiplus::Graphics& g, const Gdiplus::RectF& area, const Theme& t);

// Converts a std::string to UTF-16.
std::wstring widen(const std::string& s);
std::string narrow(const std::wstring& s);

} // namespace neb

#endif // NEBULADRIVE_NEBULA_UI_H
