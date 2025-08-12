// examples/pong.cpp
#include "../gcp.h"
#include <algorithm>
#include <cmath>

struct AABB { float x, y, w, h; }; // centered box

static bool intersects(const AABB& a, const AABB& b) {
    return std::abs(a.x - b.x) * 2.f < (a.w + b.w) &&
           std::abs(a.y - b.y) * 2.f < (a.h + b.h);
}

// draw N pips starting from (sx,sy) horizontally
static void drawPips(float sx, float sy, int count, bool rightSide) {
    const float size = 14.f;
    const float gap  = 6.f;
    for (int i = 0; i < count; ++i) {
        float x = sx + (rightSide ? -(size + gap) * i : (size + gap) * i);
        gcp::drawRect(x, sy, size, size, sf::Color(180,220,180));
    }
}

int main() {
    using namespace gcp;

    // window
    windowHint(GCP_TITLE, std::string("GCP Pong (no text)"));
    windowHint(GCP_SIZE_W, 1024);
    windowHint(GCP_SIZE_H, 720);
    windowHint(GCP_VSYNC, true);
    createWindow();
    clearColor(sf::Color(18,18,22));

    const float W = 1024, H = 720;

    // paddles
    AABB left  {  40, H/2.f, 20, 120 };
    AABB right { W-40, H/2.f, 20, 120 };
    const float paddleSpeed = 520.f;

    // ball
    float br = 12.f;
    float bx = W/2.f, by = H/2.f, bvx = 0.f, bvy = 0.f; // start paused

    auto serve = [&](int dir){ // +1 → right, -1 → left
        bx = W/2.f; by = H/2.f;
        bvx = 320.f * (float)dir;
        bvy = ((rand()%2) ? 1.f : -1.f) * 180.f;
    };

    // scores
    int scoreL = 0, scoreR = 0; // we’ll show as pips

    while (isOpen()) {
        bool quit=false; pollEvents(quit); if (quit) break;
        float dt = std::min(deltaTime(), 0.02f); // clamp spikes

        // input
        if (keyDown(sf::Keyboard::W)) left.y  -= paddleSpeed * dt;
        if (keyDown(sf::Keyboard::S)) left.y  += paddleSpeed * dt;
        if (keyDown(sf::Keyboard::Up))   right.y -= paddleSpeed * dt;
        if (keyDown(sf::Keyboard::Down)) right.y += paddleSpeed * dt;

        left.y  = std::clamp(left.y,  left.h*0.5f,  H - left.h*0.5f);
        right.y = std::clamp(right.y, right.h*0.5f, H - right.h*0.5f);

        // serve/reset
        if (keyDown(sf::Keyboard::Space) && bvx == 0.f && bvy == 0.f) {
            serve( (rand()%2)*2 - 1 );
        }

        // physics
        bx += bvx * dt;
        by += bvy * dt;

        if (by - br < 0)      { by = br;      bvy = -bvy; }
        if (by + br > H)      { by = H - br;  bvy = -bvy; }

        // collisions
        AABB ball{ bx, by, br*2.f, br*2.f };

        auto bounceFromPaddle = [&](const AABB& p, int dir){
            if (!intersects(ball, p)) return;
            bx = p.x + (p.w*0.5f + br + 1.f) * (float)dir;
            bvx = std::abs(bvx) * (float)dir;
            float dy = (by - p.y) / (p.h*0.5f); // -1..1
            bvy = std::clamp(bvy + dy * 240.f, -520.f, 520.f);
            bvx *= 1.03f; bvy *= 1.02f;
        };

        bounceFromPaddle(left,  +1);
        bounceFromPaddle(right, -1);

        // goals → pause until SPACE
        if (bx < -60)  { scoreR = std::min(scoreR + 1, 10); bvx=bvy=0.f; }
        if (bx > W+60) { scoreL = std::min(scoreL + 1, 10); bvx=bvy=0.f; }

        // draw
        beginFrame();

        // mid dashed line
        for (float y=20; y<H; y+=40)
            drawRect(W*0.5f, y, 6, 16, sf::Color(80,80,95));

        // paddles
        drawRect(left.x,  left.y,  left.w,  left.h,  sf::Color(220,220,220));
        drawRect(right.x, right.y, right.w, right.h, sf::Color(220,220,220));

        // ball
        drawCircle(bx, by, br, sf::Color(250,250,250));

        // scores as pips (max 10 shown)
        drawPips( 80.f, 40.f, scoreL, /*rightSide=*/false);           // left score → left side
        drawPips(W-80.f, 40.f, scoreR, /*rightSide=*/true);           // right score → right side

        // tiny serve hint made of rectangles (no text)
        if (bvx == 0.f && bvy == 0.f) {
            // three small bars near bottom center
            float cx = W*0.5f, y = H - 36.f;
            drawRect(cx - 40, y, 20, 6, sf::Color(150,170,190));
            drawRect(cx,       y, 20, 6, sf::Color(150,170,190));
            drawRect(cx + 40,  y, 20, 6, sf::Color(150,170,190));
            // small “space bar” rectangle to imply press space
            drawRect(cx, y+18, 120, 12, sf::Color(110,130,150));
        }

        endFrame();
    }

    destroyContext();
    return 0;
}
