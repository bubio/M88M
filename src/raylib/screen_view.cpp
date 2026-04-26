#include "screen_view.h"
#include <cstring>

RaylibDraw::RaylibDraw() : width(0), height(0), dirty(false) {
    memset(palette, 0, sizeof(palette));
    image = {0};
    texture = {0};
}

RaylibDraw::~RaylibDraw() {
    Cleanup();
}

bool RaylibDraw::Init(uint w, uint h, uint bpp) {
    width = w;
    height = h;
    buffer.resize(width * height);
    
    image.data = buffer.data();
    image.width = width;
    image.height = height;
    image.format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE;
    image.mipmaps = 1;

    // Texture will be created in Render() on the main thread
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
    dirty = true;
    mutex.unlock();
    return true;
}

uint RaylibDraw::GetStatus() {
    return readytodraw;
}

void RaylibDraw::Resize(uint w, uint h) {
    // For PC-88, width/height usually constant 640x400
}

void RaylibDraw::DrawScreen(const Region& region) {
    // Core calls this when a region is updated
}

void RaylibDraw::SetPalette(uint index, uint nents, const Palette* pal) {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (index + nents <= 256) {
        memcpy(&palette[index], pal, nents * sizeof(Palette));
    }
}

void RaylibDraw::Flip() {
}

bool RaylibDraw::SetFlipMode(bool flip) {
    return true;
}

void RaylibDraw::Render() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (texture.id == 0) {
        // Create a temporary RGBA image to upload to texture
        Image rgba = GenImageColor(width, height, BLACK);
        ImageFormat(&rgba, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        texture = LoadTextureFromImage(rgba);
        UnloadImage(rgba);
    }

    if (dirty) {
        // Convert indexed to RGBA
        std::vector<uint32_t> rgba(width * height);
        for (size_t i = 0; i < width * height; ++i) {
            uint8_t idx = buffer[i];
            Palette& p = palette[idx];
            rgba[i] = (p.red) | (p.green << 8) | (p.blue << 16) | (0xFF << 24);
        }
        UpdateTexture(texture, rgba.data());
        dirty = false;
    }

    DrawTextureEx(texture, {0, 0}, 0, 1.0f, WHITE);
}
