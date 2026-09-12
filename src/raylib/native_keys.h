#pragma once

// JIS 固有キーのうち raylib (GLFW) がキーコードを割り当てないものを、
// プラットフォームのネイティブ API で直接読み取る。
// GLFW は ¥ キー / ろ(_) キーを GLFW_KEY_UNKNOWN として捨てるため、
// IsKeyDown() では検出できない。

enum class NativeKey {
    JisYen, // ¥ | (BS の左)
    JisRo,  // \ _ ろ (右 SHIFT の左)
};

// ウィンドウがフォーカスを持っている前提で呼ぶこと。
// 対応していないプラットフォームでは常に false を返す。
bool NativeKeys_IsDown(NativeKey key);
