#include "FramePacer.h"
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

static void check(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

int main()
{
    // 1000 GUI ticks per second must still yield only real-time console frames.
    for (const int64_t period : {int64_t(16683333), int64_t(20000000), int64_t(33333333)}) {
        FramePacer clock;
        int frames = 0;
        for (int64_t now = 0; now < 10000000000LL; now += 1000000)
            frames += clock.shouldRun(now, period);
        const int expected = period == 20000000 ? 500 : period == 33333333 ? 300 : 600;
        check(frames == expected, "NTSC/PAL/30 frame cadence over ten seconds");
        const int64_t resumed = 60000000000LL;
        check(clock.shouldRun(resumed, period), "late host can resume");
        check(!clock.shouldRun(resumed + 1000000, period), "late host cannot burst to catch up");
        clock.reset();
        check(clock.shouldRun(0, period), "pause/reload starts a new clock");
        check(!clock.shouldRun(1, period), "resume does not duplicate a frame");
    }
    for (const int64_t period : {int64_t(16683333), int64_t(20000000)}) {
        FramePacer console, display;
        int steps = 0, presented = 0;
        for (int64_t now = 0; now < 10000000000LL; now += 1000000) {
            if (console.shouldRun(now, period)) {
                ++steps;
                presented += display.shouldRun(now, 33333333);
            }
        }
        check(steps == (period == 20000000 ? 500 : 600), "30 FPS presentation must preserve console speed");
        check(presented == 300, "presentation capped at 30 FPS independently of NTSC/PAL");
    }
    {
        FramePacer clock;
        constexpr int64_t period = 16683333;
        check(clock.shouldRun(0, period, 2), "emulation starts immediately");
        int recovered = 0;
        while (clock.shouldRun(1000000000, period, 2)) {
            check(++recovered <= 3, "catch-up work must remain bounded after a long stall");
        }
        check(recovered == 3, "recover at most two missed frames plus the current frame");
        check(!clock.shouldRun(1000000001, period, 2), "catch-up must stop at the real-time deadline");
        clock.reset();
        int steps = 0;
        for (int64_t now = 0; now < 10000000000LL; now += 1000000) {
            if (now % 1000000000 < 40000000) continue; // brief GUI stall each second
            while (clock.shouldRun(now, period, 2)) ++steps;
        }
        check(steps >= 599 && steps <= 600, "brief stalls must not make emulation drift or accelerate");
    }
    std::puts("PASS: NTSC/PAL pacing, 30 FPS presentation, delayed host and pause/reload");
}
