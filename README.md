# eikana-alt

Windows 用の常駐型キーボードユーティリティ。

- **左 Alt 単独押し** → IME を OFF (英数モード)
- **右 Alt 単独押し** → IME を ON (かなモード)
- **CapsLock → Ctrl** にリマップ (任意)
- **Windows サインイン時の自動起動** (任意)

すべてトレイメニューから ON/OFF できます。

## 動作環境

- Windows 11 (Windows 10 でも動作する見込み)
- Microsoft IME (TSF ベースの新 MS-IME に対応)
- 64-bit MSVC (Visual Studio 2022 / Build Tools 2022)

## ビルド

VS の **Developer Command Prompt for VS** から:

```
nmake               ビルド (incremental)
nmake clean         build\ を削除
```

`build\eikana-alt.exe` が生成されます。

## 使い方

1. `build\eikana-alt.exe` を起動するとトレイにアイコンが出ます
2. 左 Alt をタップ → IME が OFF (英数) になります
3. 右 Alt をタップ → IME が ON (かな) になります
4. Alt+F4 / Alt+Tab など他のキーと組み合わせた場合は通常通り動作します

## トレイメニュー

トレイアイコンを右クリックするとメニューが出ます。

| 項目 | 内容 |
|---|---|
| Alt キーで IME 切替 | Alt 単独押しによる IME 切替の有効/無効 |
| CapsLock を Ctrl にリマップ (要再起動) | OS レベルで CapsLock を Ctrl 化 (UAC + 再起動が必要) |
| Windows 起動時に自動起動 | サインイン時に本ツールを自動起動 |
| 終了 | 終了 |

各項目の状態はレジストリに永続化されるため、次回起動時にも引き継がれます。

## ドキュメント

- [ARCHITECTURE.md](ARCHITECTURE.md) — 内部構造とモジュール構成
- [DEVELOPMENT.md](DEVELOPMENT.md) — 設計上の判断と試行錯誤の記録

## ライセンス

[Apache License 2.0](LICENSE)
