# FMSQ - Family mruby Sequence Format (v1)

## 概要

本仕様は、ファミコンのAPU（Audio Processing Unit）に基づく音楽データを、
コンパクトなバイナリ形式で表現し、フレーム単位（60Hz）で再生するためのフォーマットを定義する。

再生側はAPUエミュレータ（nofrendo等）を使用し、FMSQコマンドから
APUレジスタへの書き込みを直接実行することで、元のNSF音源を忠実に再現する。

### 特徴

- 保存形式: フレーム区切りのイベント列
- 再生形式: フレーム駆動（1/60秒ごとにtick）
- APUレジスタ書き込みを順序・値ともに完全保持（REG_WRITE命令）
- WAITによる無変化フレームの圧縮
- 実装容易（シンプルなステートマシンで再生可能）

---

## 用語

| 短縮名 | 正式名 | 意味 |
|---|---|---|
| APU | Audio Processing Unit | ファミコン音源回路 |
| PC | Program Counter | データ読み取り位置 |
| WAIT | Wait Frames | 次イベントまでの待ちフレーム数 |

---

## ファイル構造

```
[ヘッダ (12バイト)]
[命令列 (可変長)]
```

### ヘッダ

| offset | size | 内容 |
|--------|------|------|
| 0x00 | 4 | magic: "FMSQ" |
| 0x04 | 1 | version: 1 |
| 0x05 | 1 | flags: 予約 (0) |
| 0x06 | 2 | frame_count: 総フレーム数 (little-endian) |
| 0x08 | 2 | data_size: 命令列のバイト数 (little-endian) |
| 0x0A | 2 | loop_offset: ループ先オフセット (0 = ループなし, little-endian) |

---

## 命令フォーマット

データは1バイト単位の命令列で構成される。
命令は最上位ビットで大分類される。

### 命令体系一覧

| ビットパターン | 種別 | 説明 |
|---------------|------|------|
| `0xxxxxxx` | WAIT | 1-128フレーム待ち |
| `110aaaaa` | REG_WRITE | APUレジスタ直接書き込み (+ 1バイトデータ) |
| `11111110` | END | 再生終了 |
| `11111111` | LOOP | ループ (+ 2バイトオフセット) |

---

## WAIT命令

```
0xxxxxxx
```

- 上位1ビット = 0 -> WAIT
- 下位7ビット = 待ちフレーム数 - 1

| 値 | 意味 |
|---|---|
| 0x00 | 1フレーム待ち |
| 0x01 | 2フレーム待ち |
| ... | ... |
| 0x7F | 128フレーム待ち |

---

## REG_WRITE命令

```
110aaaaa [DATA]
```

APUレジスタへの直接書き込み。NSFのAPUレジスタ操作を順序・値ともに完全に保持する。

- `aaaaa`: $4000からのオフセット (0-23)
- `DATA`: 書き込む値 (1バイト)
- 合計: 2バイト/書き込み

| offset (aaaaa) | APUレジスタ | 内容 |
|----------------|-----------|------|
| 0x00 | $4000 | Pulse1 Volume/Envelope/Duty |
| 0x01 | $4001 | Pulse1 Sweep |
| 0x02 | $4002 | Pulse1 Timer Low |
| 0x03 | $4003 | Pulse1 Timer High + Length |
| 0x04 | $4004 | Pulse2 Volume/Envelope/Duty |
| 0x05 | $4005 | Pulse2 Sweep |
| 0x06 | $4006 | Pulse2 Timer Low |
| 0x07 | $4007 | Pulse2 Timer High + Length |
| 0x08 | $4008 | Triangle Linear Counter |
| 0x0A | $400A | Triangle Timer Low |
| 0x0B | $400B | Triangle Timer High + Length |
| 0x0C | $400C | Noise Volume/Envelope |
| 0x0E | $400E | Noise Period + Mode |
| 0x0F | $400F | Noise Length |
| 0x10 | $4010 | DMC Frequency/Flags |
| 0x11 | $4011 | DMC DAC (7bit) |
| 0x12 | $4012 | DMC Sample Address |
| 0x13 | $4013 | DMC Sample Length |
| 0x15 | $4015 | Status (Channel Enable) |

再生時の処理:
```c
if (FMSQ_IS_REG_WRITE(cmd)) {
    uint8_t data = next_byte();
    apuif_write_reg(0x4000 + (cmd & 0x1F), data);
}
```

### 判定マクロ

```c
#define FMSQ_IS_REG_WRITE(cmd)  (((cmd) & 0xE0) == 0xC0)
#define FMSQ_REG_ADDR(cmd)      (0x4000 + ((cmd) & 0x1F))
```

---

## META命令

### END

```
11111110
```

- データ終端。再生停止。

### LOOP

```
11111111 [OFFSET_LO] [OFFSET_HI]
```

- ループ先オフセット（命令列先頭からの相対バイト位置、little-endian）
- PCをこのオフセットに戻して再生を継続

---

## 命令コード一覧

| コード | 命令 | 後続バイト | 合計サイズ |
|--------|------|-----------|-----------|
| 0x00-0x7F | WAIT (1-128フレーム) | 0 | 1 |
| 0xC0-0xD7 | REG_WRITE ($4000-$4017) | 1 (DATA) | 2 |
| 0xFE | END | 0 | 1 |
| 0xFF | LOOP | 2 (OFFSET) | 3 |

### 予約領域

| コード | 状態 |
|--------|------|
| 0x80-0xBF | 予約（将来のチャネル命令: NOTE_ON, NOTE_OFF, PARAM等） |
| 0xD8-0xDF | 予約（REG_WRITE拡張: $4018-$401F） |
| 0xE0-0xFD | 予約（DPCM命令、拡張チップ等） |

---

## 再生仕様

### 内部状態

- PC: 命令列内の現在位置
- wait_frames: 残り待ちフレーム数

### tick() 処理

1. `wait_frames > 0` の場合
   - `wait_frames--`
   - 終了（APUエミュレータのフレーム処理のみ実行）

2. `wait_frames == 0` の場合
   - 命令を順に読む
   - WAITまたはENDが出るまで全命令を処理
   - REG_WRITE命令は `apuif_write_reg()` を即座に呼ぶ

3. WAIT命令を読んだ場合
   - `wait_frames = value - 1` (値0x00なら待ち0で次フレームも即処理)
   - tick()終了

### 同一フレーム内イベント

WAITが出るまでに現れたREG_WRITE命令は、全て同一フレーム内で
順序通りに実行される。これにより、1フレーム内の複数レジスタ書き込みの
順序依存性が保持される。

### 再生フロー

```
init:
  apuif_init()
  PC = 0
  wait_frames = 0

loop:
  tick()                    // FMSQ命令を処理、APUレジスタ書き込み
  apuif_process(buffer)     // APUエミュレータでサンプル生成
  apuif_audio_write(buffer) // オーディオ出力
  wait_next_frame()         // 1/60秒待ち
```

---

## データ構造

### INITフレーム

FMSQファイルの先頭にはNSFのINITルーチンで実行されたAPUレジスタ書き込みが
REG_WRITE命令として格納される。再生開始時にこれらが最初に実行され、
APUの初期状態が設定される。

### PLAYフレーム

INIT後、各フレームのAPUレジスタ書き込みがREG_WRITE命令として続く。
フレーム間はWAIT命令で区切られる。APU書き込みがないフレームは
WAIT値の加算で表現される（連続する空フレームは1つのWAITにまとめられる）。

```
[INIT REG_WRITEs...] [WAIT] [Frame1 REG_WRITEs...] [WAIT] [Frame2 REG_WRITEs...] ... [END]
```

---

## 制約

- 拡張チップ (VRC6, VRC7, FDS等) は本仕様の対象外
- DPCMサンプルデータ自体はFMSQに含まない（別途ファイルシステムに配置）
- 128フレーム以上の連続無変化はWAIT命令を複数回使用
- data_sizeフィールドがuint16_tのため、命令列は最大65535バイト
  （約60秒分のデータに相当、それ以上はヘッダ拡張が必要）

---

## 拡張案（将来）

- 拡張チップ対応 (0xE0-0xFD帯を利用)
- data_sizeの32bit拡張（長時間データ対応）
- ループポイント自動検出
- DPCMサンプルデータの埋め込み
