#if !defined(__AQUA_LIB__AQUABSD_ALPS_WM)
#define __AQUA_LIB__AQUABSD_ALPS_WM

#include <aquabsd/alps/win.h>

// note on WM_CB_CLICK: this callback is intended as a way to tell the WM device that a certain click was intended for the WM and not a client, based on its coordinates
// this is *not* a way for the the WM to detect mouse presses and do event processing;
// you should use the mouse device for that instead

typedef enum {
	WM_CB_CREATE,
	WM_CB_SHOW,
	WM_CB_HIDE,
	WM_CB_MODIFY,
	WM_CB_DELETE,
	WM_CB_FOCUS,
	WM_CB_STATE,
	WM_CB_CAPTION,
	WM_CB_DWD,
	WM_CB_CLICK,
	WM_CB_LEN
} wm_cb_t;

typedef uint64_t wm_t;

device_t wm_device = -1;

int wm_init(void) {
	if (wm_device == -1) {
		wm_device = query_device("aquabsd.alps.wm");
	}

	if (wm_device == -1) {
		return -1; // failed to query device
	}

	return 0;
}

wm_t wm_create(void) {
	if (wm_init() < 0) {
		return 0;
	}

	return send_device(wm_device, 0x6377, NULL);
}

void wm_delete(wm_t wm) {
	send_device(wm_device, 0x6477, (uint64_t[]) { wm });
}

win_t* wm_get_root(wm_t wm) {
	if (win_init() < 0) {
		return NULL;
	}

	// get root window information

	uint64_t _win = send_device(wm_device, 0x7277, (uint64_t[]) { wm });

	uint64_t x_res = send_device(wm_device, 0x7872, (uint64_t[]) { wm });
	uint64_t y_res = send_device(wm_device, 0x7972, (uint64_t[]) { wm });

	// create window object

	win_t* win = calloc(1, sizeof *win);
	win->win = _win;

	win->x_res = x_res;
	win->y_res = y_res;

	return win;
}

char* wm_get_cursor(wm_t wm) {
	return (char*) send_device(wm_device, 0x6475, (uint64_t[]) { wm });
}

float wm_get_cursor_xhot(wm_t wm) {
	uint64_t xhot = send_device(wm_device, 0x7868, (uint64_t[]) { wm });
	return *(float*) &xhot;
}

float wm_get_cursor_yhot(wm_t wm) {
	uint64_t yhot = send_device(wm_device, 0x7968, (uint64_t[]) { wm });
	return *(float*) &yhot;
}

void wm_set_name(wm_t wm, const char* name) {
	send_device(wm_device, 0x736E, (uint64_t[]) { wm, (uint64_t) name });
}

// these are different functions because WM_CB_CLICK callbacks have a bit of a different signature

static inline int _wm_register_generic_cb(wm_t wm, wm_cb_t type, int (*cb) (), void* param) {
	return send_device(wm_device, 0x7263, (uint64_t[]) {wm, type, (uint64_t) cb, (uint64_t) param});
}

int wm_register_cb(wm_t wm, wm_cb_t type, int (*cb) (uint64_t wm, uint64_t win, uint64_t param), void* param) {
	return _wm_register_generic_cb(wm, type, cb, param);
}

int wm_register_cb_click(wm_t wm, int (*cb) (uint64_t x, uint64_t y, uint64_t param), void* param) {
	return _wm_register_generic_cb(wm, WM_CB_CLICK, cb, param);
}

void wm_make_compositing(wm_t wm) {
	send_device(wm_device, 0x6D63, (uint64_t[]) { wm });
}

// provider stuff
// in AQUA-land & in the context of window managers, "providers" are your individual monitors;
// that is to say they have XY coordinates and a certain resolution

int wm_provider_count(wm_t wm) {
	return send_device(wm_device, 0x7064, (uint64_t[]) { wm });
}

int wm_provider_x(wm_t wm, unsigned id) {
	return send_device(wm_device, 0x7078, (uint64_t[]) { wm, id });
}

int wm_provider_y(wm_t wm, unsigned id) {
	return send_device(wm_device, 0x7079, (uint64_t[]) { wm, id });
}

int wm_provider_x_res(wm_t wm, unsigned id) {
	return send_device(wm_device, 0x707A, (uint64_t[]) { wm, id });
}

int wm_provider_y_res(wm_t wm, unsigned id) {
	return send_device(wm_device, 0x707B, (uint64_t[]) { wm, id });
}

// useful provider macros
// yeah this BEGIN/END stuff is a bit ugly, but what can you do

#define WM_ITERATE_PROVIDERS_BEGIN(wm) { \
	int provider_count = wm_provider_count((wm)); \
	\
	while (provider_count --> 0) { \
		int x = wm_provider_x((wm), provider_count); \
		int y = wm_provider_y((wm), provider_count); \
		\
		int x_res = wm_provider_x_res((wm), provider_count); \
		int y_res = wm_provider_y_res((wm), provider_count);

#define WM_ITERATE_PROVIDERS_END }}

#endif
