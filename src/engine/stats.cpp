#include "engine/stats.h"

namespace engine {

namespace {
int64_t s_t0     = 0;
int     s_frames = 0;
float   s_fps    = 0.f;
}  // namespace

void stats_tick(int64_t now_us)
{
    if (s_t0 == 0) {
        s_t0 = now_us;
        return;
    }
    ++s_frames;
    const int64_t dt = now_us - s_t0;
    if (dt >= 1000000) {
        s_fps    = (float)s_frames * 1e6f / (float)dt;
        s_frames = 0;
        s_t0     = now_us;
    }
}

float current_fps()
{
    return s_fps;
}

}  // namespace engine
