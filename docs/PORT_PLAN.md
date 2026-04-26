# M88M macOS / raylib 移植計画

cisc 氏の PC-8801 エミュレータ M88 改造版を、Win32 + DirectX 専用から **macOS を主ターゲット、Linux / Windows もビルド可能** な構成へ移植する。上位層 (描画・音・入力・UI) は **raylib + raygui** に置き換える。

このドキュメントは生きた計画書で、各 Phase の進行に合わせて随時更新する。

---

## ゴール

| 主目的 | macOS (arm64 / x86_64) でプレイアブルな PC-8801 エミュレータ化 |
| ---  | --- |
| 副目的 | Linux / Windows でも CMake 一発でビルドできる |
| 制約 | `src/win32/` の Win32 ビルド資産はそのまま温存 (退行禁止) |
| スコープ | 最小プレイアブル (画面・OPNA/PSG/BEEP・JIS キー・D88 マウント・config + ROM 配置) |

「最小プレイアブル」の境界は **Phase 8 検証項目** 参照。c86ctl / .m88 拡張モジュール / 全デバッグウィンドウ / 設定 PropSheet UI は初期リリース対象外。

---

## 設計方針

### コアと上位層の境界

M88 はもともと **コア (`src/{common,devices,pc88,if}/` 約 21k 行) と上位層 (`src/win32/` 約 16k 行) が明確に分離** されており、コア側は `windows.h` を一切含まない (cisc 氏の COM 風 IF 設計の恩恵)。これを利用し:

- コアは `src/common,devices,pc88,if/` に in-place のまま、Win32 依存の最後の数 % (calling convention マクロ・Win32 専用 IF・MS CRT 関数) のみを `src/common/compat.h` で吸収する。
- 新フロントエンド `src/raylib/` を並置し、`Draw` / `ISoundControl` / `WinKeyIF` 相当を実装。
- `src/win32/` は **手を入れずに保持**。Visual Studio (`M88_2008.vcproj`) でこれまで通りビルドできる。

### IF の扱い

`src/if/ifcommon.h` の末尾に並んでいた Win32 専用 IF (`IConfigPropBase` / `IConfigPropSheet` / `IWinUIExtention` / `IWinUI2`) は `#ifdef _WIN32` ガードで囲い、portable 側ではコアが触らない約束になっているのでそのまま消える。`ifui.h::IMouseUI` は `POINT` を引数に取るが、`compat.h` で POSIX 用 `struct POINT { long x, y; }` を提供することで API 互換を保つ。

### Z80 エンジン

オリジナルは 32-bit MSVC で `Z80_x86.cpp` (3065 行のインライン x86 アセンブラ) を使い、`Z80c.cpp` (2374 行の C++ ポート) はバックアップ的位置づけ。macOS の arm64 / x86_64 では `Z80c.cpp` を採用する。`USE_Z80_X86` は `defined(_MSC_VER) && !defined(_WIN64) && !defined(M88_NO_Z80_X86)` の時だけ有効化、CMake は `M88_NO_Z80_X86` を `m88core` の PUBLIC 定義として渡す。

### スレッドモデル

オリジナルでは `win32/sequence.cpp` が CPU/sound 用ワーカースレッドを建て、UI スレッドが draw を回す。raylib 版でも同じ二重スレッド構造を踏襲し、`src/common/critsect.h` を `std::recursive_mutex` ベースに置き換えて (`win32/CritSect.h` の `CRITICAL_SECTION` 版と契約は同じ) 排他を維持する。

### ファイル / OS 抽象

- `FileIO`: `win32/file.cpp` の Win32 `HANDLE` 版に対し、`src/common/file.cpp` の `FILE*` 版を提供。同じ public surface (`Open` / `Read` / `Write` / `Seek` / `Tellp` / `SetEndOfFile` / `CreateNew` / `Reopen` / `Close`)。
- `StatusDisplay`: GDI 描画の代わりに「最新メッセージ + 残り表示時間 + FD ランプ状態」を内部に保持する portable 版に差し替え、フロントエンドが `GetCurrentMessage()` / `GetFDState()` で吸い上げて raygui で表示する設計。
- `LoadMonitor` / 各種 `*mon.cpp`: 全て `LOADBEGIN(s) / LOADEND(s)` を no-op マクロに collapse させて完全に消す。

---

## レイアウト

```
M88M/
├── CMakeLists.txt           ← 新規 (Phase 1)
├── docs/
│   ├── diagram.md           ← 既存 (mermaid 構造図)
│   ├── diagram.png          ← 既存 (図のレンダリング)
│   └── PORT_PLAN.md         ← このファイル
├── src/
│   ├── common/              ← コア共通基盤 + portable シム新設多数
│   ├── devices/             ← Z80c / OPNA / FMGEN / OPM / PSG (Z80_x86 は CMake で除外)
│   ├── pc88/                ← PC-88 本体ロジック
│   ├── if/                  ← IF 定義 (Win32 IF は #ifdef _WIN32 で隔離)
│   ├── raylib/              ← 新規フロントエンド (Phase 2 以降)
│   ├── win32/               ← 既存 Win32 フロントエンド (温存)
│   ├── piccolo/             ← c86ctl (温存・portable では未使用)
│   └── zlib/                ← 同梱 zlib (CMake は find_package 優先)
├── M88_2008.vcproj          ← 既存 (Win32 ビルド)
└── M88_2008.sln             ← 既存
```

### `src/common/` 新設ファイル一覧 (Phase 0 で追加)

| ファイル | 役割 |
| --- | --- |
| `compat.h`         | `IFCALL` / `IOCALL` / `MEMCALL` のクロスプラットフォーム化、`PATH_MAX`・`MAX_PATH`・`_T()`・`strncpy_s` 等の MS CRT シム、`POINT` / `GUID` / `REFIID` / `HRESULT` / `MulDiv` / `localtime_s` / `OutputDebugString` / `interface` キーワードの POSIX 側スタブ |
| `types.h`          | `uint8/uint16/.../intpointer` の型定義と `USE_Z80_X86` / `STATIC_CAST` / `MEMCALL` の選択。`src/win32/types.h` はここへの forwarder に縮約。 |
| `headers.h`        | 旧 PCH 互換のスタブ。`<stdio.h>` 等の最小セット + `using std::min/max/swap`。 |
| `diag.h`           | `Log` / `LOG0..LOG9` を可変引数 no-op マクロに。 |
| `critsect.h`       | `CriticalSection` / `CriticalSection::Lock` を `std::recursive_mutex` で実装。 |
| `loadmon.h`        | `LOADBEGIN` / `LOADEND` を no-op マクロに。 |
| `status.h/.cpp`    | `StatusDisplay` の portable 実装 + フロント向け `GetCurrentMessage` / `GetFDState`。 |
| `file.h/.cpp`      | `FileIO` の `FILE*` バック実装。 |
| `piccolo_stub.h`   | `Piccolo` / `PiccoloChip` / `PICCOLO_*` の null-instance スタブ。 |

---

## Phase 進捗

各 Phase の依存関係:

```
Phase 0 (2.0d) ──┐
                 ▼
Phase 1 (1.0d) ──┐
                 ▼
Phase 2 (2.5d) ──┬─→ Phase 3 (1.5d)  描画
                 ├─→ Phase 4 (2.0d)  サウンド
                 ├─→ Phase 5 (1.5d)  入力
                 ├─→ Phase 6 (1.0d)  ディスク
                 └─→ Phase 7 (0.5d)  設定/ROM
                              │
                              ▼
                        Phase 8 (1.0d)  検証
```

総工数見積もり: **約 13 人日**

### Phase 0 — コアの純化 ✅ 完了

`src/common/compat.h` 等を新設し、コアが windows.h なしでコンパイルできる状態にした。

主な変更:
- `src/win32/types.h` を `src/common/types.h` への forwarder 化
- `ifcommon.h` 末尾の Win32 専用 IF を `#ifdef _WIN32` で隔離
- `Z80c.cpp`, `pc88/*.cpp` (16 ファイル) の `&Member` を Python スクリプトで `&Class::Member` に一括修正 (clang は MSVC と違って unqualified member-function-pointer 取得を許さないため)
- `psg.cpp` / `screen.cpp` のナロー化キャストを `(uint8)-1` 等に明示
- `opnif.cpp` の Romeo / Piccolo include を `#ifdef _WIN32` ガード + `piccolo_stub.h` フォールバック
- `compat.h` に `MulDiv` / `localtime_s` / `_splitpath` / `strncpy_s` 等の MS CRT 互換関数を実装
- `STATIC_CAST` を MSVC 限定で `static_cast<>` に、それ以外は C-style キャストに

退行確認: `src/win32/` は1ファイル (`types.h` を1行 forwarder へ) のみ変更。Visual Studio ビルドへの影響はこの転送のみ。

### Phase 1 — CMake ビルドシステム ✅ 完了

トップレベル `CMakeLists.txt` 新設、ターゲット `m88core` (STATIC) と `m88_raylib` (EXECUTABLE; Phase 2 以降にソース追加)。

- C++17, RelWithDebInfo, `-fno-strict-aliasing` (FMGEN 要件), `-Wno-multichar` (DEV_ID マクロ)
- raylib v5.5 / raygui v4.0 を `FetchContent_Declare`
- ZLIB は `find_package(ZLIB)` 優先、不在時に `src/zlib/` をフォールバックビルド
- macOS は `-framework Cocoa/IOKit/CoreVideo/CoreAudio/AudioToolbox`
- `USE_NEW_CAST` を MSVC 限定にし、`m88core` PUBLIC で `M88_NO_Z80_X86` を定義

成果: macOS arm64 で `libm88core.a` (≈2.6 MB) が完走。

### Phase 2 — フロントエンドスケルトン ✅ 完了

`src/raylib/` を新設し、基本骨格を実装。

| ファイル | 役割 |
| --- | --- |
| `app.cpp` | エントリ点。`InitWindow` → main loop → `draw.Render()` → `EndDrawing` |
| `core_runner.cpp/.h` | `std::thread` で `PC88::Proceed()` を駆動。 |
| `screen_view.cpp/.h` | `RaylibDraw : public Draw` のスケルトン（インデックス→RGBA変換含む）。 |
| `paths.cpp/.h` | macOS / Linux の設定・ROMディレクトリ解決。 |
| `audio_out.cpp/.h` | `RaylibSound : public ISoundControl` (Phase 4 スケルトン) |
| `key_input.cpp/.h` | 入力ハンドラ (Phase 5 スケルトン) |
| `disk_dialog.cpp/.h` | raygui ディスク操作 UI (Phase 6 スケルトン) |
| `config.cpp/.h` | 設定管理 (Phase 7 スケルトン) |

**成果**: 空ウィンドウが立ち上がり、`m88core` とリンクしてバックグラウンドでコアが動作することを確認。

### Phase 3 — 描画橋渡し ✅ 完了

`Draw` 抽象 (`src/common/draw.h`) を `RaylibDraw` で実装:

- `Init(640, 400, 8)` → 8bpp インデックスバッファ。
- `SetPalette` でコアから渡されるパレットを `Color` 配列に保持。
- `Render()` 内で 8bpp → RGBA8888 変換し、テクスチャを更新。
- `DrawTexturePro` を使用し、4:3 アスペクト比を維持してスケーリング表示。
- `CoreRunner` から `UpdateScreen()` を定期的に呼び出し描画を駆動。

### Phase 4 — サウンド ✅ 完了

`RaylibSound : public ISoundControl`:

- `InitAudioDevice()` および `LoadAudioStream(44100, 16, 2)` で初期化。
- `GlobalAudioCallback` 内で各 `ISoundSource` (OPNA1/2, BEEP) の `Mix()` を呼び出し合成。
- 32-bit ミックス結果を 16-bit 整数にクリップして出力。
- `CoreRunner` の初期化時に各デバイスを `Connect()`。

### Phase 5 — キーボード / ジョイ入力 ✅ 完了

`KeyInput : public Device` を実装し、IOポート 0x00-0x0f をエミュレート:

- raylib の `IsKeyDown()` を PC-8801 の 16x8 キーマトリクスにマッピング。
- メイン CPU の `IIOBus` (Port 0x00-0x0f) に接続し、負論理で応答。
- 主要なキー（英数字、記号、SHIFT/CTRL/GRPH、STOP/ESC、矢印、F1-F5）を実装。
- ※ジョイパッド入力は初期リリース対象外として保留。

### Phase 6 — ディスク / テープ 🚧 進行中

- `src/common/file.cpp` の `FileIO` portable 実装は既存 (Phase 0 で導入済み)
- `disk_dialog.cpp` で raygui ファイルダイアログ → `DiskManager::Mount(drive, path, readonly)`
- ドラッグ & ドロップ: raylib `IsFileDropped()` + `LoadDroppedFiles()`

### Phase 7 — 設定 / ROM 配置 ⏳

ROM 検索順:

1. 環境変数 `$M88M_ROM_DIR`
2. 実行ファイル隣の `roms/`
3. 設定ディレクトリ:
   - macOS: `~/Library/Application Support/M88M/`
   - Linux: `${XDG_CONFIG_HOME:-~/.config}/m88m/`
   - Windows: `%APPDATA%\M88M\`

必須 ROM: `N88.ROM` / `N88_N.ROM` / `DISK.ROM` / `KANJI1.ROM` / `KANJI2.ROM` / `FONT.ROM` (任意 `FONT80SR.ROM`)

`config.ini` も同パス、スナップショット / ディスク履歴は `~/Documents/M88M/`。

### Phase 8 — 動作検証 ⏳

1. ROM 不在で起動 → ダイアログ表示でクラッシュなし
2. ROM 配置後 → N88-BASIC ロゴ・プロンプト到達
3. `sample1` / `sample2` の D88 を F1 マウント → `RUN"START"` 起動
4. `BEEP` / `PLAY` で BEEP / PSG / OPNA 音再生 (欠落・割れなし)
5. JIS キーで `LIST` / `PRINT` 入力一致、GRPH / STOP / COPY / カナ動作
6. F6 セーブ → リセット → F7 ロードで状態復元
7. macOS (arm64) + Linux + MinGW でビルド完走

---

## 後回し (初期リリース対象外)

| 機能 | 理由 / 後の対応案 |
| --- | --- |
| `src/piccolo/` (c86ctl 連携) | macOS には対応 OPNA ハードウェアが事実上ない |
| `src/win32/extdev.cpp` (`.m88` 拡張モジュール) | DLL ロード機構が Win32 専用、移植コスト大 |
| `*mon.cpp` 全デバッグウィンドウ (`basmon` / `codemon` / `iomon` / `memmon` / `regmon` / `loadmon` / `mvmon` / `soundmon`) | デバッグ用、初期版では割愛 |
| `cfgpage*` / `wincfg` / `88config` の PropSheet GUI | 代替で簡易 raygui タブを Phase 7 内に最小実装 |

スナップショット save / load の **ロジック自体** (`win32/wincore.cpp::SaveShapshot` / `LoadShapshot`) は portable 化して F6 / F7 のキーバインドから呼ぶ予定。

---

## ビルド / 実行

```bash
# macOS / Linux
cmake -S . -B build
cmake --build build -j

# 実行 (Phase 2 以降に m88_raylib ターゲットが生成される)
./build/m88
```

依存ライブラリ:
- raylib v5.5 (CMake が自動取得)
- raygui v4.0 (CMake が自動取得)
- zlib (システム or 同梱フォールバック)

Windows ビルド (既存) は引き続き Visual Studio で `M88_2008.sln` を開いて使用可能。

---

## 参考

- 元プロジェクト: cisc 氏 M88 — http://retropc.net/cisc/m88/
- 改造版 README: `/README.md`
- リポジトリ構造図: `/docs/diagram.md`, `/docs/diagram.png`
- 内部プラン (詳細・初期構想): `~/.claude/plans/windows-pc88-m88-macos-raylib-raylib-ancient-tower.md`
