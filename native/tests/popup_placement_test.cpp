#include <cassert>
#include <vector>
#include "surfaces/popup_placement.hpp"
extern "C" {
#include <wlr/backend/headless.h>
}

int main() {
	wlr_xdg_surface first{}, second{}, menu{}, nested{}, other{};
	for (auto* surface : {&first, &second, &menu, &nested, &other})
		wl_list_init(&surface->popups);
	wlr_xdg_popup a{}, b{}, c{};
	a.base = &menu;
	b.base = &nested;
	c.base = &other;
	wl_list_insert(&first.popups, &a.link);
	wl_list_insert(&menu.popups, &b.link);
	wl_list_insert(&second.popups, &c.link);
	std::vector<wlr_xdg_popup*> visited;
	visit_popup_descendants(&first, [&](auto* popup) { visited.push_back(popup); });
	assert((visited == std::vector<wlr_xdg_popup*>{&a, &b}));

	auto* display = wl_display_create();
	auto* backend = wlr_headless_backend_create(wl_display_get_event_loop(display));
	auto* layout = wlr_output_layout_create(display);
	auto* primary = wlr_headless_add_output(backend, 1920, 1080);
	auto* left = wlr_headless_add_output(backend, 1600, 1200);
	assert(wlr_backend_start(backend));
	wlr_output_state state{};
	wlr_output_state_init(&state);
	wlr_output_state_set_scale(&state, 2);
	assert(wlr_output_commit_state(left, &state));
	wlr_output_state_finish(&state);
	// 800x600 logical, left of and raised relative to primary.
	assert(wlr_output_layout_add(layout, left, -800, -200));
	assert(wlr_output_layout_add(layout, primary, 0, 0));
	wlr_xdg_positioner_rules rules{};
	rules.anchor_rect = {0, 0, 20, 20};
	rules.size = {200, 100};
	rules.anchor = XDG_POSITIONER_ANCHOR_BOTTOM_RIGHT;
	rules.gravity = XDG_POSITIONER_GRAVITY_BOTTOM_RIGHT;
	rules.constraint_adjustment = static_cast<xdg_positioner_constraint_adjustment>(
		XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X | XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_Y);

	auto check = [&](int parent_x, int parent_y, wlr_box expected) {
		wlr_box result{};
		assert(place_xdg_popup(layout, parent_x, parent_y, rules, result));
		assert(wlr_box_equal(&result, &expected));
	};
	check(-20, 375, {-180, -75, 200, 100}); // seam belongs to left monitor
	check(500, 400, {20, 20, 200, 100});    // ordinary placement stays anchored
	check(1890, 1060, {-170, -80, 200, 100});
	check(-20, -260, {-180, 60, 200, 100}); // anchor in a gap above the layout

	rules.constraint_adjustment = XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_FLIP_X;
	check(-20, 30, {-200, 20, 200, 100}); // submenu opens to the left
	rules.constraint_adjustment = XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_RESIZE_Y;
	rules.size.height = 1100;
	check(-600, -200, {20, 20, 200, 580});
	wlr_output_layout_remove(layout, left);
	rules.size.height = 100;
	rules.constraint_adjustment = XDG_POSITIONER_CONSTRAINT_ADJUSTMENT_SLIDE_X;
	check(-300, 30, {300, 20, 200, 100}); // nearest remaining output

	wlr_output_layout_destroy(layout);
	wlr_backend_destroy(backend);
	wl_display_destroy(display);
}
