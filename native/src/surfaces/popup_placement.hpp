#pragma once

extern "C" {
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_xdg_shell.h>
}

// parent_x/y are the parent window-geometry origin in output-layout logical units.
inline bool place_xdg_popup(wlr_output_layout* layout, int parent_x, int parent_y,
                            const wlr_xdg_positioner_rules& rules, wlr_box& geometry) {
	// With zero size and offset wlroots returns the positioner's anchor point.
	auto anchor_rules = rules;
	anchor_rules.size = {};
	anchor_rules.offset = {};

	wlr_box anchor{};
	wlr_xdg_positioner_rules_get_geometry(&anchor_rules, &anchor);
	
	// A right/bottom edge exactly on a monitor seam belongs to the anchor rect.
	double x, y;
	wlr_box_closest_point(&rules.anchor_rect, anchor.x, anchor.y, &x, &y);
	wlr_output_layout_closest_point(layout, nullptr, parent_x + x, parent_y + y, &x, &y);
	auto* output = wlr_output_layout_output_at(layout, x, y);
	if (!output)
		return false;

	wlr_box constraint{};
	wlr_output_layout_get_box(layout, output, &constraint);
	constraint.x -= parent_x;
	constraint.y -= parent_y;
	wlr_xdg_positioner_rules_get_geometry(&rules, &geometry);
	wlr_xdg_positioner_rules_unconstrain_box(&rules, &constraint, &geometry);
	return true;
}

// Walk only popups whose position is relative to this surface, in parent-first order.
template<class Visit>
void visit_popup_descendants(wlr_xdg_surface* parent, const Visit& visit) {
	wlr_xdg_popup* child;
	wl_list_for_each(child, &parent->popups, link) {
		visit(child);
		visit_popup_descendants(child->base, visit);
	}
}
