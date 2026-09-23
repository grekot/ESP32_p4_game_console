// Emulator: opcjonalny pad USB podlaczony do PC, zrodlo analogowych osi galki.
#pragma once

namespace sim {

// Wychylenie lewej galki pada (-1..1). false = brak pada, wtedy osie ida z klawiszy.
bool gamepad_axes(float& x, float& y);

bool gamepad_present();

}  // namespace sim
