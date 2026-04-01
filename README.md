# family-mruby-music-tools

Family mruby OS 向けの音楽ツール集。
NES APU エミュレータ (nofrendo) を利用した音楽再生のための変換・検証ツールを提供する。

## Tools

### nsf2fmsq

NSF ファイル（ファミコン音源）を FMSQ フォーマットに変換するコマンドラインツール。

game-music-emu を使って NSF を内部的にエミュレーションし、APU レジスタ書き込みを
キャプチャして FMSQ バイナリに変換する。

```
bin/nsf2fmsq <input.nsf> [options]
  -t <track>     Track number (default: 0)
  -d <seconds>   Duration in seconds (default: 60)
  -o <output>    Output file (default: <input>_track<N>.fmsq)
  --dump         Print FMSQ commands to stdout
```

### sdl2_simple_player (fmsq_player)

FMSQ ファイルを SDL2 経由で再生するプレイヤー。WAV エクスポートにも対応。
FMSQ フォーマットの検証用。

```
bin/fmsq_player <file.fmsq>              (SDL2 realtime playback)
bin/fmsq_player <file.fmsq> -o out.wav   (WAV file export)
```

### 6502emu_sdl2_player (nsf_6502_player)

NSF ファイルを 6502 CPU エミュレータで直接再生するプレイヤー。
FMSQ への事前変換なしで NSF を再生できる。
6502 エミュレータと NSF プレイヤーは純粋 C で実装

```
bin/nsf_6502_player <file.nsf> [-t track] [-o output.wav] [-d seconds]
```

## FMSQ Format

FMSQ (Family mruby Sequence) は NES APU 音楽データのコンパクトなバイナリフォーマット。
詳細は [doc/fmsq_format.md](doc/fmsq_format.md) を参照。

- REG_WRITE 命令で APU レジスタ書き込みを順序・値ともに保持
- WAIT 命令でフレーム境界を表現 (60Hz)

## Build

```
rake build     # Build all tools -> bin/
rake clean     # Clean all build directories
rake rebuild   # Clean + build
```

Requirements: cmake, g++, SDL2 (libsdl2-dev)

## Architecture

```
NSF File
  |
  +--[nsf2fmsq]--> FMSQ File --[fmsq_player]--> SDL2 Audio
  |
  +--[nsf_6502_player]--> 6502 CPU + APU Emu --> SDL2 Audio
                          (ESP32 portable)
```

nsf2fmsq は PC 上で NSF を事前変換するアプローチ。
nsf_6502_player は NSF をランタイムで直接実行するアプローチ。
どちらも最終的に nofrendo APU エミュレータで音声を合成する。

## Third-Party Licenses

### game-music-emu

- URL: https://github.com/libgme/game-music-emu
- License: GNU Lesser General Public License v2.1 (LGPL-2.1)
- Copyright: (C) 2003-2009 Shay Green
- Used by: nsf2fmsq (NSF emulation and APU register capture)

### emu2413

- URL: https://github.com/digital-sound-antiques/emu2413
- License: MIT License
- Copyright: (C) 2001-2019 Mitsutaka Okazaki
- Used by: nsf2fmsq (VRC7 FM synthesis emulation, part of game-music-emu)

### Nofrendo APU

- URL: https://github.com/bmarquismarkail/nofrern (originally by Matthew Conte)
- License: GNU Library General Public License v2 (LGPL-2)
- Copyright: (C) 1998-2000 Matthew Conte
- Used by: sdl2_simple_player, 6502emu_sdl2_player (APU sound synthesis)
- Note: Also used in fmruby-graphics-audio component (apu_emu)

### SDL2

- URL: https://www.libsdl.org/
- License: zlib License
- Used by: All player tools (audio output)
