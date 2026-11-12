#define _GNU_SOURCE
#include "xdg-shell-client-protocol.h"
#include "xdg-toplevel-icon-v1-client-protocol.h"
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct xdg_wm_base *shell;
static struct xdg_toplevel_icon_manager_v1 *icons;
static struct wl_surface *surface;
static struct wl_buffer *content;
static void ping(void *data, struct xdg_wm_base *base, uint32_t serial) {
	(void)data;
	xdg_wm_base_pong(base, serial);
}
static const struct xdg_wm_base_listener shell_listener = {.ping = ping};
static void icon_size(void *d, struct xdg_toplevel_icon_manager_v1 *m, int32_t size) {
	(void)d;
	(void)m;
	(void)size;
}
static void icon_done(void *d, struct xdg_toplevel_icon_manager_v1 *m) {
	(void)d;
	(void)m;
}
static const struct xdg_toplevel_icon_manager_v1_listener icon_listener = {.icon_size = icon_size,
																		   .done = icon_done};
static void top_configure(void *d, struct xdg_toplevel *t, int32_t w, int32_t h, struct wl_array *states) {
	(void)d;
	(void)t;
	(void)w;
	(void)h;
	(void)states;
}
static void top_close(void *d, struct xdg_toplevel *t) {
	(void)d;
	(void)t;
	exit(0);
}
static const struct xdg_toplevel_listener top_listener = {.configure = top_configure, .close = top_close};
static void global(void *data, struct wl_registry *registry, uint32_t id, const char *name,
				   uint32_t version) {
	(void)data;
	(void)version;
	if (!strcmp(name, "wl_compositor"))
		compositor = wl_registry_bind(registry, id, &wl_compositor_interface, 1);
	if (!strcmp(name, "wl_shm"))
		shm = wl_registry_bind(registry, id, &wl_shm_interface, 1);
	if (!strcmp(name, "xdg_wm_base")) {
		shell = wl_registry_bind(registry, id, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(shell, &shell_listener, NULL);
	}
	if (!strcmp(name, "xdg_toplevel_icon_manager_v1")) {
		icons = wl_registry_bind(registry, id, &xdg_toplevel_icon_manager_v1_interface, 1);
		xdg_toplevel_icon_manager_v1_add_listener(icons, &icon_listener, NULL);
	}
}
static void removed(void *data, struct wl_registry *registry, uint32_t id) {
	(void)data;
	(void)registry;
	(void)id;
}
static const struct wl_registry_listener registry_listener = {.global = global, .global_remove = removed};
static struct wl_buffer *buffer(int size, uint32_t color) {
	int fd = memfd_create("jdwm-icon-fixture", MFD_CLOEXEC);
	if (fd < 0 || ftruncate(fd, size * size * 4))
		exit(1);
	uint32_t *pixels = mmap(NULL, size * size * 4, PROT_WRITE, MAP_SHARED, fd, 0);
	if (pixels == MAP_FAILED)
		exit(1);
	for (int i = 0; i < size * size; ++i)
		pixels[i] = color;
	struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size * size * 4);
	struct wl_buffer *result =
		wl_shm_pool_create_buffer(pool, 0, size, size, size * 4, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	munmap(pixels, size * size * 4);
	close(fd);
	return result;
}
static void configure(void *data, struct xdg_surface *xdg, uint32_t serial) {
	(void)data;
	xdg_surface_ack_configure(xdg, serial);
	wl_surface_attach(surface, content, 0, 0);
	wl_surface_damage(surface, 0, 0, 256, 256);
	wl_surface_commit(surface);
}
static const struct xdg_surface_listener surface_listener = {.configure = configure};
int main(void) {
	struct wl_display *display = wl_display_connect(NULL);
	if (!display)
		return 1;
	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	wl_display_roundtrip(display);
	if (!compositor || !shm || !shell || !icons) {
		fputs("Required globals unavailable\n", stderr);
		return 1;
	}
	surface = wl_compositor_create_surface(compositor);
	struct xdg_surface *xdg = xdg_wm_base_get_xdg_surface(shell, surface);
	xdg_surface_add_listener(xdg, &surface_listener, NULL);
	struct xdg_toplevel *toplevel = xdg_surface_get_toplevel(xdg);
	xdg_toplevel_add_listener(toplevel, &top_listener, NULL);
	xdg_toplevel_set_app_id(toplevel, "org.jdwm.IconFixture");
	xdg_toplevel_set_title(toplevel, "JDWM Wayland icon fixture");
	content = buffer(256, 0xff202020);
	struct wl_buffer *small = buffer(32, 0x80800000);
	struct wl_buffer *large = buffer(64, 0xff0000ff);
	struct xdg_toplevel_icon_v1 *icon = xdg_toplevel_icon_manager_v1_create_icon(icons);
	xdg_toplevel_icon_v1_set_name(icon, "jdwm-nonexistent-icon-for-fallback");
	xdg_toplevel_icon_v1_add_buffer(icon, small, 1);
	xdg_toplevel_icon_v1_add_buffer(icon, large, 2);
	xdg_toplevel_icon_manager_v1_set_icon(icons, toplevel, icon);
	xdg_toplevel_icon_v1_destroy(icon);
	wl_surface_commit(surface);
	puts("Missing theme name + two buffers, before map. Enter: reset; Enter again: identity change; Enter "
		 "again: close.");
	int phase = 0;
	while (phase < 3) {
		while (wl_display_prepare_read(display) != 0)
			if (wl_display_dispatch_pending(display) < 0)
				return 1;
		wl_display_flush(display);
		struct pollfd fds[] = {{wl_display_get_fd(display), POLLIN, 0}, {STDIN_FILENO, POLLIN, 0}};
		if (poll(fds, 2, -1) < 0) {
			wl_display_cancel_read(display);
			break;
		}
		if (fds[0].revents & POLLIN)
			wl_display_read_events(display);
		else
			wl_display_cancel_read(display);
		if (wl_display_dispatch_pending(display) < 0)
			break;
		if (fds[1].revents & (POLLIN | POLLHUP)) {
			getchar();
			++phase;
			if (phase == 1)
				xdg_toplevel_icon_manager_v1_set_icon(icons, toplevel, NULL);
			if (phase == 2)
				xdg_toplevel_set_app_id(toplevel, "org.jdwm.IconFixtureChanged");
		}
	}
	xdg_toplevel_destroy(toplevel);
	xdg_surface_destroy(xdg);
	wl_surface_destroy(surface);
	wl_buffer_destroy(content);
	wl_buffer_destroy(small);
	wl_buffer_destroy(large);
	wl_display_roundtrip(display);
	wl_display_disconnect(display);
	return 0;
}
