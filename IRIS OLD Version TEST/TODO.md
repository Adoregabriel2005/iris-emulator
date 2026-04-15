# Jaguar bring-up (Rayman) TODO

- [x] 1) Reduzir flood de logs em `src/jaguar/jaguar_gui_stubs.cpp` (rate-limit)
- [x] 2) Ajustar loop de execução por frame em `src/jaguar/JaguarSystem.cpp` para evitar spin agressivo
- [x] 3) Ajustar init de áudio Jaguar (buffer/ordem) para reduzir ruído/estouro
- [x] 4) Build de validação
- [x] 5) Atualizar TODO com status final

# Jaguar blue-screen fix TODO

- [x] 1) Tornar `JaguarSystem::step()` orientado a `frameDone` (VBL), removendo corte prematuro de 16ms
- [x] 2) Garantir execução de GPU/DSP no mesmo timeslice do 68000 e avanço consistente de eventos (`HandleNextEvent`)
- [x] 3) Adicionar logs de diagnóstico de frame (iterações, timeout, visW/visH, frameDone)
- [x] 4) Build de validação
- [x] 5) Atualizar TODO com status final
