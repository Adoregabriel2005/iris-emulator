# Agent Changelog — Íris Emulator
> Log de todas as modificações feitas pelo agente AI.
> **SEMPRE atualizar este arquivo ANTES de modificar qualquer fonte.**
> Formato: data/hora local (America/Sao_Paulo), arquivo, o que foi feito e por quê.

---

## Como usar este log para recuperação

1. Identifique o horário aproximado em que algo quebrou
2. Localize a entrada anterior a esse horário
3. Veja exatamente quais arquivos foram tocados e o que foi alterado
4. Use `git diff <commit>` ou restaure manualmente com base na descrição

---

## 2026-04-12

### 20:11 — Fix de compilação: file.cpp do Virtual Jaguar core
**Motivo:** Agente anterior (sessão das ~19h) adicionou `list(FILTER JAGUAR_SRC EXCLUDE REGEX ".*/file\\.cpp$")` no CMakeLists.txt, quebrando o símbolo `JaguarLoadFile`. Ao remover o filtro, o `file.cpp` tentou compilar com libelf/libdwarf que não existem no Windows.

**Arquivos modificados:**
- `JAGUARCOREREFERENCE/Virtual-Jaguar-Rx-master/src/file.cpp`

**O que foi feito:**
1. Envolveu os `#include` de libelf/libdwarf/ELFManager/DBGManager com `#ifndef IRIS_HEADLESS`
2. Envolveu as declarações de variáveis ELF no topo de `JaguarLoadFile` com `#ifndef IRIS_HEADLESS`
3. Envolveu o bloco `else if (fileType == JST_ELF32)` inteiro com `#ifndef IRIS_HEADLESS`
4. Envolveu `DBGManager_Reset()` com `#ifndef IRIS_HEADLESS`
5. Envolveu o bloco ELF32 dentro de `ParseFileType` com `#ifndef IRIS_HEADLESS`
6. Removeu o `list(FILTER ... file\\.cpp$)` que o agente anterior havia adicionado ao CMakeLists.txt

**Define que habilita o guard:** `IRIS_HEADLESS=1` — já estava definido no CMakeLists via `COMPILE_DEFINITIONS` para todos os arquivos do core VJ.

**Resultado:** `irisemulator.exe` compilou com sucesso (3.058.176 bytes, 20:11). `test_savestate.exe` tem erro de linker não relacionado (sem `main`), não afeta o emulador.

**Estado dos fontes antes desta sessão (commit e13a9bb — 18:16):**
Todos os fontes abaixo estavam intactos e corretos. O problema era APENAS o CMakeLists.txt:
- `src/jaguar/JaguarSystem.cpp` — emulação Jaguar completa, JaguarExecuteNew, save states
- `src/audio/VSTHost.cpp` — cadeia VST3, editor nativo, master gain, SDL wrapper
- `src/ui/MainWindow.cpp` — integração Jaguar, filtros RF/S-Video/scanlines, widescreen
- `src/ui/SettingsDialog.cpp` — configurações Jaguar, controles, VST
- `src/ui/WelcomeDialog.cpp` — wizard de boas-vindas estilo PCSX2
- `src/ui/AboutDialog.cpp` — créditos, licença, histórico de hardware
- `src/ui/DisplayWidget.cpp` — shaders OpenGL, filtros, upscaling, aspect ratio
- `src/ui/JaguarIntroWidget.cpp` — intro HLE do Atari Jaguar antes dos jogos
- `src/SDLInput.cpp` — input Jaguar (JaguarInputState, pollJaguar)

---

### 18:16 — commit e13a9bb: Jaguar HLE boot intro + fix JaguarInputState/pollJaguar
**Arquivos modificados (via git):**
- `CMakeLists.txt` — adicionado JaguarIntroWidget
- `src/SDLInput.cpp` / `.h` — fix JaguarInputState, pollJaguar
- `src/audio/VSTHost.cpp` / `.h` / `vst3_iids.cpp`
- `src/jaguar/JaguarSystem.cpp` / `.h` — JaguarExecuteNew, save states, fix framebuffer RGB32
- `src/ui/AboutDialog.cpp` / `.h`
- `src/ui/DisplayWidget.cpp` / `.h` — shaders, filtros, upscaling
- `src/ui/JaguarIntroWidget.cpp` / `.h` — intro HLE do Jaguar
- `src/ui/MainWindow.cpp` / `.h`
- `src/ui/SettingsDialog.cpp` / `.h`
- `src/ui/WelcomeDialog.cpp` / `.h`

---

### 12:48 — commit 8648219: release v1.18.1 — Jaguar filters, VST editor, save states fix, welcome wizard, about dialog
**Arquivos modificados (via git):**
- `CMakeLists.txt`
- `README.md`
- `src/audio/VSTHost.cpp` / `.h` / `vst3_iids.cpp`
- `src/jaguar/JaguarSystem.cpp` / `.h`
- `src/ui/AboutDialog.cpp` / `.h`
- `src/ui/DisplayWidget.cpp` / `.h`
- `src/ui/MainWindow.cpp` / `.h`
- `src/ui/SettingsDialog.cpp` / `.h`
- `src/ui/WelcomeDialog.cpp` / `.h`

---

## 2026-04-12 (continuação)

### 20:45 — Boot correto pela BIOS do Jaguar (SP+PC via SetBIOS)
**Motivo:** O loadROM estava ignorando os vetores da BIOS. Com `useJaguarBIOS=true`, o PC ficava zerado. Com `false`, pulava direto pro jogo via `jaguarRunAddress`. Nenhum dos dois passava pela BIOS real.

**Arquivos modificados:**
- `src/jaguar/JaguarSystem.cpp` — passo 8 do loadROM: substituído SET32 manual por SetBIOS() quando BIOS ativa

**O que foi feito:**
1. Quando `vjs.useJaguarBIOS = true`: chama `SetBIOS()` que copia os 8 bytes iniciais da ROM em `0xE00000` (SP + PC) para `jaguarMainRAM[0..7]`. O `m68k_pulse_reset()` lê esses vetores e inicia pela BIOS, que roda a intro e depois pula pro jogo.
2. Quando `vjs.useJaguarBIOS = false`: mantém comportamento HLE — SP = DRAM_size, PC = jaguarRunAddress.
3. `SelectBIOS()` já copiava a BIOS para `jagMemSpace + 0xE00000` corretamente — não foi alterado.
4. GPU/OP/Blitter já estavam habilitados via `vjs.GPUEnabled = true` — necessários para a intro renderizar.

**Resultado:** Com BIOS presente e `useJaguarBIOS=true`: tela preta → logo Jaguar → cubo → ATARI → jogo. Sem BIOS: HLE direto como antes.

---

### 20:55 — Carregamento da BIOS do disco em vez do array hardcoded
**Motivo:** O core VJ usa BIOS embutidas em código (jagbios.cpp/jagbios2.cpp). O usuário tem o arquivo real `jagboot.rom` (MD5: bcfe348c565d9dedb173822ee6850dea, 128KB, M-Series) em `D:/ROMS/JAGUAR/[BIOS] Atari Jaguar (World)/`. Precisa carregar do disco para usar a BIOS original.

**Arquivos modificados:**
- `src/jaguar/JaguarSystem.cpp` — nova função `loadBIOSFromDisk()` chamada antes de `SelectBIOS()`. Se o arquivo existir e tiver 0x20000 bytes, copia direto para `jagMemSpace + 0xE00000` e pula o `SelectBIOS()`. Se não existir, cai no fallback do array interno.
- `src/jaguar/JaguarSystem.h` — declaração de `loadBIOSFromDisk()`

**Caminho da BIOS:** configurável via QSettings key `"Jaguar/BIOSPath"`. Default auto-detect: próximo ao .exe ou `D:/ROMS/JAGUAR/[BIOS] Atari Jaguar (World)/jagboot.rom`.

**Resultado esperado:** Com arquivo presente → BIOS real carregada → intro original. Sem arquivo → fallback array interno.

---

### 21:10 — Fix DSP ausente em JaguarExecuteNew + sincronização de subsistemas
**Motivo:** `JaguarExecuteNew` executava 68K + GPU mas não chamava `DSPExec`. Doom/Wolfenstein dependem do DSP para áudio e lógica de jogo. Sem DSP: tela branca, áudio estourado, jogo não inicializa.

**Arquivos modificados:**
- `JAGUARCOREREFERENCE/Virtual-Jaguar-Rx-master/src/jaguar.cpp` — adicionado `DSPExec` dentro do loop de `JaguarExecuteNew`, condicionado a `vjs.DSPEnabled`

**O que foi feito:**
1. Adicionado `if (vjs.DSPEnabled) DSPExec(USEC_TO_RISC_CYCLES(timeToNextEvent));` após o `GPUExec` no loop de `JaguarExecuteNew`
2. DSP roda na mesma fatia de tempo que GPU e 68K — sincronização correta por evento
3. `vjs.DSPEnabled = true` já estava sendo setado no `loadROM` do `JaguarSystem.cpp`

**Resultado esperado:** Doom, Wolfenstein e outros jogos DSP-intensivos devem renderizar corretamente e ter áudio funcional.

---

### 21:30 — Fix blitter: forçar USE_ORIGINAL_BLITTER para Doom/Wolfenstein
**Motivo:** O blitter estava usando `BlitterMidsummer2` (useFastBlitter=false). O Midsummer2 tem bugs conhecidos em phrase mode com DSTEN que corrompem a memória após o primeiro frame. O original é mais compatível com jogos como Doom e Wolfenstein.

**Arquivos modificados:**
- `src/jaguar/JaguarSystem.cpp` — `vjs.useFastBlitter = true` no loadROM para usar o blitter original

**Resultado esperado:** Doom e Wolfenstein devem manter a imagem sem corromper após o primeiro frame.

---

### 21:45 — Fix blitter: forçar USE_ORIGINAL_BLITTER + validação de endereços
**Motivo:** Doom renderiza 1 frame correto e depois tela branca + áudio corrompido.
Diagnóstico: BlitterMidsummer2 (useFastBlitter=false) tem bugs conhecidos em phrase
mode com DSTEN que corrompem memória após o primeiro frame. Além disso, o blitter
não valida endereços de destino, podendo escrever fora da VRAM.

**Arquivos modificados:**
- `src/jaguar/JaguarSystem.cpp` — `vjs.useFastBlitter = true` (usa blitter original)
- `JAGUARCOREREFERENCE/.../blitter.cpp` — guard de endereço em `blitter_generic`:
  antes de WRITE_PIXEL, valida que o endereço de destino está dentro de [0x000000, 0x1FFFFF]
  (DRAM) ou [0xF03000, 0xF03FFF] (GPU RAM). Escritas fora dessas regiões são ignoradas.

**Resultado esperado:** Doom mantém imagem sem corromper o framebuffer após o 1º frame.

---

## Template para próximas entradas

```
### HH:MM — Descrição curta
**Motivo:** Por que essa mudança foi necessária.

**Arquivos modificados:**
- `caminho/do/arquivo.cpp` — o que mudou nele

**O que foi feito:**
1. Passo a passo detalhado
2. ...

**Resultado:** Compilou / não compilou / comportamento esperado.
```
