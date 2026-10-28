#pragma once

#include "damage_history.hpp"

namespace zenith::render {

// Backdrop correctness needs a complete readback dependency graph, including
// retained/clipped desktop widgets and overlays outside JDWM's window stack.
// The current engine has not been validated for incremental rendering here.
// Expanding only nonempty buffer-age repair still leaves current-frame damage
// and empty-repair frames on the incremental path (hover/tooltip regressions).
//
// A null callback disables reuse of the previous layer tree for damage diffing
// in the embedder. This is stronger than supplying a full repair rectangle,
// which still leaves frame damage based on incremental layer-tree comparison.
inline void configure_flutter_damage(FlutterOpenGLRendererConfig& config) {
	config.populate_existing_damage = nullptr;
}

// Publish complete-frame damage as well. Otherwise output clipping could keep
// incorrect pixels even after Flutter rendered the whole scene correctly. Do
// not trust partial/empty engine hints in this compatibility mode. The journal,
// generations, leases and output scheduling remain independent of this policy.
inline Damage flutter_composition_damage(const DamageHistory& history) {
	return history.full(force_full_repaint() ? DamageReason::Forced : DamageReason::Readback);
}

} // namespace zenith::render
