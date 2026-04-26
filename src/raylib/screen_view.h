#pragma once

#include "draw.h"
#include "raylib.h"
#include <vector>
#include <mutex>

class RaylibDraw : public Draw {
public:
    RaylibDraw();
    virtual ~RaylibDraw();

    bool Init(uint width, uint height, uint bpp) override;
    bool Cleanup() override;

    bool Lock(uint8** pimage, int* pbpl) override;
    bool Unlock() override;
    
    uint GetStatus() override;
    void Resize(uint width, uint height) override;
    void DrawScreen(const Region& region) override;
    void SetPalette(uint index, uint nents, const Palette* pal) override;
    void Flip() override;
    bool SetFlipMode(bool flip) override;

    void Render();

private:
    uint width, height;
    std::vector<uint8_t> buffer;
    Palette palette[256];
    Color raylib_palette[256];
    Texture2D texture;
    std::recursive_mutex mutex;
    bool dirty;
    Region update_region;
};
