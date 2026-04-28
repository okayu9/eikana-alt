# Development Notes

開発中にハマった点と、設計を変えた経緯のメモ。

## IMM API は Windows 11 の新 MS-IME に効かない

最初は `ImmGetContext(GetForegroundWindow())` + `ImmSetOpenStatus` / `ImmSetConversionStatus` でフォアグラウンドウィンドウの IME 状態を切り替える設計で実装した。これは Windows 10 までの IME と古いアプリでは動作するが、Windows 11 の **新 Microsoft IME (TSF ベース)** では効かない。

Microsoft 公式の Q&A スレッドで MSFT エンジニアが再現確認しており、API 代替は提示されていない:

- [Microsoft Q&A: ImmSetOpenStatus() not work in latest Win11 IME](https://learn.microsoft.com/en-us/answers/questions/1373936/i-dontt-know-why-immsetopenstatus()-not-work-in-th)

`AttachThreadInput` で対象スレッドの入力状態に attach する案も試したが効果なし。さらに Microsoft の troubleshooting 記事で `AttachThreadInput` + IMM 呼び出しは「IME クラッシュの原因になりうる」と警告されている。`WM_IME_CONTROL` + `IMC_SETOPENSTATUS` も同様に新 IME に効かないことが Q&A で確認されている。

最終的に **`SendInput` で `VK_IME_ON` / `VK_IME_OFF` (Win10 1903+) を送る方式** に変更。これは公式の "Keyboard Japan IME" 仕様書に absolute mode setter として明記されている仮想キーで、新 MS-IME・Google IME・ATOK のいずれでも動作する:

- [Keyboard Japan IME ImeOn/ImeOff Implementation](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/keyboard-japan-ime)

検証中にはこの他、`VK_DBE_HIRAGANA` / `VK_DBE_ALPHANUMERIC` (0xF2 / 0xF0) や `VK_CONVERT` / `VK_NONCONVERT` (0x1C / 0x1D, 要 MS-IME 設定) も候補に挙げて切り替えメニューを実装したが、最終的に `VK_IME_ON/OFF` だけで十分だったので削除した。

## CapsLock → Ctrl の LL フック実装は IME に消費されて不発

最初は `WH_KEYBOARD_LL` フックで以下を試した:

1. `CapsLock down` を捕まえる → swallow + `LCtrl down` を `SendInput` で注入
2. `CapsLock up` を捕まえる → swallow + `LCtrl up` を `SendInput` で注入

結果は **「Ctrl が押しっぱなしになる」** という症状。デバッグログで判明した実態:

- `CapsLock down` は LL フックで普通に届く → 我々が `LCtrl down` を注入する
- ユーザーが `CapsLock` を離す
- **`CapsLock up` が LL フックにも Raw Input にも届かない**
- 結果、我々は `LCtrl up` を発行しないため Ctrl が押しっぱなしになる

原因は **MS-IME の Ctrl+CapsLock ホットキー検出**。我々が注入した Ctrl と物理 CapsLock の組み合わせを「IME モード切替コマンド」として TSF レイヤが消費し、`CapsLock up` を握りつぶしている。LL フックも Raw Input (WM_INPUT) も TSF より下位の経路ではない (LL フックでの suppression は WM_INPUT の配信も止める)。

### 試した回避策

1. **Raw Input (WM_INPUT) で物理 release を検知** → ダメ。LL フックで suppress した CapsLock は WM_INPUT にも届かない
2. **`GetAsyncKeyState` で物理状態をポーリング** → ダメ。LL フックで suppress すると async key state も更新されない
3. **キーごとに Ctrl をかぶせる** (CapsLock down では Ctrl 注入せず、別キー押下時に `Ctrl down + key + Ctrl up` を注入) → 一時動作したが、エッジケース (CapsLock を先に離して別キーを後で離す等) で `g_capsHeld` が真で残り、その後の通常タイプが Ctrl+key として誤解釈される

### 最終的な判断

**「LL フックでは戦わず OS レベル (Scancode Map) でリマップする」** に切り替えた。

Scancode Map (`HKLM\SYSTEM\CurrentControlSet\Control\Keyboard Layout`) はキーボードクラスドライバ層 (`kbdclass.sys`) で処理されるため、LL フック・Raw Input・TSF いずれよりも下位で動作する。CapsLock キーを押した瞬間、そもそも CapsLock イベントが発生しなくなる (ドライバが LCtrl として上げる)。

トレードオフ:

- 管理者権限が必要 (HKLM の書き込み)
- 再起動 (またはサインアウト→サインイン) で有効化される
- ランタイムでの ON/OFF 切替不可

これらは個人ノート PC 用途では問題なく、確実に動く方を取った。

## トレイの隠しウィンドウは `HWND_MESSAGE` にしてはいけない

最初は `HWND_MESSAGE` ウィンドウを親にして `TrackPopupMenu` を呼んでいたが、`HWND_MESSAGE` ウィンドウは focusable でないため `SetForegroundWindow` が無効化され、結果として「メニュー外クリックでもメニューが消えない」という Windows の既知バグの回避策が動かなくなる。

`WS_POPUP | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE` の非表示トップレベルウィンドウに変えて解決。

## `VK_NONAME` はメニュー抑制用ダミーキーに使ってはいけない

`VK_NONAME` (0xFC) は IMM 系で `VK_DBE_DETERMINESTRING` と同値になっており、IME がコンポジション中だと「現在の変換を確定」コマンドとして解釈される。Alt キーをタップしたら IME の変換が勝手に確定する事故が起きる。

`VK_F24` (0x87) に変更。実在のキーだがほとんどのアプリが割り当てを持たず、IMM 系の特殊扱いも受けない。

## 過去のレビュー指摘から取り入れた点

セカンドパスのコードレビュー (general-purpose subagent) で挙がった主な改善:

- フォアグラウンドのレース対策 (`captured == GetForegroundWindow()` ガード + 自分の HWND チェック)
- `Scancode Map` のマージ書き込み (他ツールのリマップを保持)
- 長パス対応の `GetModuleFileNameW` ラッパ
- `CommandLineToArgvW` + テーブルによる昇格コマンドディスパッチ
- `wsprintfW` (非推奨) → `StringCchPrintfW`
- `Settings` モジュールを `AppState` に統合 (一個の bool に二モジュールは過剰)
- `TrayIcon::State` 構造体でメニュー状態を集約
- `ImeController` から `Injected::Tag` を撤去 (検出器が空の状態で実行されるため不要だった)

## 参考リンク

- [Microsoft Q&A: ImmSetOpenStatus() not work in latest Win11 IME](https://learn.microsoft.com/en-us/answers/questions/1373936/i-dontt-know-why-immsetopenstatus()-not-work-in-th)
- [Microsoft Learn: IME may crash on cross-thread window messages](https://learn.microsoft.com/en-us/troubleshoot/windows/win32/ime-crash-processing-cross-thread-sent-message)
- [Keyboard Japan IME ImeOn/ImeOff Implementation](https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/keyboard-japan-ime)
- [PowerToys issue #34554: Add VK_IME_ON/VK_IME_OFF support](https://github.com/microsoft/PowerToys/issues/34554)
- [LowLevelKeyboardProc reference](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc)
