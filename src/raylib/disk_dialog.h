#pragma once

#include "raylib.h"
#include "diskmgr.h"
#include "pc88/config.h"
#include <string>
#include <vector>

class UIManager {
public:
    UIManager();
    ~UIManager();

    void Init();
    void Update(bool& shouldExit, class PC88* pc88, class CoreRunner* coreRunner);
    void Draw(DiskManager* diskmgr, PC8801::Config& cfg, class PC88* pc88, class CoreRunner* coreRunner, bool& shouldExit);
    void OpenNativeDialog(DiskManager* diskmgr, int drive);
    void OpenBothDrives(DiskManager* diskmgr);
    // 要求したドライブすべてのマウントに成功したときだけ true。
    // イメージが開けない場合や、挿入する枚が未対応メディア / m3u の実体欠落
    // だった場合は、ドライブに一切触れずに false を返す。
    // (セレクタを出す場合にユーザーが後から選ぶ枚は検証対象外)
    bool MountDisk(DiskManager* diskmgr, const char* path, int img1, int img2, bool openSelectorIfNeeded = true);
    void AddRecent(const std::string& path);
    void LoadRecent();
    void SaveRecent();
    void SetJPFont(Font font) { fontJp = font; }
    void SetENFont(Font font) { fontEn = font; }

    bool IsMenuOpen() const { return showMenu; }
    void ToggleMenu(class CoreRunner* coreRunner = nullptr);
    void RequestQuitConfirm() {
        // メニューが閉じている状態(⌘Q 等)から呼ばれた場合は、
        // 確認ダイアログを閉じたときにメニューも畳む必要がある
        if (!showMenu) quitOpenedMenu = true;
        modalState = MODAL_CONFIRM_QUIT;
        showMenu = true;
    }

    enum ModalState {
        MODAL_NONE,
        MODAL_CONFIRM_RESET,
        MODAL_CONFIRM_QUIT,
        MODAL_CONFIRM_CLEAR_RECENT
    };

private:
    void DrawMainMenu(DiskManager* diskmgr, class PC88* pc88, bool& shouldExit, class CoreRunner* coreRunner);
    void DrawSettings(PC8801::Config& cfg, class PC88* pc88, class CoreRunner* coreRunner);
    void DrawDiskSelector(DiskManager* diskmgr);
    // セレクタの対象ドライブを切り替える。Drive 1 <-> Drive 2 の行き来は
    // ヘッダの数字しか変わらず分かりづらいので、横スライドで方向を見せる。
    // 1<->2 を書き換える箇所が複数あるため、必ずここを通す。
    void SetSelectorDrive(int drive);
    void RefreshWriteProtectCache(DiskManager* diskmgr);
    bool IsCurrentDiskWriteProtected(DiskManager* diskmgr, int drive);
    // 書き込み禁止マークの鍵。GuiDrawIcon の pixelSize は整数倍しか取れないので、
    // 一度 16px で焼いたテクスチャを任意サイズに縮小して描く。
    void DrawKeyIcon(float x, float y, float size, Color color) const;
    void DrawRecentDiskDialog(DiskManager* diskmgr);
    void DrawStateDialog(DiskManager* diskmgr, class CoreRunner* coreRunner);
    void DrawConfirmDialog(bool& shouldExit, class PC88* pc88, class CoreRunner* coreRunner);
    void DismissConfirm();
    void DrawStatusBar(DiskManager* diskmgr);
    void DrawOSDMessage();
    void DrawDriveStatus(DiskManager* diskmgr, int drive, float x, float y);
    std::string GetStatePath(DiskManager* diskmgr, int slot) const;
    std::string GetStateScreenshotPath(DiskManager* diskmgr, int slot) const;
    bool StateSlotExists(DiskManager* diskmgr, int slot) const;
    void LoadStatePreview(const std::string& path);
    void DrawEnText(const char* text, int x, int y, Color color) const;
    int MeasureEnText(const char* text) const;

    bool showMenu;
    ModalState modalState;
    bool quitOpenedMenu; // RequestQuitConfirm がメニューを開いたか
    bool showSettings;
    bool showStateDialog;
    bool showRecentDialog;
    int selectingDiskForDrive; // -1: none, 0: Drive 1, 1: Drive 2
    bool selectingBothDrives;
    float selectorSlideT;      // 1.0 で停止。0.0 -> 1.0 の間だけスライド中
    int selectorSlideDir;      // +1: 次のドライブへ (右から入る), -1: 前へ戻る
    int recentDiskTargetDrive;
    int activeTab;
    int settingsTabScroll;
    int currentStateSlot;
    Vector2 diskScrollOffset;
    Vector2 recentScrollOffset;

    // ディスクセレクタ用の write-protect 状態キャッシュ。
    // 判定はイメージのヘッダ読み込みを伴うので毎フレームは引かない。
    std::vector<char> diskWriteProtect;
    int wpCacheDrive;
    std::string wpCachePath;

    // ステータスバー用。挿入中のディスクの write-protect をドライブごとに保持する
    bool statusWriteProtect[2];
    std::string statusWpPath[2];
    int statusWpIndex[2];
    
    // UI state
    int windowScale;
    bool isFullscreen;
    bool basicModeEdit;
    bool windowScaleEdit;
    bool cpuModeEdit;
    bool port44Edit;
    bool portA8Edit;
    bool samplingEdit;
    bool keyboardEdit;
    bool speedEdit;
    bool eramEdit;
    bool bufferEdit;
    bool masterVolEdit;
    bool volFmEdit;
    bool volSsgEdit;
    bool volAdpcmEdit;
    bool volRhythmEdit;
    bool volRhythmDetailEdit[6];
    bool mouseSensEdit;
    
    // Persistent values for uint config members (GuiValueBox requires int*)
    int bufferVal;
    int mouseSensVal;
    Vector2 systemScroll;
    Vector2 mixerScroll;
    Vector2 inputScroll;
    
    std::string lastAccessedDir;
    std::vector<std::string> recentDisks;
    std::string stateMessage;
    std::string statePreviewPath;
    Texture2D statePreviewTexture;
    Font fontJp;
    Font fontEn;
    RenderTexture2D keyIconTexture;
    bool keyIconReady;
    bool resetPending;
};
