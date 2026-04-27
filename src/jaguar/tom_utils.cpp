
#include "tom_utils.h"
#include "tom.h"         // tomRam8, tomWidth, tomHeight, etc
#include "settings.h"   // VJSettings, vjs
#include "memory.h"     // GET16 macro

uint32_t TOMGetVideoModeWidth(void)
{
    // Constantes do core real
    const uint16_t VMODE = 0x28;
    const uint16_t PWIDTH = 0x0E00;

    // Macros de janela visível (definidas em tom.cpp)
    // Usar os mesmos valores do core real
    #ifdef LEFT_VISIBLE_HC
    const int leftVisibleHC = LEFT_VISIBLE_HC;
    #else
    const int leftVisibleHC = (208 - 16 - (1 * 4));
    #endif
    #ifdef RIGHT_VISIBLE_HC
    const int rightVisibleHC = RIGHT_VISIBLE_HC;
    #else
    const int rightVisibleHC = (leftVisibleHC + (VIRTUAL_SCREEN_WIDTH * 4));
    #endif
    #ifdef LEFT_VISIBLE_HC_PAL
    const int leftVisibleHC_PAL = LEFT_VISIBLE_HC_PAL;
    #else
    const int leftVisibleHC_PAL = (208 - 16 - (-3 * 4));
    #endif
    #ifdef RIGHT_VISIBLE_HC_PAL
    const int rightVisibleHC_PAL = RIGHT_VISIBLE_HC_PAL;
    #else
    const int rightVisibleHC_PAL = (leftVisibleHC_PAL + (VIRTUAL_SCREEN_WIDTH * 4));
    #endif

    uint16_t vmode = GET16(tomRam8, VMODE);
    uint16_t pwidth = ((vmode & PWIDTH) >> 9) + 1;
    bool ntsc = vjs.hardwareTypeNTSC;
    int width = (ntsc ? (rightVisibleHC - leftVisibleHC) : (rightVisibleHC_PAL - leftVisibleHC_PAL)) / pwidth;
    return width;
}
