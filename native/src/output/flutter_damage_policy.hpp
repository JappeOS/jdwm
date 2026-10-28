#pragma once

#include "damage_history.hpp"

namespace zenith::render {

// The current Flutter engine's DiffContext::ComputeDamage expands readbacks
// against frame_damage, but not accumulated_buffer_damage. A repair rectangle
// can therefore intersect an unchanged backdrop whose input is outside that
// rectangle. Repainting it can sample previously composited (already filtered
// or tinted) pixels instead of the background under the backdrop.
//
// The embedder has no readback dependency graph, including for desktop widgets
// outside JDWM's window stack. Any nonempty partial repair must cover the whole
// target. Keep an explicit empty repair empty: current-frame damage/readbacks
// are still calculated by Flutter. Do not apply this policy to frame deltas or
// output damage, which do not re-execute Flutter's backdrop filters.
inline Damage flutter_buffer_repair(const DamageHistory& history, Damage repair) {
	if (repair.empty()) return repair;
	if (repair.rect.left <= 0 && repair.rect.top <= 0 &&
	    repair.rect.right >= history.width() && repair.rect.bottom >= history.height()) {
		return repair;
	}
	return history.full(DamageReason::Readback);
}

} // namespace zenith::render
