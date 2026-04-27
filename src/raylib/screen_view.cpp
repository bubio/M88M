#include "screen_view.h"
#include <cstring>
#include <algorithm>

RaylibDraw::RaylibDraw() : width(0), height(0), dirty(false) {
    memset(raylib_palette, 0, sizeof(raylib_palette));
    texture = {0};
}

RaylibDraw::~RaylibDraw() {
    Cleanup();
}

bool RaylibDraw::Init(uint w, uint h, uint bpp) {
    width = w;
    height = h;
    buffer.assign(width * height, 0);
    for (int i = 0; i < 256; i++) {
        raylib_palette[i] = {(unsigned char)i, (unsigned char)i, (unsigned char)i, 255};
    }
    dirty = true;
    return true;
}

bool RaylibDraw::Cleanup() {
    if (IsTextureValid(texture)) {
        UnloadTexture(texture);
        texture = {0};
    }
    return true;
}

bool RaylibDraw::Lock(uint8** pimage, int* pbpl) {
    mutex.lock();
    *pimage = buffer.data();
    *pbpl = (int)width;
    return true;
}

bool RaylibDraw::Unlock() {
    dirty = true;
    mutex.unlock();
    return true;
}

uint RaylibDraw::GetStatus() {
    return (uint)(readytodraw | shouldrefresh | flippable);
}

void RaylibDraw::Resize(uint w, uint h) {}

void RaylibDraw::DrawScreen(const Region& region) {
    dirty = true;
}

void RaylibDraw::SetPalette(uint index, uint nents, const Palette* pal) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    for (uint i = 0; i < nents && (index + i) < 256; i++) {
        auto s3to8 = [](uint8 v) -> uint8 { return (v & 0x07) * 255 / 7; };
        raylib_palette[index + i] = { s3to8(pal[i].red), s3to8(pal[i].green), s3to8(pal[i].blue), 255 };
    }
    dirty = true;
}

void RaylibDraw::Flip() {
    dirty = true;
}

bool RaylibDraw::SetFlipMode(bool flip) { return true; }

void RaylibDraw::Render() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    
    if (!IsTextureValid(texture) && width > 0 && height > 0) {
        Image img = GenImageColor((int)width, (int)height, BLACK);
        texture = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    }

    if (dirty && IsTextureValid(texture)) {
        static std::vector<Color> rgba;
        if (rgba.size() != width * height) rgba.resize(width * height);
        for (size_t i = 0; i < width * height; ++i) {
            rgba[i] = raylib_palette[buffer[i]];
        }
        UpdateTexture(texture, rgba.data());
        dirty = false;
    }

    if (IsTextureValid(texture)) {
        float screenW = (float)GetScreenWidth();
        float screenH = (float)GetScreenHeight();
        // Emulation area is the whole window minus the 24px status bar at the bottom
        Rectangle dest = { 0, 0, screenW, screenH - 24 };
        DrawTexturePro(texture, { 0, 0, (float)width, (float)height }, dest, { 0, 0 }, 0, WHITE);
    }
}
