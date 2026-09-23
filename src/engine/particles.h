// Pula czasteczek o stalym rozmiarze (bez alokacji w petli gry): monety wyskakujace z blokow,
// odlamki cegiel, iskry. Kazda czasteczka ma swoja grawitacje i czas zycia; kind = 0 oznacza wolne miejsce,
// znaczenie pozostalych wartosci kind ustala gra (np. 1 = moneta, 2 = odlamek).
#pragma once

#include <stdint.h>

namespace engine {

template <int N>
class ParticlePool {
public:
    struct Particle {
        float   x = 0, y = 0, vx = 0, vy = 0;
        float   t       = 0;   // pozostaly czas zycia (s)
        float   gravity = 0;   // px/s^2 dodawane do vy co klatke
        uint8_t kind    = 0;   // 0 = wolne
    };

    static constexpr int CAPACITY = N;

    void clear()
    {
        for (Particle& p : items_) p.kind = 0;
    }

    // Zajmuje pierwsze wolne miejsce; gdy brak - czasteczka przepada (efekt wizualny, nie logika gry).
    void spawn(float x, float y, float vx, float vy, uint8_t kind, float lifetime, float gravity)
    {
        for (Particle& p : items_) {
            if (p.kind == 0) {
                p.x = x; p.y = y; p.vx = vx; p.vy = vy;
                p.kind = kind; p.t = lifetime; p.gravity = gravity;
                return;
            }
        }
    }

    // Kolejnosc operacji jak w pierwotnej wersji Lake Mario (grawitacja, potem ruch) - zachowuje slady.
    void update(float dt)
    {
        for (Particle& p : items_) {
            if (p.kind == 0) continue;
            p.t -= dt;
            if (p.t <= 0) { p.kind = 0; continue; }
            p.vy += p.gravity * dt;
            p.x  += p.vx * dt;
            p.y  += p.vy * dt;
        }
    }

    const Particle* begin() const { return items_; }
    const Particle* end() const   { return items_ + N; }

private:
    Particle items_[N];
};

}  // namespace engine
