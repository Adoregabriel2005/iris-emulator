1. HARDWARE CORE SPECIFICATIONS

    TOM (64-bit Graphics): GPU (32-bit RISC), Blitter (64-bit), Object Processor (64-bit).

    JERRY (32-bit Audio/DSP): DSP (32-bit RISC), 8KB SRAM, DACs, Timers.

    M68000 (Manager): 13.295 MHz. Handles I/O and bootstraps the RISC cores.

    DRAM: 2MB (Standard).

    Bus: 64-bit data path. All RISC engines and the Blitter compete for this bus.

2. AI EMULATION STEERAGE (INSTRUCTIONS FOR THE AI)

When generating code or debugging for the Jaguar core, you MUST follow these rules:
A. RISC Execution & Pipeline (GPU/DSP)

    Rule: Emulate a 3-stage pipeline (Fetch, Decode, Execute).

    Delay Slot: Every Jump or Branch instruction has a 1-cycle delay slot. The instruction immediately following a jump IS executed.

    Register Scoreboarding: If an instruction depends on the result of a previous load that hasn't finished, the pipe must stall.

    SRAM First: RISC code MUST be emulated as running from the internal 4KB/8KB SRAM. Fetching code from DRAM is significantly slower and should reflect bus contention.

B. Bus Arbitration (The 64-bit Bottleneck)

    Priority Logic: The Object Processor has absolute priority (it must draw the line). The Blitter is second. The M68000 is last.

    Contention: If the GPU and Blitter access DRAM simultaneously, implement a "Bus Grant" logic. Do not allow parallel access in the emulation state if the hardware bus is busy.

C. Blitter Implementation (Phrase-Mode)

    Alignment: The Blitter operates on 64-bit "phrases". If the source or destination is not 8-byte aligned, implement the hardware's internal alignment shift cycles.

    Z-Buffering: Emulate the hardware Z-compare before the pixel write.

D. Object Processor (The Display List)

    Line-by-Line: Do not use a global frame buffer. You must process the Object List every scanline.

    Object Types: Support Bitmap, Scaled Bitmap, Branch, and Stop objects. Branch objects must be checked against the GPU flags or Y-Counter.

3. CRITICAL REGISTER MAP (REFERENCE)
Register	Address	Purpose	AI Instruction
G_CTRL	$F02114	GPU Control	Check for GPU start/stop bits.
D_CTRL	$F1A114	DSP Control	DSP must be held in reset until code is uploaded to SRAM.
VMODE	$F00028	Video Mode	Defines CRY (16-bit) or RGB (24-bit) rendering path.
B_CMD	$F02238	Blitter Command	Triggers the blit operation. Must block the bus.
JOYSTICK	$F14000	Input/Mute	Bit 8 enables/disables Audio DACs.
4. MEMORY MAP CONSTRAINTS

    $000000 - $1FFFFF: System DRAM (2MB).

    $F02000 - $F021FF: GPU Registers.

    $F02200 - $F022FF: Blitter Registers.

    $F03000 - $F03FFF: GPU SRAM (Local).

    $F1A100 - $F1A1FF: DSP Registers.

    $F1B000 - $F1BFFF: DSP SRAM (Local).

5. DEBUGGING PROTOCOL FOR AI

    Timing Check: Is the 68000 running at half the speed of Tom/Jerry?

    Interrupt Latency: Does the External Interrupt from Tom reach the 68000 within 2-4 cycles?

    MDS (Multi-Data Store): Ensure the Blitter handles the 64-bit phrase writes correctly without overlapping neighboring pixels unless specified.

    The "Silent Boot" Bug: If audio is missing, check if the code toggles Bit 8 of $F14000.