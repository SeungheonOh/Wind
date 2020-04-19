#include <X11/Xlib.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define ABS(N) (((N)<0)?-(N):(N))
#define MAX(x,y) ((x)<(y)?(y):(x))
#define MIN(x,y) ((x)>(y)?(y):(x))

typedef struct client {
	Window window;
	int x, y;
	unsigned int width, height;
} client;

Display *d;
int s;
Window root;
client current;
XButtonEvent mouse;

void outline(int x, int y, unsigned int width, unsigned int height) {
	static int X, Y, W, H; // previous outline
	GC gc = XCreateGC(d, root, GCFunction|GCLineWidth, &(XGCValues){.function = GXinvert, .line_width=3});
	if(!gc) return;
	XSetForeground(d, gc, WhitePixel(d, s));
	XDrawRectangle(d, root, gc, X, Y, W, H);
	XDrawRectangle(d, root, gc, x, y, width, height);
	XFreeGC(d, gc);
	XFlush(d);
	X = x;
	Y = y;
	W = width;
	H = height;
}

void init(){
	Window *child;
	unsigned int nchild;
	XQueryTree(d, root, &(Window){0}, 
			&(Window){0}, &child, &nchild);

	for(unsigned int i = 0; i < nchild; i++) {
		XSelectInput(d, child[i], EnterWindowMask|LeaveWindowMask|SubstructureNotifyMask);
		XMapWindow(d, child[i]);
	}
}

client get_client(Window win){
	client c;

	XGetGeometry(d, win, &(Window){0}, 
			&c.x, &c.y, &c.width, &c.height, 
			&(unsigned int){0}, &(unsigned int){0});

	c.window = win;
	return c;
}

void btn_press(XEvent *e){
	mouse = e->xbutton;

	if(!e->xbutton.subwindow) return;
	current = get_client(e->xbutton.subwindow);
	XRaiseWindow(d, current.window);

	int sd = 0; // Wheel resize
	if(e->xbutton.button == 4) sd = 5;
	else if(e->xbutton.button == 5) sd = -5;

	XMoveResizeWindow(d, current.window, 
			current.x      - sd, current.y      - sd,
			current.width  + sd*2, current.height + sd*2);
}

void btn_release(XEvent *e){
	outline(0, 0, 0, 0);
	XDefineCursor(d, root, XCreateFontCursor(d, 68));
	if(!current.window || mouse.subwindow)return;
	XMoveResizeWindow(d, current.window, 
			MIN(mouse.x_root, e->xbutton.x_root), MIN(mouse.y_root, e->xbutton.y_root), 
			ABS(e->xbutton.x_root - mouse.x_root), ABS(e->xbutton.y_root - mouse.y_root)); 

	current = (client){0};
	mouse = (XButtonEvent){0};
}

void config_request(XEvent *e){
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XConfigureWindow(d, ev->window, ev->value_mask, &(XWindowChanges){
		.x          = ev->x,     .y          = ev->y,
		.width      = ev->width, .height     = ev->height,
		.sibling    = ev->above, .stack_mode = ev->detail
	});
}

void map_request(XEvent *e){
	int x = 0, y = 0;
	if(!XQueryPointer(d, root, &(Window){0}, &(Window){0}, &x, &y, &(int){0}, &(int){0}, &(unsigned int){0})) return;
	Window win = e->xmaprequest.window;
	XSelectInput(d, win, EnterWindowMask|LeaveWindowMask|SubstructureNotifyMask);
	current = get_client(win);
	XMoveWindow(d, win, x - current.width/2, y - current.height/2);
	XMapWindow(d, win);
  XSetWindowBorder(d, win, WhitePixel(d, s));
  XConfigureWindow(d,  win, CWBorderWidth, &(XWindowChanges){.border_width = 2});
	XSetInputFocus(d, e->xcrossing.window, RevertToParent, CurrentTime);
}

void enter_notify(XEvent *e){
	if(e->xcrossing.window)
		XSetInputFocus(d, e->xcrossing.window, RevertToParent, CurrentTime);
}

void motion_notify(XEvent *e) {
	int xd = e->xbutton.x_root - mouse.x_root;
	int yd = e->xbutton.y_root - mouse.y_root;

	if(!current.window || !mouse.subwindow){
		XDefineCursor(d, root, XCreateFontCursor(d, 34));
		outline(MIN(mouse.x_root, e->xbutton.x_root), MIN(mouse.y_root, e->xbutton.y_root), 
			ABS(xd), ABS(yd));
		return;
	}

	XMoveResizeWindow(d, current.window, 
			current.x      + (mouse.button == 1 ? xd : 0),
			current.y      + (mouse.button == 1 ? yd : 0),
			current.width  + (mouse.button == 3 ? xd : 0),
			current.height + (mouse.button == 3 ? yd : 0));
}

int xerror(){ return 0; }

static void (*event_handler[LASTEvent])(XEvent *e) = {
	[ButtonPress]      = btn_press,
	[ButtonRelease]    = btn_release,
	[ConfigureRequest] = config_request,
	[MapRequest]       = map_request,
	[EnterNotify]      = enter_notify,
	[MotionNotify]     = motion_notify
};

int main() {
	if(!(d = XOpenDisplay(0))) exit(1);
	s = DefaultScreen(d);
	root = RootWindow(d, s);

	signal(SIGCHLD, SIG_IGN);
	XSetErrorHandler(xerror);

	XSelectInput(d, root, SubstructureRedirectMask);
	XDefineCursor(d, root, XCreateFontCursor(d, 68));

	init();

	for(int i = 1; i < 7; i++)
		XGrabButton(d, i, Mod4Mask , DefaultRootWindow(d), True, 
				ButtonPressMask|ButtonReleaseMask|PointerMotionMask, 
				GrabModeAsync, GrabModeAsync, 0, 0);

	XEvent ev;
	while(!XNextEvent(d, &ev))
		if(event_handler[ev.type])event_handler[ev.type](&ev);
}

