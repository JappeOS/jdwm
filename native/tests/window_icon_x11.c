// Interactive protocol fixture: Enter advances each icon/identity state.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xcb/xcb.h>
static xcb_atom_t atom(xcb_connection_t *c, const char *name) {
	xcb_intern_atom_reply_t *r = xcb_intern_atom_reply(c, xcb_intern_atom(c, 0, strlen(name), name), NULL);
	if (!r)
		exit(1);
	xcb_atom_t a = r->atom;
	free(r);
	return a;
}
int main(void) {
	xcb_connection_t *c = xcb_connect(NULL, NULL);
	if (xcb_connection_has_error(c))
		return 1;
	xcb_screen_t *screen = xcb_setup_roots_iterator(xcb_get_setup(c)).data;
	xcb_window_t window = xcb_generate_id(c);
	xcb_create_window(c, XCB_COPY_FROM_PARENT, window, screen->root, 0, 0, 320, 200, 0,
					  XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, 0, NULL);
	const char cls[] = "icon-fixture\0JDWMIconFixture\0";
	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8,
						sizeof(cls) - 1, cls);
	const char title[] = "JDWM client icon fixture";
	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
						sizeof(title) - 1, title);
	xcb_atom_t icon = atom(c, "_NET_WM_ICON");
	uint32_t images[2 + 32 * 32 + 2 + 64 * 64];
	images[0] = images[1] = 32;
	for (int i = 0; i < 32 * 32; ++i)
		images[2 + i] = 0x80ff0000;
	images[1026] = images[1027] = 64;
	for (int i = 0; i < 64 * 64; ++i)
		images[1028 + i] = 0xff0000ff;
	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, icon, XCB_ATOM_CARDINAL, 32, sizeof(images) / 4,
						images);
	xcb_map_window(c, window);
	xcb_flush(c);
	puts("Two-size red/blue icon set before map. Enter: replace with green.");
	getchar();
	uint32_t green[] = {1, 1, 0xff00ff00};
	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, icon, XCB_ATOM_CARDINAL, 32, 3, green);
	xcb_flush(c);
	puts("Enter: delete icon (desktop fallback).");
	getchar();
	xcb_delete_property(c, window, icon);
	xcb_flush(c);
	puts("Enter: malformed icon and class change (fallback, no crash).");
	getchar();
	uint32_t bad[] = {UINT32_MAX, UINT32_MAX};
	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, icon, XCB_ATOM_CARDINAL, 32, 2, bad);
	const char changed[] = "second\0JDWMIconFixtureChanged\0";
	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_CLASS, XCB_ATOM_STRING, 8,
						sizeof(changed) - 1, changed);
	xcb_flush(c);
	puts("Enter: replace and immediately destroy.");
	getchar();
	xcb_change_property(c, XCB_PROP_MODE_REPLACE, window, icon, XCB_ATOM_CARDINAL, 32, 3, green);
	xcb_destroy_window(c, window);
	xcb_flush(c);
	xcb_disconnect(c);
	return 0;
}
