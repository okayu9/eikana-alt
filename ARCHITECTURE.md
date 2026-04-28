# Architecture

## ファイル構成

```
src/
  main.cpp                    エントリ、メッセージループ、各モジュール結線
  AltSoloDetector.{h,cpp}     単独 Alt 検出の状態機械 (純粋ロジック)
  AppState.{h,cpp}            有効/無効フラグ + 永続化
  Autostart.{h,cpp}           HKCU Run キー操作
  CapsRemapInstaller.{h,cpp}  HKLM Scancode Map 操作 + UAC 昇格
  ImeController.{h,cpp}       VK_IME_ON/OFF 注入
  InjectedTag.h               自分の合成イベント識別タグ
  KeyboardHook.{h,cpp}        WH_KEYBOARD_LL ラッパ
  ModulePath.h                long-path 対応の GetModuleFileName ラッパ
  Resource.h                  メニュー ID + アプリケーションメッセージ
  TrayIcon.{h,cpp}            Shell_NotifyIcon ラッパ + コンテキストメニュー
Makefile                      NMAKE 用ビルド定義
```

## ビルド

`Makefile` は NMAKE 用。`{src\}.cpp{build\}.obj::` バッチモード推論ルールで out-of-date な .cpp をまとめて 1 回の `cl` 呼び出しに渡し、`/MP` で並列コンパイルする。`$(OBJS): $(HEADERS)` の coarse-grained 依存により、ヘッダ変更時は全 .obj が再コンパイルされる (per-cpp 依存追跡は未実装)。

ビルドフラグ: `/std:c++20 /permissive- /utf-8 /W4 /O2 /MP`、`/SUBSYSTEM:WINDOWS`。
リンクは `user32 / shell32 / advapi32`。

## 単独 Alt 検出

`WH_KEYBOARD_LL` でキーイベントを受け、`AltSoloDetector` が以下のロジックで判定:

- Alt down → 該当 Alt の `down` フラグをセット
- 他キーが入る → `combined` フラグを立てる
- Alt up → `down` かつ `!combined` なら solo 確定

solo Alt up を捕まえたらフックは event を swallow し、`onSoloAlt` コールバックが起動する。コールバックでは:

1. メニュー抑制シーケンスを `SendInput` で注入
2. その時点のフォアグラウンド HWND を捕獲
3. `WM_APP_SOLO_ALT` をメッセージループに `PostMessage` (HWND を `lParam` で渡す)

メッセージループ側で IME 切替を実行する。フック内で `SendInput` を多用するとフックタイムアウトのリスクがあるため、重い処理は遅延させている。

## メニュー抑制

Alt 単独押下は Windows のメニューバー活性化トリガになる。これを避けるため、solo Alt up 検出時に以下を 1 回の `SendInput` で連続注入する:

1. `VK_F24` down
2. `VK_F24` up
3. `VK_LMENU` / `VK_RMENU` up

ステップ 1〜2 で「Alt + 何かのキー」状態を成立させ、Windows 内部の menu-bar trigger フラグをクリア。ステップ 3 で我々が swallow した本物の Alt up を補填する。

`VK_F24` を選んでいる理由:

- 物理キーとしてほぼ存在しないため、アプリが割り当てを持っていない
- IMM 系の特殊扱い (例: `VK_NONAME=0xFC` は IMM の `VK_DBE_DETERMINESTRING` と同値) を受けない

注入イベントには `dwExtraInfo = Injected::Tag` (`0xE1CA1A`) を載せ、フック側で再エントリしてもタグ判定で即 return false させる。

## フォアグラウンドのレース対策

`onSoloAlt` で捕獲した HWND を `WM_APP_SOLO_ALT` の `lParam` に積み、メッセージ処理時点で `GetForegroundWindow()` と一致しなければ実行を中止する。Alt+Tab 直後やトレイメニュー操作中の発火で、別ウィンドウに IME キーを送り込むのを防ぐ。

捕獲 HWND が自分の隠しウィンドウ (`g_hwnd`) と一致する場合も中止 (`SetForegroundWindow` でメニューを表示する瞬間に Alt をタップしたケース)。

## IME 切替

`ImeController::setEnglish` / `setJapanese` が `SendInput` で `VK_IME_OFF` (0x1A) または `VK_IME_ON` (0x16) を 1 タップ送る。

これらは Windows 10 1903 で公式に absolute mode setter として規定された仮想キーで、新 MS-IME (TSF ベース) でも、Google IME / ATOK でも動作する。`ImmSetOpenStatus` 等の IMM API は新 MS-IME で機能しない (DEVELOPMENT.md 参照)。

## CapsLock → Ctrl

`HKLM\SYSTEM\CurrentControlSet\Control\Keyboard Layout` の `Scancode Map` (REG_BINARY) を編集する。

バイナリ形式:

```
[8 byte header (zeros)]
[4 byte: count = N + 1]    ← N 個のマッピング + 終端
[4 byte: target_scancode | source_scancode] × N
[4 byte terminator (zeros)]
```

CapsLock (0x3A) を LCtrl (0x1D) にマップするエントリ `1D 00 3A 00` を追加する。`isInstalled` / `installAsElevated` / `uninstallAsElevated` はいずれも既存値を読み込み、エントリ単位でマージ/部分削除するため、SharpKeys 等の他ツールが設定したリマップは保持される。

キーボードクラスドライバ層 (`kbdclass.sys`) で動作するため、LL フック・Raw Input・TSF いずれよりも下位で機能する。再起動 (またはサインアウト→サインイン) で有効化。

### UAC 昇格

`HKLM` への書き込みは管理者権限が必要なので、`ShellExecuteEx` の `runas` verb で自分自身を再起動する。コマンドライン引数で動作を分岐:

- `--install-caps-remap` → `CapsRemapInstaller::installAsElevated()`
- `--uninstall-caps-remap` → `CapsRemapInstaller::uninstallAsElevated()`

子プロセスは `CommandLineToArgvW` でパースされた `argv[1]` を `kElevatedActions[]` テーブルから引いてディスパッチする (`main.cpp::dispatchElevated`)。子は登録/削除を実行して MessageBox で結果を表示し、即座に終了 (トレイは作らず、Mutex も取らない)。

親プロセスは子の完了を待たず、メニューを開くたびに HKLM を読み直して状態を表示する。

## 自動起動

`HKCU\Software\Microsoft\Windows\CurrentVersion\Run` の `eikana-alt` 値に EXE のフルパス (クォート付き) を書き込む。HKCU なので管理者権限不要で、UAC プロンプトも出ない。

`Autostart::setEnabled` は `ModulePath::get()` で long-path 対応の `GetModuleFileNameW` ラッパを使い、`MAX_PATH` を超えるパスでもバッファ二倍化リトライで取得する。

## 永続化

| 設定 | 場所 |
|---|---|
| Alt → IME 切替の有効/無効 | `HKCU\Software\eikana-alt\Enabled` (REG_DWORD) |
| CapsLock → Ctrl リマップ | `HKLM\...\Keyboard Layout\Scancode Map` 自体が状態保持 |
| 自動起動 | `HKCU\...\Run\eikana-alt` 値の有無が状態 |

`AppState::load()` は起動時に呼ばれる。レジストリ値が無ければ既定値 (有効) を書き込んで永続化する (初回起動時に値が出来る)。

## 単一インスタンス

名前付き Mutex `Local\eikana-alt-singleton` を `CreateMutexW(bInitialOwner=TRUE)` で取得。`Local\` 名前空間はセッション固有なので、ファストユーザースイッチで複数ユーザーが各自起動可能。

UAC 昇格の子プロセスはコマンドライン引数で早期に `wWinMain` から return するので、Mutex を取りに行かない。親と子の衝突は発生しない。

## ウィンドウ

`HWND_MESSAGE` ではなく `WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE` の非表示トップレベルウィンドウを使う。`HWND_MESSAGE` だと `SetForegroundWindow` が機能せず、`TrackPopupMenu` のメニュー外クリック dismiss が壊れるため。

## C++ スタイル

- `/std:c++20 /permissive-`
- `noexcept` を Win32 ラッパ全般に付与
- 副作用無視を防ぐため成功フラグ系には `[[nodiscard]]`
- ホットパス (LL フックの hot path) には `[[likely]]` / `[[unlikely]]`
- `std::function` ではなく `noexcept` 関数ポインタ型 (`std::function` の隠れヒープ確保を避ける)
- INPUT 構造体は designated initializer + `std::array` で記述
- グローバル状態は `constinit std::atomic<T>` で初期化順序を確定
