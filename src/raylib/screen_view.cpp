#include "screen_view.h"
#include <cstring>
#include <algorithm>

RaylibDraw::RaylibDraw() : width(0), height(0), dirty(false) {
    memset(palette, 0, sizeof(palette));
    memset(raylib_palette, 0, sizeof(raylib_palette));
    texture = {0};
    update_region.Reset();
}

RaylibDraw::~RaylibDraw() {
    Cleanup();
}

bool RaylibDraw::Init(uint w, uint h, uint bpp) {
    width = w;
    height = h;
    buffer.assign(width * height, 0);
    
    // Initial palette (grayscale)
    for (int i = 0; i < 256; i++) {
        raylib_palette[i] = {(unsigned char)i, (unsigned char)i, (unsigned char)i, 255};
    }

    return true;
}

bool RaylibDraw::Cleanup() {
    if (texture.id != 0) {
        UnloadTexture(texture);
        texture.id = 0;
    }
    return true;
}

bool RaylibDraw::Lock(uint8** pimage, int* pbpl) {
    mutex.lock();
    *pimage = buffer.data();
    *pbpl = width;
    return true;
}

bool RaylibDraw::Unlock() {
    mutex.unlock();
    return true;
}

uint RaylibDraw::GetStatus() {
    return readytodraw;
}

void RaylibDraw::Resize(uint w, uint h) {
}

void RaylibDraw::DrawScreen(const Region& region) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!update_region.Valid()) {
        update_region = region;
    } else {
        update_region.Update(region.left, region.top, region.right, region.bottom);
    }
    dirty = true;
}

void RaylibDraw::SetPalette(uint index, uint nents, const Palette* pal) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    for (uint i = 0; i < nents && (index + i) < 256; i++) {
        palette[index + i] = pal[i];
        raylib_palette[index + i] = {pal[i].red, pal[i].green, pal[i].blue, 255};
    }
    dirty = true;
    update_region.Update(0, 0, width, height); // Refresh all if palette changes
}

void RaylibDraw::Flip() {
}

bool RaylibDraw::SetFlipMode(bool flip) {
    return true;
}

void RaylibDraw::Render() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    
    if (texture.id == 0) {
        // Create an empty texture
        Image img = GenImageColor(width, height, BLACK);
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        texture = LoadTextureFromImage(img);
        UnloadImage(img);
        SetTextureFilter(texture, TEXTURE_FILTER_POINT);
    }

    if (dirty) {
        // Convert indexed to RGBA
        // For simplicity, we convert the whole buffer if dirty
        // In the future, we can optimize using update_region
        static std::vector<Color> rgba;
        rgba.resize(width * height);
        
        for (size_t i = 0; i < width * height; ++i) {
            rgba[i] = raylib_palette[buffer[i]];
        }
        
        UpdateTexture(texture, rgba.data());
        dirty = false;
        update_region.Reset();
    }

    // Calculate source and destination rectangles for aspect-ratio preservation
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    
    // PC-88 original is 640x400 (8:5), but displayed as 4:3 on CRT
    float targetAspect = 4.0f / 3.0f;
    float windowAspect = screenW / screenH;
    
    Rectangle dest;
    if (windowAspect > targetAspect) {
        float h = screenH;
        float w = h * targetAspect;
        dest = { (screenW - w) / 2.0f, 0, w, h };
    } else {
        float w = screenW;
        float h = w / targetAspect;
        dest = { 0, (screenH - h) / 2.0f, w, h };
    }

    DrawTexturePro(texture, { 0, 0, (float)width, (float)height }, dest, { 0, 0 }, 0, WHITE);
}
