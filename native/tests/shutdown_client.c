// Minimal mapped SHM client, intentionally kept connected through server exit.
// Generate xdg-shell-client-protocol.h and xdg-shell-protocol.c with wayland-scanner.
#define _GNU_SOURCE
#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <wayland-client.h>
#include "xdg-shell-client-protocol.h"

static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct xdg_wm_base *shell;
static struct wl_surface *surface;
static struct wl_buffer *buffer;

static void ping(void *data, struct xdg_wm_base *base, uint32_t serial) {
	(void)data;
	xdg_wm_base_pong(base, serial);
}
static const struct xdg_wm_base_listener shell_listener = {.ping = ping};

static void global(void *data, struct wl_registry *registry, uint32_t name,
                   const char *interface, uint32_t version) {
	(void)data; (void)version;
	if (!strcmp(interface, "wl_compositor"))
		compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
	else if (!strcmp(interface, "wl_shm"))
		shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
	else if (!strcmp(interface, "xdg_wm_base")) {
		shell = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
		xdg_wm_base_add_listener(shell, &shell_listener, NULL);
	}
}
static void removed(void *data, struct wl_registry *registry, uint32_t name) {
	(void)data; (void)registry; (void)name;
}
static const struct wl_registry_listener registry_listener = {global, removed};

static void configure(void *data, struct xdg_surface *xdg, uint32_t serial) {
	(void)data;
	xdg_surface_ack_configure(xdg, serial);
	wl_surface_attach(surface, buffer, 0, 0);
	wl_surface_damage(surface, 0, 0, 64, 64);
	wl_surface_commit(surface);
}
static const struct xdg_surface_listener surface_listener = {.configure = configure};

static void toplevel_configure(void *data, struct xdg_toplevel *toplevel,
                              int32_t width, int32_t height, struct wl_array *states) {
	(void)data; (void)toplevel; (void)width; (void)height; (void)states;
}
static void toplevel_close(void *data, struct xdg_toplevel *toplevel) {
	(void)data; (void)toplevel;
}
static const struct xdg_toplevel_listener toplevel_listener = {
	.configure = toplevel_configure, .close = toplevel_close,
};

int main(void) {
	struct wl_display *display = wl_display_connect(NULL);
	assert(display);
	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, NULL);
	assert(wl_display_roundtrip(display) >= 0);
	assert(compositor && shm && shell);
	int fd = memfd_create("shutdown-client", MFD_CLOEXEC);
	assert(fd >= 0 && ftruncate(fd, 64 * 64 * 4) == 0);
	uint32_t *pixels = mmap(NULL, 64 * 64 * 4, PROT_WRITE, MAP_SHARED, fd, 0);
	assert(pixels != MAP_FAILED);
	for (int i = 0; i < 64 * 64; ++i) pixels[i] = 0xff6688aa;
	struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, 64 * 64 * 4);
	buffer = wl_shm_pool_create_buffer(pool, 0, 64, 64, 64 * 4, WL_SHM_FORMAT_ARGB8888);
	wl_shm_pool_destroy(pool);
	close(fd);
	surface = wl_compositor_create_surface(compositor);
	struct xdg_surface *xdg = xdg_wm_base_get_xdg_surface(shell, surface);
	xdg_surface_add_listener(xdg, &surface_listener, NULL);
	struct xdg_toplevel *toplevel = xdg_surface_get_toplevel(xdg);
	xdg_toplevel_add_listener(toplevel, &toplevel_listener, NULL);
	wl_surface_commit(surface);
	while (wl_display_dispatch(display) >= 0) {}
	wl_display_disconnect(display);
	munmap(pixels, 64 * 64 * 4);
	return 0;
}
