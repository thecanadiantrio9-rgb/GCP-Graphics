#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <cmath>

namespace gcp {

// --- hint keys ---
enum HintKey {
    GCP_RESIZABLE,
    GCP_VSYNC,
    GCP_FPS_LIMIT,
    GCP_MSAA,
    GCP_TITLE,
    GCP_SIZE_W,
    GCP_SIZE_H
};

using HintVal = std::variant<bool, int, std::string>;
inline sf::Color s_clearColor = sf::Color(18, 18, 20); // default

struct Config {
    bool resizable = true;
    bool vsync = true;
    int fpsLimit = 0;
    int msaa = 0;
    std::string title = "gcp window";
    int w = 1024, h = 720;
};

struct State {
    Config cfg;
    sf::RenderWindow window;
    sf::View view;
    bool initialized = false;
    sf::Clock clock;
};

inline State* s_state = nullptr;

// ===== API =====
inline void createContext() {
    if (!s_state) s_state = new State();
}

inline void destroyContext() {
    if (s_state) {
        if (s_state->initialized) s_state->window.close();
        delete s_state;
        s_state = nullptr;
    }
}

inline void windowHint(HintKey key, const HintVal& val) {
    if (!s_state) createContext();
    auto& c = s_state->cfg;
    if (key == GCP_RESIZABLE)     c.resizable = std::get<bool>(val);
    else if (key == GCP_VSYNC)    c.vsync = std::get<bool>(val);
    else if (key == GCP_FPS_LIMIT)c.fpsLimit = std::get<int>(val);
    else if (key == GCP_MSAA)     c.msaa = std::get<int>(val);
    else if (key == GCP_TITLE)    c.title = std::get<std::string>(val);
    else if (key == GCP_SIZE_W)   c.w = std::get<int>(val);
    else if (key == GCP_SIZE_H)   c.h = std::get<int>(val);
}

inline bool createWindow() {
    if (!s_state) createContext();
    sf::ContextSettings settings;
    settings.antialiasingLevel = s_state->cfg.msaa;

    auto style = s_state->cfg.resizable ? sf::Style::Default : (sf::Style::Titlebar | sf::Style::Close);
    s_state->window.create(sf::VideoMode(s_state->cfg.w, s_state->cfg.h), s_state->cfg.title, style, settings);

    s_state->window.setVerticalSyncEnabled(s_state->cfg.vsync);
    if (s_state->cfg.fpsLimit > 0) s_state->window.setFramerateLimit(s_state->cfg.fpsLimit);

    s_state->view = s_state->window.getDefaultView();
    s_state->window.setView(s_state->view);

    s_state->initialized = true;
    return true;
}

inline bool isOpen() {
    return s_state && s_state->window.isOpen();
}

inline void clearColor(const sf::Color& c) {
    s_clearColor = c;
}

inline void pollEvents(bool& shouldClose) {
    sf::Event e;
    while (s_state->window.pollEvent(e)) {
        if (e.type == sf::Event::Closed) shouldClose = true;
        if (e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Escape)
            shouldClose = true;
    }
}

inline void beginFrame() {
    s_state->window.clear(s_clearColor);
}


inline void endFrame() {
    s_state->window.display();
}

inline float deltaTime() {
    return s_state->clock.restart().asSeconds();
}

inline bool keyDown(sf::Keyboard::Key k) {
    return sf::Keyboard::isKeyPressed(k);
}

// ===== Drawing =====
inline void drawRect(float x, float y, float w, float h, sf::Color color = sf::Color::White, float rotationDeg = 0.0f) {
    sf::RectangleShape r;
    r.setSize({w, h});
    r.setOrigin(w*0.5f, h*0.5f);
    r.setPosition(x, y);
    r.setRotation(rotationDeg);
    r.setFillColor(color);
    s_state->window.draw(r);
}

inline void drawCircle(float x, float y, float r, sf::Color color = sf::Color::White, float rotationDeg = 0.0f) {
    sf::CircleShape c;
    c.setRadius(r);
    c.setOrigin(r, r);
    c.setPosition(x, y);
    c.setRotation(rotationDeg);
    c.setFillColor(color);
    s_state->window.draw(c);
}

inline void drawLine(float x1, float y1, float x2, float y2,
                     float thickness = 1.0f,
                     sf::Color color = sf::Color::White) {
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = std::sqrt(dx*dx + dy*dy);
    if (len <= 0.0001f) return;

    float angleDeg = std::atan2(dy, dx) * 180.0f / 3.14159265f;

    sf::RectangleShape lineShape({len, thickness});
    lineShape.setOrigin(0.f, thickness * 0.5f); // anchor at start, centered on thickness
    lineShape.setPosition(x1, y1);
    lineShape.setRotation(angleDeg);
    lineShape.setFillColor(color);

    s_state->window.draw(lineShape);
}
// == Fonts ==
inline sf::Font& loadFont(const std::string& path) {
    static std::unordered_map<std::string, sf::Font> cache;
    if (auto it = cache.find(path); it != cache.end()) return it->second;
    auto& f = cache[path];
    f.loadFromFile(path); // assumes path is valid; add error handling if you like
    return f;
}
enum class TextAlign { Left, Center, Right };
inline sf::Vector2f measureText(const std::string& str, sf::Font& font, unsigned size) {
    sf::Text t(str, font, size);
    auto b = t.getLocalBounds();
    return { b.width, b.height };
}
inline void drawText(const std::string& str,
                     float x, float y,
                     unsigned size,
                     sf::Color color = sf::Color::White,
                     TextAlign align = TextAlign::Left,
                     float outlineThickness = 0.f,
                     sf::Color outlineColor = sf::Color::Black,
                     sf::Font* fontPtr = nullptr,
                     const std::string& fontPathIfNull = "Roboto-Regular.ttf")
{
    // Pick font: provided pointer or load from path
    sf::Font& font = fontPtr ? *fontPtr : loadFont(fontPathIfNull);

    sf::Text t(str, font, size);
    t.setFillColor(color);

    if (outlineThickness > 0.f) {
        t.setOutlineThickness(outlineThickness);
        t.setOutlineColor(outlineColor);
    }

    // Align by adjusting origin using local bounds
    auto b = t.getLocalBounds(); 
    // Note: b.left/b.top can be non-zero (glyph padding); include them for correct centering
    sf::Vector2f origin(b.left, b.top);
    if (align == TextAlign::Center) origin.x += b.width * 0.5f;
    else if (align == TextAlign::Right) origin.x += b.width;

    // vertical: make (x,y) the visual centerline (nicer for HUDs); switch to top if you prefer
    origin.y += b.height * 0.5f;

    t.setOrigin(origin);
    t.setPosition(x, y);

    s_state->window.draw(t);
}

// ===== Textures & Sprites =====
inline sf::Texture& loadTexture(const std::string& path) {
    static std::unordered_map<std::string, sf::Texture> cache;
    auto it = cache.find(path);
    if (it != cache.end()) return it->second;
    auto& tex = cache[path];
    tex.loadFromFile(path);
    tex.setSmooth(true);
    return tex;
}

inline void drawSprite(sf::Texture& tex, float x, float y, float scale=1.f, float rotDeg=0.f) {
    sf::Sprite s(tex);
    s.setOrigin(tex.getSize().x*0.5f, tex.getSize().y*0.5f);
    s.setPosition(x, y);
    s.setRotation(rotDeg);
    s.setScale(scale, scale);
    s_state->window.draw(s);
}

} // namespace gcp
