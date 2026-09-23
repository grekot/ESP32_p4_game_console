// Fizyczne przyciski plytki. Na razie tylko BOOT (GPIO35) - uzywany jako START/PAUZA.
// Docelowo: przyciski pada na zlaczu JP1 (pins::EXP_IO).
#pragma once

namespace board::buttons {

void init();
bool boot_pressed();

}  // namespace board::buttons
