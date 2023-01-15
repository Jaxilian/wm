#include <X11/Xlib.h>
#include "glad/glx.h"
#include <stdbool.h>
#include <string.h>
#include <unordered_map>
#include <stdlib.h>
#include <stdio.h>

const unsigned int BORDER_WIDTH = 3;
const unsigned long BORDER_COLOR = 0xff0000;
const unsigned long BG_COLOR = 0x0000ff;
const unsigned int SCREEN_BIT_DEPTH = 24;

std::unordered_map<Window, Window> g_clients;

static int      g_root;
static int      g_screen;
static Display* g_display;
static int      g_screen_count = 0;
static bool     g_wm_conflict = false;
static const char* g_disp_name = NULL;


int wm_exists_err(Display* display, XErrorEvent* e){
    if(e->error_code == BadAccess){
        g_wm_conflict = true;
    }
    return 0;
};

int x_err_cb(Display* display, XErrorEvent* e){
    const int MAX_ERROR_TEXT_LENGTH = 1024;
    char error_text[MAX_ERROR_TEXT_LENGTH];
    XGetErrorText(display, e->error_code, error_text, sizeof(error_text));
    printf("\nX Error: \nRequest: %d - %d\nError code: %d - %s\nResource ID: %lu\n\n",
        e->request_code,
        e->request_code,
        e->error_code,
        error_text,
        e->resourceid
    );
    exit(1);

  return 0;
}

void frame(Window w, bool existed_before){
    if (g_clients.count(w)){
        printf("window is already framed!\n");
        exit(1);
    }

    XWindowAttributes attrb;
    XGetWindowAttributes(g_display, w, &attrb);

    if(existed_before){
         if (attrb.override_redirect ||
            attrb.map_state != IsViewable) {return;}
    }

    const Window frame = XCreateSimpleWindow(
        g_display,
        g_root,
        attrb.x,
        attrb.y,
        attrb.width,
        attrb.height,
        BORDER_WIDTH,
        BORDER_COLOR,
        BG_COLOR);

    XSelectInput(
        g_display,
        frame,
        SubstructureRedirectMask | SubstructureNotifyMask);

    XAddToSaveSet(g_display, w);

    XReparentWindow(
        g_display,
        w,
        frame,
        0, 0);

    XMapWindow(g_display, frame);

    g_clients[w] = frame;

     XGrabButton(
      g_display,
      Button1,
      Mod1Mask,
      w,
      false,
      ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
      GrabModeAsync,
      GrabModeAsync,
      None,
      None);
  //   b. Resize windows with alt + right button.
  XGrabButton(
      g_display,
      Button3,
      Mod1Mask,
      w,
      false,
      ButtonPressMask | ButtonReleaseMask | ButtonMotionMask,
      GrabModeAsync,
      GrabModeAsync,
      None,
      None);
  //   c. Kill windows with alt + f4.
  XGrabKey(
      g_display,
      XKeysymToKeycode(g_display, XK_F4),
      Mod1Mask,
      w,
      false,
      GrabModeAsync,
      GrabModeAsync);
  //   d. Switch windows with alt + tab.
  XGrabKey(
      g_display,
      XKeysymToKeycode(g_display, XK_Tab),
      Mod1Mask,
      w,
      false,
      GrabModeAsync,
      GrabModeAsync);

}

void unframe(Window w) {

  const Window frame = g_clients[w];
  XUnmapWindow(g_display, frame);

  XReparentWindow(
      g_display,
      w,
      g_root,
      0, 0);  

  XRemoveFromSaveSet(g_display, w);

  XDestroyWindow(g_display, frame);

  g_clients.erase(w);
}

////////////////// EVENTS ///////////////////////////////////////////

void on_create_notify(const XCreateWindowEvent& e) {}

void on_destroy_notify(const XDestroyWindowEvent& e) {}

void on_reparent_notify(const XReparentEvent& e) {}

void on_map_notify(const XMapEvent& e) {}

void on_unmap_notify(const XUnmapEvent& e) {
    if(!g_clients.count(e.window)) return;
    if(e.event == g_root) return;
    unframe(e.window);
}

void on_configure_notify(const XConfigureEvent& e) {}

void on_map_request(const XMapRequestEvent& e) {
    frame(e.window,false);
    XMapWindow(g_display, e.window);
}

void on_configure_request(const XConfigureRequestEvent& e) {

}

void on_button_pressed(const XButtonEvent& e) {

}

void on_button_release(const XButtonEvent& e) {}

void on_motion_notify(const XMotionEvent& e) {
 
}

void on_key_pressed(const XKeyEvent& e) {
    if ((e.state & Mod1Mask) &&
      (e.keycode == XKeysymToKeycode(g_display, XK_F4))) {
    }
}

void on_key_release(const XKeyEvent& e) {}


/////////////////////////////////////////////////////////////////////

void xl_init(){
    printf("xlayer loading...\n");
    g_display = XOpenDisplay(NULL);
    g_root = DefaultRootWindow(g_display);

    if(!g_display){
        printf("failed to load xlayer due to: No Display found!\n");
        exit(1);
    }
    printf("xlayer: display found\n");

    
    g_screen_count = XScreenCount(g_display);
    if(g_screen_count <= 0){
        printf("xlayer failed: No screen was found\n");
        exit(1);
    }
    printf("xlayer: found %d screens\n", g_screen_count);

    XSetErrorHandler(&wm_exists_err);
    XSelectInput(
        g_display,
        g_root,
        SubstructureRedirectMask | SubstructureNotifyMask
    );
    
    XSync(g_display, false);

    if(g_wm_conflict){
        printf("xlayer failed: a window manager is already in use\n");
        exit(1);
    }
    printf("xlayer: no conflicts found\n");

    XSetErrorHandler(x_err_cb);
    XGrabServer(g_display);

    printf("xlayer: fetching window tree... ");
    Window returned_root, returned_parent;
    Window* top_level_windows;
    unsigned int num_top_level_windows;
    XQueryTree(
        g_display,
        g_root,
        &returned_root,
        &returned_parent,
        &top_level_windows,
        &num_top_level_windows
    );
    printf("success\n");

    printf("xlayer: framing %d windows\n", num_top_level_windows);
    for (unsigned int i = 0; i < num_top_level_windows; ++i) {
        printf("xlayer: framing window %d\n", i);
        frame(top_level_windows[i], true);
    }
    printf("done!\n");
    XFree(top_level_windows);
    XUngrabServer(g_display);

    printf("xlayer loaded: [OK]\n");
}

void xl_listen(){
    for (;;) {
        XEvent e;
        XNextEvent(g_display, &e);
        printf("xlayer: event recieved \n");

        switch (e.type) {
        case CreateNotify:
            on_create_notify(e.xcreatewindow);
            break;
        case DestroyNotify:
            on_destroy_notify(e.xdestroywindow);
            break;
        case ReparentNotify:
            on_reparent_notify(e.xreparent);
            break;
        case MapNotify:
            on_map_notify(e.xmap);
            break;
        case UnmapNotify:
            on_unmap_notify(e.xunmap);
            break;
        case ConfigureNotify:
            on_configure_notify(e.xconfigure);
            break;
        case MapRequest:
            on_map_request(e.xmaprequest);
            break;
        case ConfigureRequest:
            on_configure_request(e.xconfigurerequest);
            break;
        case ButtonPress:
            on_button_pressed(e.xbutton);
            break;
        case ButtonRelease:
            on_button_release(e.xbutton);
            break;
        case MotionNotify:
            // Skip any already pending motion events.
            while (XCheckTypedWindowEvent(
                g_display, e.xmotion.window, MotionNotify, &e)) {}
            on_motion_notify(e.xmotion);
            break;
        case KeyPress:
            on_key_pressed(e.xkey);
            break;
        case KeyRelease:
            on_key_release(e.xkey);
            break;
        default:
        break;
        }
    }
}

void xl_free(){
    
    XCloseDisplay(g_display);
}


int main(){
    xl_init();
    xl_listen();
    xl_free();
}