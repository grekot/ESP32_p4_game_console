#include "engine/rng.h"

namespace engine {

namespace {
Rng s_rng;
}

uint32_t Rng::next()
{
    uint32_t x = state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state = x;
    return x;
}

int Rng::range(int a, int b)
{
    if (a > b) { const int t = a; a = b; b = t; }
    const uint32_t span = (uint32_t)(b - a) + 1u;
    if (span == 0) return a;   // caly zakres int
    return a + (int)(next() % span);
}

float Rng::unit()
{
    return (float)(next() >> 8) / 16777216.f;   // 24 bity mantysy
}

Rng& rng() { return s_rng; }

void seed_rng(uint32_t seed)
{
    s_rng.state = seed ? seed : 0x4C414B45u;
}

}  // namespace engine
