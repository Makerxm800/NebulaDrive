#include "nebula_ui.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

namespace neb {

// ============================================================
// Lifecycle
// ============================================================

static ULONG_PTR g_token = 0;
static bool g_glassEnabled = true;

void uiStartup() {
    if (g_token != 0) return;
    GdiplusStartupInput input;
    GdiplusStartup(&g_token, &input, nullptr);
}

void uiShutdown() {
    if (g_token == 0) return;
    GdiplusShutdown(g_token);
    g_token = 0;
}

// Whether the frosted glass effect is enabled.
bool glassEnabled() {
    return g_glassEnabled;
}

void setGlassEnabled(bool enabled) {
    g_glassEnabled = enabled;
}

// ============================================================
// Text conversion
// ============================================================

Color rgba(BYTE r, BYTE g, BYTE b, BYTE a) {
    return Color::MakeARGB(a, r, g, b);
}

std::wstring widen(const std::string& s) {
    if (s.empty()) return L"";
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    if (len <= 1) return L"";
    std::wstring out(static_cast<size_t>(len - 1), 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], len);
    return out;
}

std::string narrow(const std::wstring& s) {
    if (s.empty()) return "";
    int len = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len <= 1) return "";
    std::string out(static_cast<size_t>(len - 1), 0);
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), -1, &out[0], len, nullptr, nullptr);
    return out;
}

// ============================================================
// Theme registry
// ============================================================

static Theme makeDark(const std::string& name, Color a, Color b, Color top, Color mid, Color bottom,
                      bool premium = false) {
    Theme t;
    t.name = name;
    t.light = false;
    t.premium = premium;
    t.bgTop = top;
    t.bgMid = mid;
    t.bgBottom = bottom;
    t.accentA = a;
    t.accentB = b;
    t.accentSolid = a;
    return t;
}

static Theme makeLight(const std::string& name, Color a, Color b) {
    Theme t;
    t.name = name;
    t.light = true;
    t.bgTop = Color(240, 242, 245);
    t.bgMid = Color(226, 230, 237);
    t.bgBottom = Color(213, 218, 228);
    t.glass = rgba(255, 255, 255, 209);
    t.glassStrong = rgba(255, 255, 255, 230);
    t.glassHover = rgba(255, 255, 255, 242);
    t.border = rgba(102, 126, 234, 51);
    t.borderStrong = rgba(102, 126, 234, 74);
    t.cardBg = rgba(255, 255, 255, 217);
    t.text = Color(26, 29, 46);
    t.textSecondary = rgba(26, 29, 46, 184);
    t.textMuted = rgba(26, 29, 46, 115);
    t.accentA = a;
    t.accentB = b;
    t.accentSolid = a;
    return t;
}

static std::vector<Theme> buildThemes() {
    std::vector<Theme> list;

    // --- Standard themes ---
    list.push_back(makeDark("Dark Nebula", Color(102, 126, 234), Color(118, 75, 162),
                            Color(15, 12, 41), Color(48, 43, 99), Color(36, 36, 62)));
    list.push_back(makeLight("Light Aurora", Color(91, 110, 245), Color(109, 78, 196)));
    list.push_back(makeDark("Azure Abyss", Color(14, 165, 233), Color(6, 182, 212),
                            Color(6, 22, 40), Color(8, 47, 73), Color(4, 18, 33)));
    list.push_back(makeDark("Crimson Horizon", Color(251, 146, 60), Color(236, 72, 153),
                            Color(32, 10, 28), Color(66, 20, 47), Color(28, 8, 26)));
    list.push_back(makeDark("Nordic Frost", Color(94, 129, 172), Color(136, 192, 208),
                            Color(16, 24, 36), Color(32, 47, 66), Color(18, 26, 40)));
    list.push_back(makeDark("Dracula Noir", Color(189, 147, 249), Color(255, 121, 198),
                            Color(24, 16, 40), Color(48, 26, 66), Color(26, 14, 42)));
    list.push_back(makeDark("Graphite", Color(107, 114, 128), Color(55, 65, 81),
                            Color(17, 19, 24), Color(34, 38, 46), Color(20, 22, 28)));
    list.push_back(makeLight("Silver Mist", Color(148, 163, 184), Color(100, 116, 139)));
    list.push_back(makeDark("Pine Grove", Color(77, 138, 104), Color(29, 71, 54),
                            Color(8, 24, 18), Color(18, 44, 32), Color(10, 28, 20)));
    list.push_back(makeLight("Desert Clay", Color(176, 122, 69), Color(236, 223, 204)));
    list.push_back(makeDark("Slate Blue", Color(91, 127, 180), Color(29, 38, 56),
                            Color(12, 18, 30), Color(26, 38, 58), Color(14, 20, 34)));

    // --- Premium themes ---
    list.push_back(makeDark("Midnight Gold", Color(245, 215, 110), Color(184, 134, 11),
                            Color(22, 18, 10), Color(48, 40, 20), Color(26, 22, 12), true));
    list.push_back(makeDark("Velvet Rose", Color(255, 111, 179), Color(168, 85, 247),
                            Color(30, 10, 30), Color(58, 20, 62), Color(28, 10, 32), true));
    list.push_back(makeDark("Emerald Noir", Color(16, 185, 129), Color(5, 150, 105),
                            Color(6, 26, 22), Color(10, 48, 40), Color(5, 20, 18), true));
    list.push_back(makeDark("Platinum Frost", Color(148, 163, 184), Color(51, 65, 85),
                            Color(14, 16, 22), Color(28, 33, 44), Color(16, 19, 26), true));
    list.push_back(makeDark("Crimson Ember", Color(239, 68, 68), Color(249, 115, 22),
                            Color(30, 10, 10), Color(58, 18, 14), Color(26, 8, 8), true));
    list.push_back(makeDark("Sapphire Royale", Color(59, 130, 246), Color(30, 64, 175),
                            Color(8, 16, 40), Color(16, 30, 74), Color(8, 12, 30), true));
    list.push_back(makeDark("Obsidian Chrome", Color(148, 163, 184), Color(71, 85, 105),
                            Color(10, 12, 16), Color(22, 26, 34), Color(12, 14, 20), true));
    list.push_back(makeDark("Aurora Borealis", Color(0, 212, 255), Color(123, 47, 190),
                            Color(8, 16, 34), Color(20, 26, 62), Color(10, 12, 36), true));
    list.push_back(makeDark("Cosmic Latte", Color(167, 139, 250), Color(236, 72, 153),
                            Color(28, 14, 30), Color(54, 26, 48), Color(30, 14, 34), true));
    list.push_back(makeDark("Forest Whisper", Color(52, 211, 153), Color(5, 150, 105),
                            Color(6, 28, 22), Color(12, 50, 40), Color(6, 22, 18), true));
    list.push_back(makeDark("Rose Gold", Color(244, 114, 182), Color(249, 115, 22),
                            Color(32, 12, 24), Color(60, 26, 36), Color(30, 10, 22), true));
    list.push_back(makeDark("Nebula Rose", Color(192, 132, 252), Color(244, 114, 182),
                            Color(28, 10, 32), Color(56, 20, 62), Color(24, 8, 28), true));
    list.push_back(makeDark("Arctic Midnight", Color(0, 212, 170), Color(14, 165, 233),
                            Color(6, 24, 28), Color(10, 40, 54), Color(6, 18, 26), true));
    list.push_back(makeDark("Sunset Boulevard", Color(245, 158, 11), Color(239, 68, 68),
                            Color(30, 14, 8), Color(58, 26, 16), Color(28, 10, 8), true));
    list.push_back(makeDark("Twilight Amethyst", Color(167, 139, 250), Color(129, 140, 248),
                            Color(16, 12, 32), Color(34, 26, 62), Color(18, 14, 36), true));
    list.push_back(makeDark("Carbon Fiber", Color(100, 116, 139), Color(51, 65, 85),
                            Color(10, 12, 16), Color(24, 28, 34), Color(12, 14, 18), true));
    list.push_back(makeDark("Emerald Frost", Color(52, 211, 153), Color(6, 182, 212),
                            Color(6, 26, 28), Color(10, 44, 50), Color(5, 20, 26), true));
    list.push_back(makeDark("Crimson Velvet", Color(220, 38, 38), Color(153, 27, 27),
                            Color(28, 8, 12), Color(54, 14, 18), Color(24, 6, 10), true));
    list.push_back(makeDark("Solarized Dusk", Color(38, 139, 210), Color(42, 161, 152),
                            Color(0, 43, 54), Color(7, 54, 66), Color(2, 32, 40), true));

    return list;
}

static std::vector<Theme> g_themes = buildThemes();
static size_t g_themeIndex = 0;

const std::vector<std::string>& themeNames() {
    static std::vector<std::string> names = [] {
        std::vector<std::string> n;
        for (const auto& t : g_themes) n.push_back(t.name);
        return n;
    }();
    return names;
}

// Index of the first premium theme, so the grid can be split into sections.
size_t standardThemeCount() {
    size_t count = 0;
    for (const auto& theme : g_themes) {
        if (theme.premium) break;
        ++count;
    }
    return count;
}

bool themeIsPremium(size_t index) {
    return index < g_themes.size() && g_themes[index].premium;
}

const Theme* findTheme(const std::string& name) {
    for (size_t i = 0; i < g_themes.size(); ++i) {
        if (g_themes[i].name == name) return &g_themes[i];
    }
    return nullptr;
}

const Theme& currentTheme() {
    return g_themes[g_themeIndex];
}

void applyTheme(const std::string& name) {
    for (size_t i = 0; i < g_themes.size(); ++i) {
        if (g_themes[i].name == name) {
            g_themeIndex = i;
            return;
        }
    }
}

// ============================================================
// Shape helpers
// ============================================================

// Rounded rectangle path. GDI+ on MinGW has a private copy constructor for
// GraphicsPath, so the path is filled in place rather than returned by value.
void addRoundedRect(GraphicsPath& path, float x, float y, float w, float h, float radius) {
    if (w <= 0 || h <= 0) return;
    if (radius * 2.0f > w) radius = w / 2.0f;
    if (radius * 2.0f > h) radius = h / 2.0f;
    float d = radius * 2.0f;

    path.AddArc(x, y, d, d, 180.0f, 90.0f);
    path.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    path.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    path.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    path.CloseFigure();
}

void fillRound(Graphics& g, const RectF& r, float radius, Color c) {
    if (r.Width <= 0 || r.Height <= 0) return;
    SolidBrush brush(c);
    GraphicsPath path;
    addRoundedRect(path, r.X, r.Y, r.Width, r.Height, radius);
    g.FillPath(&brush, &path);
}

void fillRoundGradientV(Graphics& g, const RectF& r, float radius, Color top, Color bottom) {
    if (r.Width <= 0 || r.Height <= 0) return;
    LinearGradientBrush brush(PointF(r.X, r.Y), PointF(r.X, r.Y + r.Height), top, bottom);
    GraphicsPath path;
    addRoundedRect(path, r.X, r.Y, r.Width, r.Height, radius);
    g.FillPath(&brush, &path);
}

void fillRoundAccent(Graphics& g, const RectF& r, float radius) {
    if (r.Width <= 0 || r.Height <= 0) return;
    const Theme& t = currentTheme();
    LinearGradientBrush brush(PointF(r.X, r.Y), PointF(r.X + r.Width, r.Y + r.Height), t.accentA,
                              t.accentB);
    GraphicsPath path;
    addRoundedRect(path, r.X, r.Y, r.Width, r.Height, radius);
    g.FillPath(&brush, &path);
}

void strokeRound(Graphics& g, const RectF& r, float radius, Color c, float width) {
    if (r.Width <= 0 || r.Height <= 0) return;
    Pen pen(c, width);
    pen.SetAlignment(PenAlignmentCenter);
    GraphicsPath path;
    addRoundedRect(path, r.X, r.Y, r.Width, r.Height, radius);
    g.DrawPath(&pen, &path);
}

void drawGlassPanel(Graphics& g, const RectF& r, float radius, bool hovered) {
    if (r.Width <= 0 || r.Height <= 0) return;
    const Theme& t = currentTheme();

    // Soft drop shadow beneath the panel.
    for (int i = 6; i >= 2; i -= 2) {
        float spread = static_cast<float>(i);
        RectF shadow(r.X - spread * 0.35f, r.Y - spread * 0.2f, r.Width + spread * 0.7f,
                     r.Height + spread * 0.9f);
        Color shadowColor(t.light ? 102 : 0, t.light ? 126 : 0, t.light ? 234 : 0,
                          static_cast<BYTE>(t.light ? 12 : 28));
        fillRound(g, shadow, radius + spread * 0.4f, shadowColor);
    }

    // When liquid glass is off the panel keeps a flat, solid fill.
    Color fill = hovered ? t.glassHover : t.glass;
    if (!glassEnabled()) {
        fill = t.light ? rgba(255, 255, 255, 226) : rgba(24, 22, 48, 226);
    }
    fillRound(g, r, radius, fill);
    strokeRound(g, r, radius, hovered ? t.borderStrong : t.border, 1.0f);

    // Inner top highlight for the glass sheen.
    if (!t.light) {
        RectF sheen(r.X + 1.0f, r.Y + 1.0f, r.Width - 2.0f, r.Height * 0.42f);
        LinearGradientBrush gloss(PointF(sheen.X, sheen.Y), PointF(sheen.X, sheen.Y + sheen.Height),
                                  rgba(255, 255, 255, 22), rgba(255, 255, 255, 0));
        GraphicsPath path;
        addRoundedRect(path, r.X, r.Y, r.Width, r.Height, radius);
        GraphicsState state = g.Save();
        g.SetClip(&path);
        g.FillRectangle(&gloss, sheen);
        g.Restore(state);
    }
}

// ============================================================
// Text
// ============================================================

static const WCHAR* kFontFamily = L"Segoe UI";

static Font* getFont(float size, bool bold, bool italic) {
    static std::map<int, std::unique_ptr<Font>> cache;
    int key = (static_cast<int>(size * 4) << 4) | (bold ? 2 : 0) | (italic ? 1 : 0);
    auto it = cache.find(key);
    if (it != cache.end()) return it->second.get();

    FontStyle style = FontStyleRegular;
    if (bold) style = FontStyleBold;
    if (italic) style = static_cast<FontStyle>(style | FontStyleItalic);

    FontFamily family(kFontFamily);
    auto font = std::make_unique<Font>(&family, size, style, UnitPixel);
    Font* raw = font.get();
    cache[key] = std::move(font);
    return raw;
}

void drawText(Graphics& g, const std::wstring& text, const RectF& r, float fontSize, bool bold,
              Color color, StringAlignment hAlign, StringAlignment vAlign, bool italic) {
    if (text.empty() || r.Width <= 0 || r.Height <= 0) return;
    Font* font = getFont(fontSize, bold, italic);
    SolidBrush brush(color);
    StringFormat format;
    format.SetAlignment(hAlign);
    format.SetLineAlignment(vAlign);
    format.SetTrimming(StringTrimmingEllipsisCharacter);
    format.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);
    g.DrawString(text.c_str(), -1, font, r, &format, &brush);
}

void drawTextAt(Graphics& g, const std::wstring& text, float x, float y, float fontSize, bool bold,
                Color color, bool italic) {
    if (text.empty()) return;
    Font* font = getFont(fontSize, bold, italic);
    SolidBrush brush(color);
    StringFormat format;
    format.SetAlignment(StringAlignmentNear);
    format.SetLineAlignment(StringAlignmentNear);
    format.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);
    RectF r(x, y, 4000.0f, fontSize * 1.6f);
    g.DrawString(text.c_str(), -1, font, r, &format, &brush);
}

SizeF measureText(Graphics& g, const std::wstring& text, float fontSize, bool bold, bool italic) {
    if (text.empty()) return SizeF(0.0f, 0.0f);
    Font* font = getFont(fontSize, bold, italic);
    RectF bounds;
    StringFormat format;
    format.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);
    g.MeasureString(text.c_str(), -1, font, PointF(0.0f, 0.0f), &format, &bounds);
    return SizeF(bounds.Width, bounds.Height);
}

// Flags and emoji live in a separate colour font on Windows.
void drawEmojiText(Graphics& g, const std::wstring& text, const RectF& r, float fontSize) {
    if (text.empty() || r.Width <= 0 || r.Height <= 0) return;

    // Pick the first family that is actually installed.
    static const WCHAR* candidates[] = {L"Segoe UI Emoji", L"Segoe UI Symbol", L"Segoe UI"};
    static int chosen = -1;
    if (chosen < 0) {
        for (int i = 0; i < 3; ++i) {
            FontFamily probe(candidates[i]);
            if (probe.IsAvailable()) {
                chosen = i;
                break;
            }
        }
        if (chosen < 0) chosen = 2;
    }

    FontFamily family(candidates[chosen]);
    Font font(&family, fontSize, FontStyleRegular, UnitPixel);
    SolidBrush brush(Color(255, 255, 255));
    StringFormat format;
    format.SetAlignment(StringAlignment::StringAlignmentNear);
    format.SetLineAlignment(StringAlignment::StringAlignmentCenter);
    format.SetFormatFlags(StringFormatFlagsNoWrap | StringFormatFlagsNoClip);
    g.DrawString(text.c_str(), -1, &font, r, &format, &brush);
}

// ============================================================
// Icons
// ============================================================

// Maps a normalized 24x24 space onto the target rect.
struct IconSpace {
    float sx;
    float sy;
    float ox;
    float oy;

    PointF map(float x, float y) const { return PointF(ox + x * sx, oy + y * sy); }
};

static IconSpace iconSpace(const RectF& r) {
    float size = std::min(r.Width, r.Height);
    IconSpace s;
    s.sx = size / 24.0f;
    s.sy = size / 24.0f;
    s.ox = r.X + (r.Width - size) / 2.0f;
    s.oy = r.Y + (r.Height - size) / 2.0f;
    return s;
}

static void penPath(Graphics& g, GraphicsPath& path, IconSpace s, Color c, float width) {
    Pen pen(c, width);
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    pen.SetLineJoin(LineJoinRound);
    g.DrawPath(&pen, &path);
}

// GDI+ on MinGW has no AddArc overload that takes a point, so build the
// rectangle first and use the six-argument form.
static void addArc(GraphicsPath& path, IconSpace s, float x, float y, float w, float h,
                   float start, float sweep) {
    path.AddArc(s.map(x, y).X, s.map(x, y).Y, w * s.sx, h * s.sy, start, sweep);
}

// Builds a colour with an explicit alpha channel.
static Color withAlpha(Color c, BYTE alpha) {
    return Color::MakeARGB(alpha, c.GetR(), c.GetG(), c.GetB());
}

void drawIcon(Graphics& g, Icon icon, const RectF& r, Color color, float strokeWidth) {
    if (icon == Icon::None || r.Width <= 0 || r.Height <= 0) return;
    IconSpace s = iconSpace(r);
    GraphicsPath path;

    switch (icon) {
        case Icon::Folder:
            path.AddLine(s.map(2, 6), s.map(9, 6));
            path.AddLine(s.map(9, 6), s.map(11, 9));
            path.AddLine(s.map(11, 9), s.map(22, 9));
            path.AddLine(s.map(22, 9), s.map(22, 19));
            path.AddLine(s.map(22, 19), s.map(2, 19));
            path.AddLine(s.map(2, 19), s.map(2, 6));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::FolderPlus:
            path.AddLine(s.map(2, 6), s.map(9, 6));
            path.AddLine(s.map(9, 6), s.map(11, 9));
            path.AddLine(s.map(11, 9), s.map(22, 9));
            path.AddLine(s.map(22, 9), s.map(22, 19));
            path.AddLine(s.map(22, 19), s.map(2, 19));
            path.AddLine(s.map(2, 19), s.map(2, 6));
            penPath(g, path, s, color, strokeWidth);
            {
                GraphicsPath plus;
                plus.AddLine(s.map(12, 12), s.map(12, 17));
                plus.AddLine(s.map(9.5, 14.5), s.map(14.5, 14.5));
                penPath(g, plus, s, color, strokeWidth);
            }
            break;

        case Icon::Star:
        case Icon::StarFilled: {
            PointF pts[10] = {
                s.map(12, 2),  s.map(15.09f, 8.26f), s.map(22, 9.27f), s.map(17, 14.14f),
                s.map(18.18f, 21.02f), s.map(12, 17.77f), s.map(5.82f, 21.02f), s.map(7, 14.14f),
                s.map(2, 9.27f),  s.map(8.91f, 8.26f)};
            path.AddPolygon(pts, 10);
            if (icon == Icon::StarFilled) {
                SolidBrush brush(color);
                g.FillPath(&brush, &path);
            } else {
                penPath(g, path, s, color, strokeWidth);
            }
            break;
        }

        case Icon::Clock:
            path.AddEllipse(s.map(2, 2).X, s.map(2, 2).Y, 20.0f * s.sx, 20.0f * s.sy);
            path.AddLine(s.map(12, 6), s.map(12, 12));
            path.AddLine(s.map(12, 12), s.map(16, 14));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Trash:
            path.AddLine(s.map(3, 6), s.map(21, 6));
            path.AddLine(s.map(19, 6), s.map(19, 20));
            path.AddLine(s.map(19, 20), s.map(5, 20));
            path.AddLine(s.map(5, 20), s.map(5, 6));
            path.AddLine(s.map(9, 6), s.map(9, 3));
            path.AddLine(s.map(9, 3), s.map(15, 3));
            path.AddLine(s.map(15, 3), s.map(15, 6));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Gear: {
            // Simplified gear: outer ring plus spokes.
            path.AddEllipse(s.map(2, 2).X, s.map(2, 2).Y, 20.0f * s.sx, 20.0f * s.sy);
            path.AddEllipse(s.map(9, 9).X, s.map(9, 9).Y, 6.0f * s.sx, 6.0f * s.sy);
            for (int i = 0; i < 8; ++i) {
                double a = i * M_PI / 4.0;
                float c = static_cast<float>(std::cos(a));
                float sn = static_cast<float>(std::sin(a));
                path.AddLine(s.map(12 + c * 7.0f, 12 + sn * 7.0f),
                             s.map(12 + c * 11.0f, 12 + sn * 11.0f));
            }
            penPath(g, path, s, color, strokeWidth * 0.9f);
            break;
        }

        case Icon::Info:
            path.AddEllipse(s.map(2, 2).X, s.map(2, 2).Y, 20.0f * s.sx, 20.0f * s.sy);
            path.AddLine(s.map(12, 11), s.map(12, 16));
            path.AddLine(s.map(12, 8), s.map(12.01f, 8));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Globe:
            path.AddEllipse(s.map(2, 2).X, s.map(2, 2).Y, 20.0f * s.sx, 20.0f * s.sy);
            path.AddLine(s.map(2, 12), s.map(22, 12));
            addArc(path, s, 7, 2, 10, 20, 90, 180);
            addArc(path, s, 7, 2, 10, 20, 270, 180);
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Search:
            path.AddEllipse(s.map(3, 3).X, s.map(3, 3).Y, 14.0f * s.sx, 14.0f * s.sy);
            path.AddLine(s.map(15.5f, 15.5f), s.map(21, 21));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Back:
            path.AddLine(s.map(15, 5), s.map(8, 12));
            path.AddLine(s.map(8, 12), s.map(15, 19));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Forward:
            path.AddLine(s.map(9, 5), s.map(16, 12));
            path.AddLine(s.map(16, 12), s.map(9, 19));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Refresh:
            addArc(path, s, 4, 4, 16, 16, 60, 280);
            {
                GraphicsPath head;
                head.AddLine(s.map(19, 4), s.map(20, 9));
                head.AddLine(s.map(20, 9), s.map(15, 8));
                penPath(g, head, s, color, strokeWidth);
            }
            break;

        case Icon::Plus:
            path.AddLine(s.map(12, 5), s.map(12, 19));
            path.AddLine(s.map(5, 12), s.map(19, 12));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Download:
            path.AddLine(s.map(12, 3), s.map(12, 15));
            path.AddLine(s.map(7, 10), s.map(12, 15));
            path.AddLine(s.map(17, 10), s.map(12, 15));
            path.AddLine(s.map(4, 20), s.map(20, 20));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Upload:
            path.AddLine(s.map(12, 15), s.map(12, 3));
            path.AddLine(s.map(7, 8), s.map(12, 3));
            path.AddLine(s.map(17, 8), s.map(12, 3));
            path.AddLine(s.map(4, 20), s.map(20, 20));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Eye:
            addArc(path, s, 2, 7, 20, 16, 200, 140);
            addArc(path, s, 2, 1, 20, 16, 20, 140);
            penPath(g, path, s, color, strokeWidth);
            path.Reset();
            path.AddEllipse(s.map(9, 9).X, s.map(9, 9).Y, 6.0f * s.sx, 6.0f * s.sy);
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::More:
            {
                SolidBrush brush(color);
                float r = 1.6f;
                g.FillEllipse(&brush, s.map(5, 12).X - r, s.map(5, 12).Y - r, r * 2, r * 2);
                g.FillEllipse(&brush, s.map(12, 12).X - r, s.map(12, 12).Y - r, r * 2, r * 2);
                g.FillEllipse(&brush, s.map(19, 12).X - r, s.map(19, 12).Y - r, r * 2, r * 2);
            }
            break;

        case Icon::Home:
            path.AddLine(s.map(3, 11), s.map(12, 3));
            path.AddLine(s.map(12, 3), s.map(21, 11));
            path.AddLine(s.map(6, 10), s.map(6, 21));
            path.AddLine(s.map(6, 21), s.map(18, 21));
            path.AddLine(s.map(18, 21), s.map(18, 10));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Check:
            path.AddLine(s.map(4, 13), s.map(9, 18));
            path.AddLine(s.map(9, 18), s.map(20, 6));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::User:
            addArc(path, s, 4, 2, 16, 16, 180, 180);
            addArc(path, s, 2, 11, 20, 11, 180, 180);
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Music:
            path.AddLine(s.map(9, 18), s.map(9, 5));
            path.AddLine(s.map(9, 5), s.map(21, 3));
            path.AddLine(s.map(21, 3), s.map(21, 16));
            path.AddLine(s.map(21, 16), s.map(9, 18));
            {
                SolidBrush brush(color);
                g.FillEllipse(&brush, s.map(4, 15).X, s.map(4, 15).Y, s.sx * 6.0f, s.sy * 6.0f);
                g.FillEllipse(&brush, s.map(16, 13).X, s.map(16, 13).Y, s.sx * 6.0f, s.sy * 6.0f);
            }
            break;

        case Icon::Video:
            path.AddLine(s.map(3, 6), s.map(15, 6));
            path.AddLine(s.map(15, 6), s.map(15, 18));
            path.AddLine(s.map(15, 18), s.map(3, 18));
            path.AddLine(s.map(3, 18), s.map(3, 6));
            path.AddLine(s.map(15, 10), s.map(21, 7));
            path.AddLine(s.map(21, 7), s.map(21, 17));
            path.AddLine(s.map(21, 17), s.map(15, 14));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Document:
            path.AddLine(s.map(6, 3), s.map(14, 3));
            path.AddLine(s.map(14, 3), s.map(19, 8));
            path.AddLine(s.map(19, 8), s.map(19, 21));
            path.AddLine(s.map(19, 21), s.map(6, 21));
            path.AddLine(s.map(6, 21), s.map(6, 3));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Image:
            path.AddLine(s.map(3, 5), s.map(3, 19));
            path.AddLine(s.map(3, 19), s.map(21, 19));
            path.AddLine(s.map(21, 19), s.map(21, 5));
            path.AddLine(s.map(21, 5), s.map(3, 5));
            {
                SolidBrush brush(color);
                g.FillEllipse(&brush, s.map(7, 8).X, s.map(7, 8).Y, 3.0f * s.sx, 3.0f * s.sy);
            }
            path.Reset();
            path.AddLine(s.map(3, 17), s.map(9, 11));
            path.AddLine(s.map(9, 11), s.map(14, 16));
            path.AddLine(s.map(14, 16), s.map(18, 12));
            path.AddLine(s.map(18, 12), s.map(21, 16));
            penPath(g, path, s, color, strokeWidth);
            break;

        case Icon::Archive:
            path.AddLine(s.map(3, 5), s.map(21, 5));
            path.AddLine(s.map(21, 5), s.map(21, 21));
            path.AddLine(s.map(21, 21), s.map(3, 21));
            path.AddLine(s.map(3, 21), s.map(3, 5));
            path.AddLine(s.map(10, 10), s.map(14, 10));
            path.AddLine(s.map(14, 10), s.map(14, 14));
            path.AddLine(s.map(14, 14), s.map(10, 14));
            path.AddLine(s.map(10, 14), s.map(10, 10));
            penPath(g, path, s, color, strokeWidth);
            break;

        default:
            break;
    }
}

// Draws a small national flag as coloured shapes, so the picker shows real
// flags rather than relying on emoji font shaping.
void drawFlag(Graphics& g, const std::string& code, const RectF& r) {
    if (r.Width <= 0 || r.Height <= 0) return;

    auto band = [&](float x, float y, float bw, float bh, Color c) {
        SolidBrush brush(c);
        g.FillRectangle(&brush, x, y, bw, bh);
    };

    float w = r.Width;
    float h = r.Height;

    if (code == "en") {
        band(r.X, r.Y, w, h, Color(0, 33, 105));
        band(r.X, r.Y, w, h / 4.0f, Color(255, 255, 255));
        band(r.X, r.Y + h * 0.75f, w, h / 4.0f, Color(255, 255, 255));
        band(r.X, r.Y, w / 4.0f, h, Color(255, 255, 255));
        band(r.X + w * 0.75f, r.Y, w / 4.0f, h, Color(255, 255, 255));
        band(r.X, r.Y, w, h / 6.0f, Color(200, 16, 46));
        band(r.X, r.Y + h * 0.833f, w, h / 6.0f, Color(200, 16, 46));
        band(r.X, r.Y, w / 6.0f, h, Color(200, 16, 46));
        band(r.X + w * 0.833f, r.Y, w / 6.0f, h, Color(200, 16, 46));
    } else if (code == "es") {
        band(r.X, r.Y, w, h / 3.0f, Color(198, 11, 30));
        band(r.X, r.Y + h / 3.0f, w, h / 3.0f, Color(255, 196, 0));
        band(r.X, r.Y + h * 2.0f / 3.0f, w, h / 3.0f, Color(198, 11, 30));
    } else if (code == "fr") {
        band(r.X, r.Y, w / 3.0f, h, Color(0, 85, 164));
        band(r.X + w / 3.0f, r.Y, w / 3.0f, h, Color(255, 255, 255));
        band(r.X + w * 2.0f / 3.0f, r.Y, w / 3.0f, h, Color(239, 65, 53));
    } else if (code == "de") {
        band(r.X, r.Y, w, h / 3.0f, Color(0, 0, 0));
        band(r.X, r.Y + h / 3.0f, w, h / 3.0f, Color(221, 0, 0));
        band(r.X, r.Y + h * 2.0f / 3.0f, w, h / 3.0f, Color(255, 206, 0));
    } else if (code == "pt") {
        band(r.X, r.Y, w * 0.4f, h, Color(0, 102, 71));
        band(r.X + w * 0.4f, r.Y, w * 0.6f, h, Color(255, 0, 0));
        SolidBrush gold(Color(255, 204, 0));
        g.FillEllipse(&gold, r.X + w * 0.4f - h * 0.2f, r.Y + h * 0.4f, h * 0.4f, h * 0.4f);
    } else if (code == "hi") {
        band(r.X, r.Y, w, h / 3.0f, Color(255, 136, 0));
        band(r.X, r.Y + h / 3.0f, w, h / 3.0f, Color(255, 255, 255));
        band(r.X, r.Y + h * 2.0f / 3.0f, w, h / 3.0f, Color(20, 138, 87));
        SolidBrush navy(Color(0, 0, 139));
        g.FillEllipse(&navy, r.X + w * 0.5f - h * 0.12f, r.Y + h / 3.0f - h * 0.12f, h * 0.24f, h * 0.24f);
    } else if (code == "ar") {
        band(r.X, r.Y, w, h / 3.0f, Color(0, 122, 61));
        band(r.X, r.Y + h / 3.0f, w, h / 3.0f, Color(255, 255, 255));
        band(r.X, r.Y + h * 2.0f / 3.0f, w, h / 3.0f, Color(0, 0, 0));
    } else if (code == "zh") {
        band(r.X, r.Y, w, h, Color(222, 41, 16));
        SolidBrush gold(Color(255, 222, 0));
        PointF pts[10];
        for (int i = 0; i < 10; ++i) {
            double angle = -M_PI / 2.0 + i * M_PI / 5.0;
            float radius = (i % 2 == 0) ? h * 0.28f : h * 0.12f;
            pts[i] = PointF(static_cast<float>(r.X + w * 0.22 + std::cos(angle) * radius),
                            static_cast<float>(r.Y + h * 0.3 + std::sin(angle) * radius));
        }
        g.FillPolygon(&gold, pts, 10);
    } else {
        band(r.X, r.Y, w, h, Color(102, 126, 234));
    }
}

void drawLogoMark(Graphics& g, const RectF& r) {
    if (r.Width <= 0 || r.Height <= 0) return;
    float size = std::min(r.Width, r.Height);
    RectF box(r.X + (r.Width - size) / 2.0f, r.Y + (r.Height - size) / 2.0f, size, size);
    float radius = size * 0.28f;

    LinearGradientBrush grad(PointF(box.X, box.Y), PointF(box.X + size, box.Y + size),
                             currentTheme().accentA, currentTheme().accentB);
    GraphicsPath path;
    addRoundedRect(path, box.X, box.Y, box.Width, box.Height, radius);
    g.FillPath(&grad, &path);

    // Work in a 48x48 grid so the glyph scales with the mark.
    IconSpace s;
    s.sx = size / 48.0f;
    s.sy = size / 48.0f;
    s.ox = box.X;
    s.oy = box.Y;

    // The white cloud: a rounded base plus two lobes.
    SolidBrush white(Color(255, 255, 255));
    GraphicsPath cloud;
    addArc(cloud, s, 13, 17, 16, 16, 0, 360);
    addArc(cloud, s, 24, 13, 14, 14, 0, 360);
    addArc(cloud, s, 30, 19, 11, 11, 0, 360);
    GraphicsPath base;
    addRoundedRect(base, s.map(15, 24).X, s.map(15, 24).Y, 24.0f * s.sx, 9.0f * s.sy, 4.5f * s.sx);
    g.FillPath(&white, &cloud);
    g.FillPath(&white, &base);

    // The accent download arrow sitting on the cloud.
    GraphicsPath arrow;
    arrow.AddLine(s.map(24, 22), s.map(24, 33));
    arrow.AddLine(s.map(24, 22), s.map(19.5f, 27));
    arrow.AddLine(s.map(24, 22), s.map(28.5f, 27));
    Pen pen(currentTheme().accentB, std::max(1.8f, size * 0.085f));
    pen.SetStartCap(LineCapRound);
    pen.SetEndCap(LineCapRound);
    pen.SetLineJoin(LineJoinRound);
    g.DrawPath(&pen, &arrow);
}

static bool g_bgDeferred = false;
void setBackgroundDeferred(bool deferred) { g_bgDeferred = deferred; }

void drawBackground(Graphics& g, const RectF& area, const Theme& t) {
    if (area.Width <= 0 || area.Height <= 0) return;

    // The background is expensive to build (several large gradients and
    // dozens of soft glows), so it is cached and only rebuilt when the
    // size or the theme changes.
    static Bitmap* cached = nullptr;
    static float cachedWidth = 0.0f;
    static float cachedHeight = 0.0f;
    static std::string cachedTheme;

    bool rebuild = cached == nullptr || cachedWidth != area.Width || cachedHeight != area.Height ||
                   cachedTheme != t.name;
    if (g_bgDeferred && cached != nullptr && cachedTheme == t.name) rebuild = false;

    if (rebuild) {
        delete cached;
        cached = new Bitmap(static_cast<INT>(area.Width), static_cast<INT>(area.Height), PixelFormat32bppRGB);
        Graphics back(cached);
        back.SetSmoothingMode(SmoothingModeAntiAlias);

        LinearGradientBrush base(PointF(area.X, area.Y),
                                 PointF(area.X + area.Width * 0.35f, area.Y + area.Height), t.bgTop, t.bgMid);
        back.FillRectangle(&base, area);

        LinearGradientBrush lower(PointF(area.X, area.Y + area.Height * 0.45f),
                                  PointF(area.X + area.Width * 0.8f, area.Y + area.Height), t.bgMid,
                                  t.bgBottom);
        back.FillRectangle(&lower, area);

        // Soft accent glows, matching the web app's radial light sources.
        for (int i = 0; i < 5; ++i) {
            float spread = 1.0f - static_cast<float>(i) * 0.16f;
            Color glow = withAlpha(t.accentSolid, t.light ? 6 : 12);
            float cx = area.X + area.Width * (0.10f + 0.07f * i);
            float cy = area.Y + area.Height * (0.20f + 0.11f * i);
            float radius = area.Width * 0.30f * spread;
            for (int s = 0; s < 8; ++s) {
                float k = 1.0f - static_cast<float>(s) / 8.0f;
                Color c = withAlpha(glow, static_cast<BYTE>(glow.GetAlpha() * k * 0.9f));
                SolidBrush brush(c);
                float r = radius * (0.55f + 0.45f * (1.0f - k));
                back.FillEllipse(&brush, cx - r, cy - r, r * 2.0f, r * 2.0f);
            }
        }

        cachedWidth = area.Width;
        cachedHeight = area.Height;
        cachedTheme = t.name;
    }

    InterpolationMode previousMode = g.GetInterpolationMode();
    g.SetInterpolationMode(InterpolationModeNearestNeighbor);
    g.DrawImage(cached, RectF(area.X, area.Y, area.Width, area.Height), 0.0f, 0.0f,
                static_cast<float>(cached->GetWidth()), static_cast<float>(cached->GetHeight()), UnitPixel);
    g.SetInterpolationMode(previousMode);
}

} // namespace neb
