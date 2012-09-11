/**
 * compton.h
 */

// Throw everything in here.

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/Xdamage.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/shape.h>

#if COMPOSITE_MAJOR > 0 || COMPOSITE_MINOR >= 2
#define HAS_NAME_WINDOW_PIXMAP 1
#endif

#define CAN_DO_USABLE 0
#define DEBUG_REPAINT 0
#define DEBUG_EVENTS 0
#define DEBUG_RESTACK 0
#define DEBUG_WINTYPE 0
#define MONITOR_REPAINT 0

#define OPAQUE 0xffffffff
#define REGISTER_PROP "_NET_WM_CM_S"

#define WINDOW_SOLID 0
#define WINDOW_TRANS 1
#define WINDOW_ARGB 2

/**
 * Types
 */

typedef enum {
  WINTYPE_UNKNOWN,
  WINTYPE_DESKTOP,
  WINTYPE_DOCK,
  WINTYPE_TOOLBAR,
  WINTYPE_MENU,
  WINTYPE_UTILITY,
  WINTYPE_SPLASH,
  WINTYPE_DIALOG,
  WINTYPE_NORMAL,
  WINTYPE_DROPDOWN_MENU,
  WINTYPE_POPUP_MENU,
  WINTYPE_TOOLTIP,
  WINTYPE_NOTIFY,
  WINTYPE_COMBO,
  WINTYPE_DND,
  NUM_WINTYPES
} wintype;

typedef struct _ignore {
  struct _ignore *next;
  unsigned long sequence;
} ignore;

typedef struct _win {
  struct _win *next;
  Window id;
  Window client_win;
#if HAS_NAME_WINDOW_PIXMAP
  Pixmap pixmap;
#endif
  XWindowAttributes a;
#if CAN_DO_USABLE
  Bool usable; /* mapped and all damaged at one point */
  XRectangle damage_bounds; /* bounds of damage */
#endif
  int mode;
  int damaged;
  Damage damage;
  Picture picture;
  Picture alpha_pict;
  Picture alpha_border_pict;
  Picture shadow_pict;
  XserverRegion border_size;
  XserverRegion extents;
  Picture shadow;
  int shadow_dx;
  int shadow_dy;
  int shadow_width;
  int shadow_height;
  unsigned int opacity;
  wintype window_type;
  unsigned long damage_sequence; /* sequence when damage was created */
  Bool destroyed;
  unsigned int left_width;
  unsigned int right_width;
  unsigned int top_width;
  unsigned int bottom_width;

  Bool need_configure;
  XConfigureEvent queue_configure;

  /* for drawing translucent windows */
  XserverRegion border_clip;
  struct _win *prev_trans;
} win;

typedef struct _conv {
  int size;
  double *data;
} conv;

typedef struct _fade {
  struct _fade *next;
  win *w;
  double cur;
  double finish;
  double step;
  void (*callback) (Display *dpy, win *w);
  Display *dpy;
} fade;

extern int root_height, root_width;
/**
 * Functions
 */

// inline functions must be made static to compile correctly under clang:
// http://clang.llvm.org/compatibility.html#inline

static inline double normalize_d(double d) {
  if (d > 1.0)
    return 1.0;
  if (d < 0.0)
    return 0.0;

  return d;
}

static int
get_time_in_milliseconds();

static fade *
find_fade(win *w);

static void
dequeue_fade(Display *dpy, fade *f);

static void
cleanup_fade(Display *dpy, win *w);

static void
enqueue_fade(Display *dpy, fade *f);

static void
set_fade(Display *dpy, win *w, double start,
         double finish, double step,
         void(*callback) (Display *dpy, win *w),
         Bool exec_callback, Bool override);

static int
fade_timeout(void);

static void
run_fades(Display *dpy);

static double
gaussian(double r, double x, double y);

static conv *
make_gaussian_map(Display *dpy, double r);

static unsigned char
sum_gaussian(conv *map, double opacity,
             int x, int y, int width, int height);

static void
presum_gaussian(conv *map);

static XImage *
make_shadow(Display *dpy, double opacity,
            int width, int height);

static Picture
shadow_picture(Display *dpy, double opacity, Picture alpha_pict,
               int width, int height, int *wp, int *hp);

static Picture
solid_picture(Display *dpy, Bool argb, double a,
              double r, double g, double b);

static void
discard_ignore(Display *dpy, unsigned long sequence);

static void
set_ignore(Display *dpy, unsigned long sequence);

static int
should_ignore(Display *dpy, unsigned long sequence);

static win *
find_win(Display *dpy, Window id);

static win *
find_toplevel(Display *dpy, Window id);

static Picture
root_tile_f(Display *dpy);

static void
paint_root(Display *dpy);

static XserverRegion
win_extents(Display *dpy, win *w);

static XserverRegion
border_size(Display *dpy, win *w);

static Window
find_client_win(Display *dpy, Window win);

static void
get_frame_extents(Display *dpy, Window w,
                  unsigned int *left,
                  unsigned int *right,
                  unsigned int *top,
                  unsigned int *bottom);

static void
paint_all(Display *dpy, XserverRegion region);

static void
add_damage(Display *dpy, XserverRegion damage);

static void
repair_win(Display *dpy, win *w);

static wintype
get_wintype_prop(Display * dpy, Window w);

static wintype
determine_wintype(Display *dpy, Window w, Window top);

static void
map_win(Display *dpy, Window id,
        unsigned long sequence, Bool fade,
        Bool override_redirect);

static void
finish_unmap_win(Display *dpy, win *w);

#if HAS_NAME_WINDOW_PIXMAP
static void
unmap_callback(Display *dpy, win *w);
#endif

static void
unmap_win(Display *dpy, Window id, Bool fade);

static unsigned int
get_opacity_prop(Display *dpy, win *w, unsigned int def);

static double
get_opacity_percent(Display *dpy, win *w);

static void
determine_mode(Display *dpy, win *w);

static void
set_opacity(Display *dpy, win *w, unsigned long opacity);

static void
add_win(Display *dpy, Window id, Window prev, Bool override_redirect);

static void
restack_win(Display *dpy, win *w, Window new_above);

static void
configure_win(Display *dpy, XConfigureEvent *ce);

static void
circulate_win(Display *dpy, XCirculateEvent *ce);

static void
finish_destroy_win(Display *dpy, Window id);

#if HAS_NAME_WINDOW_PIXMAP
static void
destroy_callback(Display *dpy, win *w);
#endif

static void
destroy_win(Display *dpy, Window id, Bool fade);

static void
damage_win(Display *dpy, XDamageNotifyEvent *de);

static int
error(Display *dpy, XErrorEvent *ev);

static void
expose_root(Display *dpy, Window root, XRectangle *rects, int nrects);

#if DEBUG_EVENTS
static int
ev_serial(XEvent *ev);

static char *
ev_name(XEvent *ev);

static Window
ev_window(XEvent *ev);
#endif

static void
usage();

static void
register_cm(int scr);

inline static void
ev_focus_in(XFocusChangeEvent *ev);

inline static void
ev_focus_out(XFocusChangeEvent *ev);

inline static void
ev_create_notify(XCreateWindowEvent *ev);

inline static void
ev_configure_notify(XConfigureEvent *ev);

inline static void
ev_destroy_notify(XDestroyWindowEvent *ev);

inline static void
ev_map_notify(XMapEvent *ev);

inline static void
ev_unmap_notify(XUnmapEvent *ev);

inline static void
ev_reparent_notify(XReparentEvent *ev);

inline static void
ev_circulate_notify(XCirculateEvent *ev);

inline static void
ev_expose(XExposeEvent *ev);

inline static void
ev_property_notify(XPropertyEvent *ev);

inline static void
ev_damage_notify(XDamageNotifyEvent *ev);

/**
 * Destory the cached border_size of a window.
 */
inline static void win_free_border_size(Display *dpy, win *w) {
  if (w->border_size) {
    set_ignore(dpy, NextRequest(dpy));
    XFixesDestroyRegion(dpy, w->border_size);
    w->border_size = None;
  }
}

/**
 * Get a region of the screen size.
 */
inline static XserverRegion get_screen_region(Display *dpy) {
  XRectangle r;

  r.x = 0;
  r.y = 0;
  r.width = root_width;
  r.height = root_height;
  return XFixesCreateRegion(dpy, &r, 1);
}

/**
 * Copies a region
 */
inline static XserverRegion copy_region(Display *dpy,
    XserverRegion oldregion) {
  XserverRegion region = XFixesCreateRegion(dpy, NULL, 0);

  XFixesCopyRegion(dpy, region, oldregion);

  return region;
}

inline static void
ev_handle(XEvent *ev);

static void
fork_after();

static void
get_atoms();
