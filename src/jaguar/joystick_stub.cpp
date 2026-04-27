// joystick_stub.cpp - Minimal joystick stub for Iris build
#include <stdint.h>
#include <string.h>

#include "settings.h"

// External from jaguarcore_link_stubs.cpp
extern unsigned char* joypad0Buttons;
extern unsigned char* joypad1Buttons;
extern VJSettings vjs;

// Local state
static uint8_t joystick_ram[4];
bool audioEnabled = false;
bool joysticksEnabled = false;

// Debug vars - required by various VJ core files (stubbed)
int start_logging = 0;
bool interactiveMode = false;
bool blitterSingleStep = false;
bool bssGo = false;
bool bssHeld = false;
bool GUIKeyHeld = false;
bool keyHeld1 = false, keyHeld2 = false, keyHeld3 = false;
bool iLeft = false, iRight = false, iToggle = false;
int objectPtr = 0;
bool startMemLog = false;
bool doDSPDis = false, doGPUDis = false;
int gpu_start_log = 0;
int op_start_log = 0;
int blit_start_log = 0;
int effect_start = 0;
int effect_start2 = 0, effect_start3 = 0, effect_start4 = 0, effect_start5 = 0, effect_start6 = 0;

// Button definitions
#define BUTTON_0   0
#define BUTTON_1   1
#define BUTTON_2   2
#define BUTTON_3   3
#define BUTTON_4   4
#define BUTTON_5   5
#define BUTTON_6   6
#define BUTTON_7   7
#define BUTTON_8   8
#define BUTTON_9   9
#define BUTTON_s   10
#define BUTTON_d   11
#define BUTTON_U   12
#define BUTTON_D   13
#define BUTTON_L   14
#define BUTTON_R   15
#define BUTTON_A   16
#define BUTTON_B   17
#define BUTTON_C   18
#define BUTTON_OPTION  19
#define BUTTON_PAUSE   20

// Forward declarations
static void JoystickReset(void);

static void JoystickReset(void)
{
    memset(joystick_ram, 0x00, 4);
    if (joypad0Buttons) memset(joypad0Buttons, 0, 21);
    if (joypad1Buttons) memset(joypad1Buttons, 0, 21);
}

uint16_t JoystickReadWord(uint32_t offset)
{
    static uint8_t joypad0Offset[16] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x0C, 0xFF, 0xFF, 0xFF, 0x08, 0xFF, 0x04, 0x00, 0xFF
    };
    static uint8_t joypad1Offset[16] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0xFF, 0xFF, 0xFF, 0x04, 0xFF, 0x08, 0x0C, 0xFF
    };

    offset &= 0x03;

    if (offset == 0)
    {
        if (!joysticksEnabled || !joypad0Buttons || !joypad1Buttons)
            return 0xFFFF;

        uint16_t data = 0xFFFF;
        uint8_t offset0 = joypad0Offset[joystick_ram[1] & 0x0F];
        uint8_t offset1 = joypad1Offset[(joystick_ram[1] >> 4) & 0x0F];

        if (offset0 != 0xFF)
        {
            uint16_t mask[4] = { 0xFEFF, 0xFDFF, 0xFBFF, 0xF7FF };
            uint16_t msk2[4] = { 0xFFFF, 0xFFFD, 0xFFFB, 0xFFF7 };

            for (uint8_t i = 0; i < 4; i++)
                data &= (joypad0Buttons[offset0 + i] ? mask[i] : 0xFFFF);

            data &= msk2[offset0 / 4];
        }

        if (offset1 != 0xFF)
        {
            uint16_t mask[4] = { 0xEFFF, 0xDFFF, 0xBFFF, 0x7FFF };
            uint16_t msk2[4] = { 0xFF7F, 0xFFBF, 0xFFDF, 0xFFEF };

            for (uint8_t i = 0; i < 4; i++)
                data &= (joypad1Buttons[offset1 + i] ? mask[i] : 0xFFFF);

            data &= msk2[offset1 / 4];
        }

        return data;
    }
    else if (offset == 2)
    {
        uint16_t data = 0xFF6F | (vjs.hardwareTypeNTSC ? 0x10 : 0x00);

        if (!joysticksEnabled || !joypad0Buttons || !joypad1Buttons)
            return data;

        uint8_t offset0 = joypad0Offset[joystick_ram[1] & 0x0F];
        uint8_t offset1 = joypad1Offset[(joystick_ram[1] >> 4) & 0x0F];

        if (offset0 != 0xFF)
        {
            offset0 /= 4;
            uint8_t mask[4][2] = { { BUTTON_A, BUTTON_PAUSE }, { BUTTON_B, 0xFF }, { BUTTON_C, 0xFF }, { BUTTON_OPTION, 0xFF } };
            data &= (joypad0Buttons[mask[offset0][0]] ? 0xFFFD : 0xFFFF);

            if (mask[offset0][1] != 0xFF)
                data &= (joypad0Buttons[mask[offset0][1]] ? 0xFFFE : 0xFFFF);
        }

        if (offset1 != 0xFF)
        {
            offset1 /= 4;
            uint8_t mask[4][2] = { { BUTTON_A, BUTTON_PAUSE }, { BUTTON_B, 0xFF }, { BUTTON_C, 0xFF }, { BUTTON_OPTION, 0xFF } };
            data &= (joypad1Buttons[mask[offset1][0]] ? 0xFFF7 : 0xFFFF);

            if (mask[offset1][1] != 0xFF)
                data &= (joypad1Buttons[mask[offset1][1]] ? 0xFFFB : 0xFFFF);
        }

        return data;
    }

    return 0xFFFF;
}

void JoystickWriteWord(uint32_t offset, uint16_t data)
{
    offset &= 0x03;
    joystick_ram[offset + 0] = (data >> 8) & 0xFF;
    joystick_ram[offset + 1] = data & 0xFF;

    if (offset == 0)
    {
        audioEnabled = (data & 0x0100 ? true : false);
        joysticksEnabled = (data & 0x8000 ? true : false);
    }
}

void JoystickInit(void)
{
    JoystickReset();
}

void JoystickExec(void)
{
    // Stub - no-op in headless build
}

void JoystickDone(void)
{
    // Stub - no-op
}
