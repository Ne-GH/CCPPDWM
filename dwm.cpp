#include <locale.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <string>

#include "Dwm.h"
#include "Client.h"
#include "Monitor.h"
#include "drw.h"
#include "util.h"


/* variables */
static const char broken[] = "broken";
static char stext[256];
static int screen;
static int sw, sh;           /* X display screen geometry width, height */
static int bh;               /* bar height */
static int lrpad;            /* sum of left and right padding for text */
static int (*xerrorxlib)(Display *, XErrorEvent *);
static unsigned int numlockmask = 0;
static void (*handler[LASTEvent]) (XEvent *) = {
    [0] = nullptr,
    [1] = nullptr,
    [KeyPress] = keypress,//2 键盘事件
    [3] = nullptr,
    [ButtonPress] = buttonpress,//4 ,鼠标点击事件
    [5] = nullptr,
    [MotionNotify] = motionnotify,//6 鼠标移动事件
    [EnterNotify] = enternotify,//7 鼠标进入窗口事件
    [8] = nullptr,
    [FocusIn] = focusin,//9 处理焦点 TODO
    [10] = nullptr,
    [11] = nullptr,
    [Expose] = expose,//12 处理曝光事件,此处重绘了事件对应的监视器上的状态栏
    [13] = nullptr,
    [14] = nullptr,
    [15] = nullptr,
    [16] = nullptr,
    [DestroyNotify] = destroynotify,//17 处理 X Window 系统中窗口销毁通知事件 XDestroyWindowEvent
    [UnmapNotify] = unmapnotify,//18 unmapnotify 函数用于处理窗口取消映射通知事件。
    [19] = nullptr,
    [MapRequest] = maprequest,//20 // 处理 X Window 系统中的窗口映射请求事件 (XMapRequestEvent)
    [21] = nullptr,
    [ConfigureNotify] = configurenotify,//22 处理 X Window 系统中的窗口配置通知事件 (XConfigureEvent)，重排窗口等
    [ConfigureRequest] = configurerequest,//23
    [24] = nullptr,
    [25] = nullptr,
    [26] = nullptr,
    [27] = nullptr,
    [PropertyNotify] = propertynotify,//28
    [29] = nullptr,
    [30] = nullptr,
    [31] = nullptr,
    [32] = nullptr,
    [ClientMessage] = clientmessage,//33 处理客户端窗口发送的客户端消息事件
    [MappingNotify] = mappingnotify,//34
};

static Atom wmatom[WMLast], netatom[NetLast];
static int running = 1;
static Cur *cursor[CurLast];
static Clr **scheme;
static Display *dpy;
static Drw *drw;
// mons监视器链表,selmon当前选中的监视器
static Monitor *mons, *selmon;
// root根窗口，wmcheckwin辅助窗口
static Window root, wmcheckwin;

/* configuration, allows nested code to access above variables */
/* appearance */
static const unsigned int borderpx  = 1;        /* border pixel of windows */
static const unsigned int snap      = 32;       /* snap pixel */
static const int showbar            = 1;        /* 0 means no bar */
static const int topbar             = 1;        /* 0 means bottom bar */
static const char *fonts[]          = { "monospace:size=10" };
static const char dmenufont[]       = "monospace:size=10";
static const char col_gray1[]       = "#222222";
static const char col_gray2[]       = "#444444";
static const char col_gray3[]       = "#bbbbbb";
static const char col_gray4[]       = "#eeeeee";
static const char col_cyan[]        = "#005577";
static const char *colors[][3]      = {
        /*               fg         bg         border   */
        [SchemeNorm] = { col_gray3, col_gray1, col_gray2 },
        [SchemeSel]  = { col_gray4, col_cyan,  col_cyan  },
};


static const Rule rules[] = {
        /* xprop(1):
         *	WM_CLASS(STRING) = instance, class
         *	WM_NAME(STRING) = title
         */
        /* class      instance    title       tags mask     isfloating   monitor */
        { "Gimp",     NULL,       NULL,       0,            1,           -1 },
        { "Firefox",  NULL,       NULL,       1 << 8,       0,           -1 },
};

/* layout(s) */
static const float mfact     = 0.55; /* factor of master area size [0.05..0.95] */
static const int nmaster     = 1;    /* number of clients in master area */
static const int resizehints = 1;    /* 1 means respect size hints in tiled resizals */
static const int lockfullscreen = 1; /* 1 will force focus on the fullscreen window */

static const Layout layouts[] = {
        /* symbol     arrange function */
        { "[]=",      tile },    /* first entry is default */
        { "><>",      NULL },    /* no layout function means floating behavior */
        { "[M]",      monocle },
};

/* key definitions */
#define MODKEY Mod1Mask
#define TAGKEYS(KEY,TAG) \
	{ MODKEY,                       KEY,      view,           {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask,           KEY,      toggleview,     {.ui = 1 << TAG} }, \
	{ MODKEY|ShiftMask,             KEY,      tag,            {.ui = 1 << TAG} }, \
	{ MODKEY|ControlMask|ShiftMask, KEY,      toggletag,      {.ui = 1 << TAG} },

/* helper for spawning shell commands in the pre dwm-5.0 fashion */
#define SHCMD(cmd) { .v = (const char*[]){ "/bin/sh", "-c", cmd, NULL } }

/* commands */
static const char *dmenucmd[] = { "dmenu_run", "-fn", dmenufont, "-nb", col_gray1, "-nf", col_gray3, "-sb", col_cyan, "-sf", col_gray4, NULL };
static const char *termcmd[]  = { "st", NULL };

static const Key keys[] = {
        /* modifier                     key        function        argument */
        { MODKEY,                       XK_p,      spawn,          {.v = dmenucmd } },
        { MODKEY|ShiftMask,             XK_Return, spawn,          {.v = termcmd } },
        { MODKEY,                       XK_b,      togglebar,      {0} },
        { MODKEY,                       XK_j,      focusstack,     {.i = +1 } },
        { MODKEY,                       XK_k,      focusstack,     {.i = -1 } },
        { MODKEY,                       XK_i,      incnmaster,     {.i = +1 } },
        { MODKEY,                       XK_d,      incnmaster,     {.i = -1 } },
        { MODKEY,                       XK_h,      setmfact,       {.f = -0.05} },
        { MODKEY,                       XK_l,      setmfact,       {.f = +0.05} },
        { MODKEY,                       XK_Return, zoom,           {0} },
        { MODKEY,                       XK_Tab,    view,           {0} },
        { MODKEY|ShiftMask,             XK_c,      killclient,     {0} },
        { MODKEY,                       XK_t,      setlayout,      {.v = &layouts[0]} },
        { MODKEY,                       XK_f,      setlayout,      {.v = &layouts[1]} },
        { MODKEY,                       XK_m,      setlayout,      {.v = &layouts[2]} },
        { MODKEY,                       XK_space,  setlayout,      {0} },
        { MODKEY|ShiftMask,             XK_space,  togglefloating, {0} },
        { MODKEY,                       XK_0,      view,           {.ui = (unsigned int)~0 } },
        { MODKEY|ShiftMask,             XK_0,      tag,            {.ui = (unsigned int)~0 } },
        { MODKEY,                       XK_comma,  focusmon,       {.i = -1 } },
        { MODKEY,                       XK_period, focusmon,       {.i = +1 } },
        { MODKEY|ShiftMask,             XK_comma,  tagmon,         {.i = -1 } },
        { MODKEY|ShiftMask,             XK_period, tagmon,         {.i = +1 } },
        TAGKEYS(                        XK_1,                      0)
        TAGKEYS(                        XK_2,                      1)
        TAGKEYS(                        XK_3,                      2)
        TAGKEYS(                        XK_4,                      3)
        TAGKEYS(                        XK_5,                      4)
        TAGKEYS(                        XK_6,                      5)
        TAGKEYS(                        XK_7,                      6)
        TAGKEYS(                        XK_8,                      7)
        TAGKEYS(                        XK_9,                      8)
        { MODKEY|ShiftMask,             XK_q,      quit,           {0} },
};

/* button definitions */
/* click can be ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle, ClkClientWin, or ClkRootWin */
static const Button buttons[] = {
        /* click                event mask      button          function        argument */
        { ClkLtSymbol,          0,              Button1,        setlayout,      {0} },
        { ClkLtSymbol,          0,              Button3,        setlayout,      {.v = &layouts[2]} },
        { ClkWinTitle,          0,              Button2,        zoom,           {0} },
        { ClkStatusText,        0,              Button2,        spawn,          {.v = termcmd } },
        { ClkClientWin,         MODKEY,         Button1,        movemouse,      {0} },
        { ClkClientWin,         MODKEY,         Button2,        togglefloating, {0} },
        { ClkClientWin,         MODKEY,         Button3,        resizemouse,    {0} },
        { ClkTagBar,            0,              Button1,        view,           {0} },
        { ClkTagBar,            0,              Button3,        toggleview,     {0} },
        { ClkTagBar,            MODKEY,         Button1,        tag,            {0} },
        { ClkTagBar,            MODKEY,         Button3,        toggletag,      {0} },
};



/* tagging */
static const char *tags[] = { "1", "2", "3", "4", "5", "6", "7", "8", "9" };
//#define TAGMASK                 ((1 << LENGTH(tags)) - 1)
#define TAGMASK                 ((1 << 9) - 1)


/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags { char limitexceeded[LENGTH(tags) > 31 ? -1 : 1]; };

int XError(Display *dpy, XErrorEvent *ee)
{
    if (ee->error_code == BadWindow
        || (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
        || (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
        || (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
        || (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
        || (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
        || (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
        || (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
        || (ee->request_code == X_CopyArea && ee->error_code == BadDrawable))
        return 0;
    fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
            ee->request_code, ee->error_code);
    return xerrorxlib(dpy, ee); /* may call exit */
}

/*******************************************************************************
 * 通过window编号在所有窗口中找到指向这个窗口的指针
*******************************************************************************/
Client* Client::wintoclient(Window w) {
    // 遍历所有监视器
    for (Monitor *m = mons; m; m = m->next)
        // 遍历所有窗口
        for (Client *c = m->clients; c; c = c->next)
            if (c->win == w)
                return c;
    return nullptr;
}

void Client::configure() {
    // 用于通知x11服务器对窗口进行重新配置
    XConfigureEvent ce;
    // 通知事件
    ce.type = ConfigureNotify;
    ce.display = dpy;
    // 事件窗口
    ce.event = win;
    // window和event相同，也是客户端窗口
    ce.window = win;
    ce.x = x;
    ce.y = y;
    ce.width = w;
    ce.height = h;
    ce.border_width = bw;
    // 窗口的兄弟窗口，设置为 None 表示没有兄弟窗口。
    ce.above = None;
    // 覆盖重定向标志，设置为 False 表示窗口不会被覆盖重定向。
    ce.override_redirect = False;
    // 将这个 ConfigureNotify 事件发送给客户端窗口 c->win，
    // 并指定了事件掩码 StructureNotifyMask，以通知 X11 服务器重新配置客户端窗口。
    XSendEvent(dpy, win, False, StructureNotifyMask, (XEvent *)&ce);
}

/*******************************************************************************
 * 更新窗口的标题
*******************************************************************************/
void Client::updatetitle() {
    // 从窗口的_NET_WM_NAME属性中获取窗口的名称并存储到c->name中
    if (!gettextprop(win, netatom[NetWMName], name, sizeof name))
        gettextprop(win, XA_WM_NAME, name, sizeof name);
    // 如果没有获取到窗口的名称,将窗口的名称设置为“broken”
    if (name[0] == '\0') /* hack to mark broken clients,标记损坏的客户端 */
        strcpy(name, broken);
}

/*******************************************************************************
 * 更新窗口属性
*******************************************************************************/
void Client::updatewindowtype() {
    Atom state = ::getatomprop(this, netatom[NetWMState]);
    Atom wtype = ::getatomprop(this, netatom[NetWMWindowType]);

    if (state == netatom[NetWMFullscreen])
        setfullscreen(1);
    if (wtype == netatom[NetWMWindowTypeDialog])
        isfloating = 1;
}

void Client::updatewmhints() {
    XWMHints *wmh;
    if ((wmh = XGetWMHints(dpy, win))) {
        if (this == selmon->sel && wmh->flags & XUrgencyHint) {
            wmh->flags &= ~XUrgencyHint;
            XSetWMHints(dpy, win, wmh);
        } else
            isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
        if (wmh->flags & InputHint)
            neverfocus = !wmh->input;
        else
            neverfocus = 0;
        XFree(wmh);
    }
}

void Client::setfocus() {
    if (!neverfocus) {
        XSetInputFocus(dpy, win, RevertToPointerRoot, CurrentTime);
        XChangeProperty(dpy, root, netatom[NetActiveWindow],
                        XA_WINDOW, 32, PropModeReplace,
                        (unsigned char *) &(win), 1);
    }
    sendevent(wmatom[WMTakeFocus]);
}

void Client::showhide() {

}

void Client::updatesizehints() {
    long msize;
    XSizeHints size;

    if (!XGetWMNormalHints(dpy, win, &size, &msize))
        /* size is uninitialized, ensure that size.flags aren't used */
        size.flags = PSize;
    if (size.flags & PBaseSize) {
        basew = size.base_width;
        baseh = size.base_height;
    } else if (size.flags & PMinSize) {
        basew = size.min_width;
        baseh = size.min_height;
    } else
        basew = baseh = 0;
    if (size.flags & PResizeInc) {
        incw = size.width_inc;
        inch = size.height_inc;
    } else
        incw = inch = 0;
    if (size.flags & PMaxSize) {
        maxw = size.max_width;
        maxh = size.max_height;
    } else
        maxw = maxh = 0;
    if (size.flags & PMinSize) {
        minw = size.min_width;
        minh = size.min_height;
    } else if (size.flags & PBaseSize) {
        minw = size.base_width;
        minh = size.base_height;
    } else
        minw = minh = 0;
    if (size.flags & PAspect) {
        mina = (float)size.min_aspect.y / size.min_aspect.x;
        maxa = (float)size.max_aspect.x / size.max_aspect.y;
    } else
        maxa = mina = 0.0;
    isfixed = (maxw && maxh && maxw == minw && maxh == minh);
    hintsvalid = 1;
}

void Client::attachstack() {
    snext = mon->stack;
    mon->stack = this;
}

/*******************************************************************************
 * 根据窗口规则设置窗口的属性
*******************************************************************************/
void Client::applyrules() {
    // 存储窗口的类别和实例信息
    const char *_class, *instance;
    Monitor *m;
    XClassHint ch = {nullptr, nullptr };

    /* rule matching */
    // 窗口是否浮动
    isfloating = 0;
    tags = 0;
    // 获取窗口的类别和实例信息，并存储在ch中
    XGetClassHint(dpy, win, &ch);
    _class    = ch.res_class ? ch.res_class : broken;
    instance = ch.res_name  ? ch.res_name  : broken;

    for (int i = 0; i < LENGTH(rules); i++) {
        const Rule *r = &rules[i];
        // 如果规则标题为空或者窗口标题不等于规则标题
        if ((!r->title || strstr(name, r->title))
            // 如果规则的class为空，或者当前class和规则的calss不同
            && (!r->_class || strstr(_class, r->_class))
            // 如果规则的instance为空，或者当前instance和规则的instance不同
            && (!r->instance || strstr(instance, r->instance))) {
            isfloating = r->isfloating;
            tags |= r->tags;
            for (m = mons; m && m->num != r->monitor; m = m->next)
                ;
            if (m)
                mon = m;
        }
    }
    if (ch.res_class)
        XFree(ch.res_class);
    if (ch.res_name)
        XFree(ch.res_name);
    // 代码将客户端窗口的标签进行了重新调整。
    // 它首先检查客户端窗口的标签是否在 TAGMASK 中，
    // 如果在其中，则保持不变。
    // 否则，它将客户端窗口的标签设置为当前监视器的标签集合中的标签。这确保了窗口的标签始终有效，并且不会超出当前监视器的标签集合。
    tags = tags & TAGMASK ? tags & TAGMASK : mon->tagset[mon->seltags];
}

void Client::attach() {
    next = mon->clients;
    mon->clients = this;
}

void Client::detach() {
    Client **tc;

    for (tc = &mon->clients; *tc && *tc != this; tc = &(*tc)->next);
    *tc = next;
}

void Client::detachstack() {
    Client **tc;
    for (tc = &mon->stack; *tc && *tc != this; tc = &(*tc)->snext)
        ;
    *tc = snext;

    Client *t;
    if (this == mon->sel) {
        for (t = mon->stack; t && !ISVISIBLE(t); t = t->snext)
            ;
        mon->sel = t;
    }
}

void Client::focus() {

}

void Client::pop() {
    detach();
    attach();
    ::focus(this);
    arrange(this->mon);
}

Client *Client::nexttiled() {
    return nullptr;
}

void Client::unfocus(int setfocus) {

}

void Client::unmanage(int destroyed) {
    Monitor *m = mon;
    XWindowChanges wc;

    detach();
    detachstack();
    if (!destroyed) {
        wc.border_width = oldbw;
        XGrabServer(dpy); /* avoid race conditions */
        XSetErrorHandler(xerrordummy);
        XSelectInput(dpy, win, NoEventMask);
        XConfigureWindow(dpy, win, CWBorderWidth, &wc); /* restore border */
        XUngrabButton(dpy, AnyButton, AnyModifier, win);
        setclientstate(WithdrawnState);
        XSync(dpy, False);
        XSetErrorHandler(XError);
        XUngrabServer(dpy);
    }
//    free(c);
    delete this;
    ::focus(NULL);
    ::updateclientlist();
    ::arrange(m);
}


int Client::sendevent(Atom proto) {
    int n;
    Atom *protocols;
    int exists = 0;
    XEvent ev;

    if (XGetWMProtocols(dpy, win, &protocols, &n)) {
        while (!exists && n--)
            exists = protocols[n] == proto;
        XFree(protocols);
    }
    if (exists) {
        ev.type = ClientMessage;
        ev.xclient.window = win;
        ev.xclient.message_type = wmatom[WMProtocols];
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = proto;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, win, False, NoEventMask, &ev);
    }
    return exists;
}

void Client::sendmon(Monitor *m) {

}

void Client::setclientstate(long state) {
    long data[] = { state, None };
    XChangeProperty(dpy, win, wmatom[WMState], wmatom[WMState], 32,
                    PropModeReplace, (unsigned char *)data, 2);
}

void Client::setfullscreen(int fullscreen) {
    if (fullscreen && !isfullscreen) {
        XChangeProperty(dpy, win, netatom[NetWMState], XA_ATOM, 32,
                        PropModeReplace, (unsigned char*)&netatom[NetWMFullscreen], 1);
        isfullscreen = 1;
        oldstate = isfloating;
        oldbw = bw;
        bw = 0;
        isfloating = 1;
        ::resizeclient(this, mon->mx, mon->my, mon->mw, mon->mh);
        XRaiseWindow(dpy, win);
    } else if (!fullscreen && isfullscreen){
        XChangeProperty(dpy, win, netatom[NetWMState], XA_ATOM, 32,
                        PropModeReplace, (unsigned char*)0, 0);
        isfullscreen = 0;
        isfloating = oldstate;
        bw = oldbw;
        x = oldx;
        y = oldy;
        w = oldw;
        h = oldh;
        ::resizeclient(this, x, y, w, h);
        ::arrange(mon);
    }
}

/*******************************************************************************
 * 将窗口标记为紧急窗口。
 * 如果 urg 为非零（通常为 1），则窗口被标记为紧急窗口；如果 urg 为零，窗口将取消紧急窗口标记。
*******************************************************************************/
void Client::seturgent(int urg) {
    XWMHints *wmh;
    isurgent = urg;
    // 检查是否成功获取窗口提示（XWMHints）的结构体。
    if (!(wmh = XGetWMHints(dpy, win)))
        return;
    // 传入的 urg 值来更新窗口提示结构体的 flags 字段，以添加或取消紧急窗口提示标志。如果 urg 为非零，表示窗口应标记为紧急窗口，就会设置 XUrgencyHint 标志；如果 urg 为零，表示窗口应取消紧急窗口标志，就会将 XUrgencyHint 标志从 flags 中移除。
    wmh->flags = urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
    XSetWMHints(dpy, win, wmh);
    // 将更新后的窗口提示结构体重新设置到窗口 c->win 上，以便通知窗口管理器或窗口管理器的用户界面环境有关窗口的重要性。
    XFree(wmh);
}

void Client::grabbuttons(int focused) {

}

int Client::applysizehints(int *x, int *y, int *w, int *h, int interact) {
    return 0;
}

Atom Client::getatomprop(Atom prop) {
    return 0;
}


int
applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact)
{
	int baseismin;
	Monitor *m = c->mon;

	/* set minimum possible */
	*w = MAX(1, *w);
	*h = MAX(1, *h);
	if (interact) {
		if (*x > sw)
			*x = sw - WIDTH(c);
		if (*y > sh)
			*y = sh - HEIGHT(c);
		if (*x + *w + 2 * c->bw < 0)
			*x = 0;
		if (*y + *h + 2 * c->bw < 0)
			*y = 0;
	} else {
		if (*x >= m->wx + m->ww)
			*x = m->wx + m->ww - WIDTH(c);
		if (*y >= m->wy + m->wh)
			*y = m->wy + m->wh - HEIGHT(c);
		if (*x + *w + 2 * c->bw <= m->wx)
			*x = m->wx;
		if (*y + *h + 2 * c->bw <= m->wy)
			*y = m->wy;
	}
	if (*h < bh)
		*h = bh;
	if (*w < bh)
		*w = bh;
	if (resizehints || c->isfloating || !c->mon->lt[c->mon->sellt]->arrange) {
		if (!c->hintsvalid)
			c->updatesizehints();
		/* see last two sentences in ICCCM 4.1.2.3 */
		baseismin = c->basew == c->minw && c->baseh == c->minh;
		if (!baseismin) { /* temporarily remove base dimensions */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for aspect limits */
		if (c->mina > 0 && c->maxa > 0) {
			if (c->maxa < (float)*w / *h)
				*w = *h * c->maxa + 0.5;
			else if (c->mina < (float)*h / *w)
				*h = *w * c->mina + 0.5;
		}
		if (baseismin) { /* increment calculation requires this */
			*w -= c->basew;
			*h -= c->baseh;
		}
		/* adjust for increment value */
		if (c->incw)
			*w -= *w % c->incw;
		if (c->inch)
			*h -= *h % c->inch;
		/* restore base dimensions */
		*w = MAX(*w + c->basew, c->minw);
		*h = MAX(*h + c->baseh, c->minh);
		if (c->maxw)
			*w = MIN(*w, c->maxw);
		if (c->maxh)
			*h = MIN(*h, c->maxh);
	}
	return *x != c->x || *y != c->y || *w != c->w || *h != c->h;
}

// 重新排列所有客户端，以适应新的窗口几何信息。
void arrange(Monitor *m) {
	if (m) {
        showhide(m->stack);
    }
	else {
        for (m = mons; m; m = m->next)
            showhide(m->stack);
    }

	if (m) {
        // 根据当前的布局策略来重新排列客户端窗口。不同的布局策略会导致窗口的不同排列方式，例如平铺、浮动等。
		arrangemon(m);
        // 重新排列监视器上的窗口，确保它们的层叠顺序正确。这是因为窗口的层叠顺序可能会受到其他窗口的遮挡或影响。
		restack(m);
	}
    else {
        for (m = mons; m; m = m->next)
            arrangemon(m);
    }
}

/*******************************************************************************
 * 根据当前的布局策略来重新排列客户端窗口。不同的布局策略会导致窗口的不同排列方式，例如平铺、浮动等。
*******************************************************************************/
void arrangemon(Monitor *m) {
	strncpy(m->ltsymbol, m->lt[m->sellt]->symbol, sizeof m->ltsymbol);
	if (m->lt[m->sellt]->arrange)
		m->lt[m->sellt]->arrange(m);
}





/*******************************************************************************
 * 处理鼠标点击事件
*******************************************************************************/
void buttonpress(XEvent *e) {
	Arg arg = {0};
	Client *c;
	Monitor *m;
    // 获取鼠标事件，包括按下的按键、按键状态、鼠标所在窗口
	XButtonPressedEvent *ev = &e->xbutton;

	int click = ClkRootWin;
	/* focus monitor if necessary */
    // 检测点击的监视器是哪一个，如果不是当前监视器
    // 切换监视器并重新聚焦
	if ((m = wintomon(ev->window)) && m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
    // 如果点击的窗口是当前监视器中的任务栏,TODO
	if (ev->window == selmon->barwin) {
        unsigned int i = 0,x = 0;
        // 判断点击区域
		do
			x += TEXTW(tags[i]);
		while (ev->x >= x && ++i < LENGTH(tags));

        // 如果点击的是tag
		if (i < LENGTH(tags)) {
			click = ClkTagBar;
			arg.ui = 1 << i;
		}
        // 如果点击的是layout
        else if (ev->x < x + TEXTW(selmon->ltsymbol))
			click = ClkLtSymbol;
        // 如果点击的是状态栏文本
		else if (ev->x > selmon->ww - (int)TEXTW(stext))
			click = ClkStatusText;
        // 如果点击的是窗口标题区域
		else
			click = ClkWinTitle;
	}
    // 如果点击的是普通窗口
    else if ((c = Client::wintoclient(ev->window))) {
		focus(c);
        // 重新排列当前监视器上的窗口
		restack(selmon);
        // 允许X服务器记录并重放事件，确保鼠标点击事件能够正常工作
		XAllowEvents(dpy, ReplayPointer, CurrentTime);
		click = ClkClientWin;
	}
    // 遍历config中定义的鼠标按键
	for (int i = 0; i < LENGTH(buttons); i++)
		if (click == buttons[i].click && buttons[i].func && buttons[i].button == ev->button
		&& CLEANMASK(buttons[i].mask) == CLEANMASK(ev->state))
			buttons[i].func(click == ClkTagBar && buttons[i].arg.i == 0 ? &arg : &buttons[i].arg);
}





void
cleanupmon(Monitor *mon)
{
	Monitor *m;

	if (mon == mons)
		mons = mons->next;
	else {
		for (m = mons; m && m->next != mon; m = m->next);
		m->next = mon->next;
	}
	XUnmapWindow(dpy, mon->barwin);
	XDestroyWindow(dpy, mon->barwin);
	free(mon);
}

/*******************************************************************************
 * 处理客户端窗口发送的客户端消息事件
*******************************************************************************/
void clientmessage(XEvent *e) {
	XClientMessageEvent *cme = &e->xclient;
	Client *c = Client::wintoclient(cme->window);

	if (!c)
		return;
    // 检查接收到的客户端消息事件的类型是否为 _NET_WM_STATE，这通常用于客户端通知窗口状态的变化。
	if (cme->message_type == netatom[NetWMState]) {
        // 检查客户端消息事件中的数据字段，以确定是否涉及全屏状态的变化
        // 如果为 1（表示添加全屏状态）或 2（表示切换全屏状态）
		if (cme->data.l[1] == netatom[NetWMFullscreen]
		|| cme->data.l[2] == netatom[NetWMFullscreen])
			c->setfullscreen((cme->data.l[0] == 1 /* _NET_WM_STATE_ADD    */
				|| (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ && !c->isfullscreen)));
	}
    // 检查接收到的客户端消息事件的类型是否为 _NET_ACTIVE_WINDOW，这通常用于激活或突出显示窗口。
    else if (cme->message_type == netatom[NetActiveWindow]) {
        // 检查客户端窗口是否不是当前选中监视器 selmon 上的选中窗口，并且窗口不是紧急窗口。
		if (c != selmon->sel && !c->isurgent)
            // 将窗口 c 设置为紧急窗口，以突出显示它。
			c->seturgent(1);
	}
}



/*******************************************************************************
 * 处理 X Window 系统中的窗口配置通知事件 (XConfigureEvent)。
 * 重排窗口等
*******************************************************************************/
void configurenotify(XEvent *e) {
	Monitor *m;
	Client *c;
	XConfigureEvent *ev = &e->xconfigure;
	int dirty;

	/* TODO: updategeom handling sucks, needs to be simplified
	 *       updategeom 处理很糟糕，需要简化
	**/
	if (ev->window == root) {
        // 计算变量 dirty，用于表示窗口的宽度或高度是否发生了变化
        // 如果当前的窗口宽度 (sw) 或高度 (sh) 与配置通知事件中的宽度 (ev->width) 或高度 (ev->height) 不相等，则表示窗口的大小发生了变化。
		dirty = (sw != ev->width || sh != ev->height);
		sw = ev->width;
		sh = ev->height;
        // 更新窗口管理器的几何信息（例如监视器的位置和大小），
        // || 检查是否窗口的大小发生了变化 (dirty) 或几何信息需要更新。
		if (updategeom() || dirty) {
			drw_resize(drw, sw, bh);
			updatebars();
			for (m = mons; m; m = m->next) {
				for (c = m->clients; c; c = c->next)
					if (c->isfullscreen)
						resizeclient(c, m->mx, m->my, m->mw, m->mh);
                // 移动和调整监视器上的状态栏的位置和大小，以适应新的监视器几何信息。
				XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
			}
			focus(NULL);
            // 重新排列所有客户端，以适应新的窗口几何信息。
			arrange(NULL);
		}
	}
}

void
configurerequest(XEvent *e)
{
	Client *c;
	Monitor *m;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;
	XWindowChanges wc;

	if ((c = Client::wintoclient(ev->window))) {
		if (ev->value_mask & CWBorderWidth)
			c->bw = ev->border_width;
		else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
			m = c->mon;
			if (ev->value_mask & CWX) {
				c->oldx = c->x;
				c->x = m->mx + ev->x;
			}
			if (ev->value_mask & CWY) {
				c->oldy = c->y;
				c->y = m->my + ev->y;
			}
			if (ev->value_mask & CWWidth) {
				c->oldw = c->w;
				c->w = ev->width;
			}
			if (ev->value_mask & CWHeight) {
				c->oldh = c->h;
				c->h = ev->height;
			}
			if ((c->x + c->w) > m->mx + m->mw && c->isfloating)
				c->x = m->mx + (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
			if ((c->y + c->h) > m->my + m->mh && c->isfloating)
				c->y = m->my + (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
			if ((ev->value_mask & (CWX|CWY)) && !(ev->value_mask & (CWWidth|CWHeight)))
                c->configure();
			if (ISVISIBLE(c))
				XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
		} else
            c->configure();
	} else {
		wc.x = ev->x;
		wc.y = ev->y;
		wc.width = ev->width;
		wc.height = ev->height;
		wc.border_width = ev->border_width;
		wc.sibling = ev->above;
		wc.stack_mode = ev->detail;
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
	}
	XSync(dpy, False);
}

Monitor *
createmon(void)
{
	Monitor *m;

	m = (Monitor *)ecalloc(1, sizeof(Monitor));
	m->tagset[0] = m->tagset[1] = 1;
	m->mfact = mfact;
	m->nmaster = nmaster;
	m->showbar = showbar;
	m->topbar = topbar;
	m->lt[0] = &layouts[0];
	m->lt[1] = &layouts[1 % LENGTH(layouts)];
	strncpy(m->ltsymbol, layouts[0].symbol, sizeof m->ltsymbol);
	return m;
}

/*******************************************************************************
 * 处理X 系统窗口中窗口销毁通知事件 “XDestroyWindowEvent”
*******************************************************************************/
void destroynotify(XEvent *e) {
	Client *c;
	XDestroyWindowEvent *ev = &e->xdestroywindow;
    // 如果获取到这个窗口,就删除这个窗口
	if ((c = Client::wintoclient(ev->window)))
		c->unmanage(1);
}





Monitor *
dirtomon(int dir)
{
	Monitor *m = NULL;

	if (dir > 0) {
		if (!(m = selmon->next))
			m = mons;
	} else if (selmon == mons)
		for (m = mons; m->next; m = m->next);
	else
		for (m = mons; m->next != selmon; m = m->next);
	return m;
}

/*******************************************************************************
 * 重绘特定监视器的状态栏
 * TODO
*******************************************************************************/
void
drawbar(Monitor *m)
{
	int x, w, tw = 0;
	int boxs = drw->fonts->h / 9;
	int boxw = drw->fonts->h / 6 + 2;
	unsigned int i, occ = 0, urg = 0;
	Client *c;

	if (!m->showbar)
		return;

	/* draw status first so it can be overdrawn by tags later */
	if (m == selmon) { /* status is only drawn on selected monitor */
		drw_setscheme(drw, scheme[SchemeNorm]);
		tw = TEXTW(stext) - lrpad + 2; /* 2px right padding */
		drw_text(drw, m->ww - tw, 0, tw, bh, 0, stext, 0);
	}

	for (c = m->clients; c; c = c->next) {
		occ |= c->tags;
		if (c->isurgent)
			urg |= c->tags;
	}
	x = 0;
	for (i = 0; i < LENGTH(tags); i++) {
		w = TEXTW(tags[i]);
		drw_setscheme(drw, scheme[m->tagset[m->seltags] & 1 << i ? SchemeSel : SchemeNorm]);
		drw_text(drw, x, 0, w, bh, lrpad / 2, tags[i], urg & 1 << i);
		if (occ & 1 << i)
			drw_rect(drw, x + boxs, boxs, boxw, boxw,
				m == selmon && selmon->sel && selmon->sel->tags & 1 << i,
				urg & 1 << i);
		x += w;
	}
	w = TEXTW(m->ltsymbol);
	drw_setscheme(drw, scheme[SchemeNorm]);
	x = drw_text(drw, x, 0, w, bh, lrpad / 2, m->ltsymbol, 0);

	if ((w = m->ww - tw - x) > bh) {
		if (m->sel) {
			drw_setscheme(drw, scheme[m == selmon ? SchemeSel : SchemeNorm]);
			drw_text(drw, x, 0, w, bh, lrpad / 2, m->sel->name, 0);
			if (m->sel->isfloating)
				drw_rect(drw, x + boxs, boxs, boxw, boxw, m->sel->isfixed, 0);
		} else {
			drw_setscheme(drw, scheme[SchemeNorm]);
			drw_rect(drw, x, 0, w, bh, 1, 1);
		}
	}
	drw_map(drw, m->barwin, 0, 0, m->ww, bh);
}

void
drawbars(void)
{
	Monitor *m;

	for (m = mons; m; m = m->next)
		drawbar(m);
}

/*******************************************************************************
 * 鼠标进入窗口事件
*******************************************************************************/
void enternotify(XEvent *e) {
	Client *c;
	Monitor *m;
	XCrossingEvent *ev = &e->xcrossing;
    // !(如果事件模式是“进入窗口” 或者 详细信息是“鼠标进入了窗口”)
	if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior)
    // 并且事件的窗口不是根窗口，直接返回
    && ev->window != root)
		return;
    // 获取该窗口
	c = Client::wintoclient(ev->window);
    // 获取监视器
	m = c ? c->mon : wintomon(ev->window);
    // 如果不是当前监视器
	if (m != selmon) {
		unfocus(selmon->sel, 1);
		selmon = m;
	}
    // 如果 窗口不存在或者窗口是当前窗口
    else if (!c || c == selmon->sel)
		return;
	focus(c);
}

/*******************************************************************************
 * 处理曝光事件(通知窗口管理器和应用程序窗口的内容需要重绘)
*******************************************************************************/
void expose(XEvent *e) {
	Monitor *m;
	XExposeEvent *ev = &e->xexpose;

    // ==0时曝光事件的序列已经完成，曝光事件是按照一系列事件依次发送的，ev->count 指示了还有多少个曝光事件在队列中等待处理。当 ev->count 为零时，表示曝光事件序列已完成。
    // 获取事件所在的监视器
	if (ev->count == 0 && (m = wintomon(ev->window)))
        // 重绘该监视器上的状态栏
		drawbar(m);
}

/*******************************************************************************
 * 设置窗口管理器的焦点
 * TODO
*******************************************************************************/
void
focus(Client *c)
{
	if (!c || !ISVISIBLE(c))
		for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
	if (selmon->sel && selmon->sel != c)
		unfocus(selmon->sel, 0);
	if (c) {
		if (c->mon != selmon)
			selmon = c->mon;
		if (c->isurgent)
			c->seturgent(0);
		c->detachstack();
		c->attachstack();
		grabbuttons(c, 1);
		XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
		c->setfocus();
	} else {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
	selmon->sel = c;
	drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
// 处理焦点
void focusin(XEvent *e) {
	XFocusChangeEvent *ev = &e->xfocus;
    // 如果当前监视器指向的窗口不为空
    // 并且 事件窗口不等于当前监视器所在的当前窗口
	if (selmon->sel && ev->window != selmon->sel->win)
		selmon->sel->setfocus();
}

void
focusmon(const Arg *arg)
{
	Monitor *m;

	if (!mons->next)
		return;
	if ((m = dirtomon(arg->i)) == selmon)
		return;
	unfocus(selmon->sel, 0);
	selmon = m;
	focus(NULL);
}

void
focusstack(const Arg *arg)
{
	Client *c = NULL, *i;

	if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen))
		return;
	if (arg->i > 0) {
		for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
		if (!c)
			for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
	} else {
		for (i = selmon->clients; i != selmon->sel; i = i->next)
			if (ISVISIBLE(i))
				c = i;
		if (!c)
			for (; i; i = i->next)
				if (ISVISIBLE(i))
					c = i;
	}
	if (c) {
		focus(c);
		restack(selmon);
	}
}

Atom
getatomprop(Client *c, Atom prop)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da, atom = None;

	if (XGetWindowProperty(dpy, c->win, prop, 0L, sizeof atom, False, XA_ATOM,
		&da, &di, &dl, &dl, &p) == Success && p) {
		atom = *(Atom *)p;
		XFree(p);
	}
	return atom;
}

int
getrootptr(int *x, int *y)
{
	int di;
	unsigned int dui;
	Window dummy;

	return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long
getstate(Window w)
{
	int format;
	long result = -1;
	unsigned char *p = NULL;
	unsigned long n, extra;
	Atom real;

	if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False, wmatom[WMState],
		&real, &format, &n, &extra, (unsigned char **)&p) != Success)
		return -1;
	if (n != 0)
		result = *p;
	XFree(p);
	return result;
}

int
gettextprop(Window w, Atom atom, char *text, unsigned int size)
{
	char **list = NULL;
	int n;
	XTextProperty name;

	if (!text || size == 0)
		return 0;
	text[0] = '\0';
	if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems)
		return 0;
	if (name.encoding == XA_STRING) {
		strncpy(text, (char *)name.value, size - 1);
	} else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success && n > 0 && *list) {
		strncpy(text, *list, size - 1);
		XFreeStringList(list);
	}
	text[size - 1] = '\0';
	XFree(name.value);
	return 1;
}

void
grabbuttons(Client *c, int focused)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
		if (!focused)
			XGrabButton(dpy, AnyButton, AnyModifier, c->win, False,
				BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
		for (i = 0; i < LENGTH(buttons); i++)
			if (buttons[i].click == ClkClientWin)
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabButton(dpy, buttons[i].button,
						buttons[i].mask | modifiers[j],
						c->win, False, BUTTONMASK,
						GrabModeAsync, GrabModeSync, None, None);
	}
}

/*******************************************************************************
 * 捕获键盘快捷键的输入事件，以便窗口管理器可以响应用户的键盘操作。
 * TODO
*******************************************************************************/
void
grabkeys(void)
{
	updatenumlockmask();
	{
		unsigned int i, j;
		unsigned int modifiers[] = { 0, LockMask, numlockmask, numlockmask|LockMask };
		KeyCode code;

		XUngrabKey(dpy, AnyKey, AnyModifier, root);
		for (i = 0; i < LENGTH(keys); i++)
			if ((code = XKeysymToKeycode(dpy, keys[i].keysym)))
				for (j = 0; j < LENGTH(modifiers); j++)
					XGrabKey(dpy, code, keys[i].mod | modifiers[j], root,
						True, GrabModeAsync, GrabModeAsync);
	}
}

void
incnmaster(const Arg *arg)
{
	selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
	arrange(selmon);
}


/*******************************************************************************
 * 检查Xinerama扩展提供的屏幕信息是否表示唯一的屏幕几何
 * 参数1：存储已知的屏幕几何信息
 * 参数2：已知唯一屏幕信息数组unique的长度
 * 参数3：要检查的新的屏幕几何信息
 * 返回值：如果info和unique中的任何一个屏幕信息相同则返回0,否则返回1
O*******************************************************************************/
#ifdef XINERAMA
static int isuniquegeom(XineramaScreenInfo *unique, size_t n, XineramaScreenInfo *info) {
	while (n--)
		if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org
		&& unique[n].width == info->width && unique[n].height == info->height)
			return 0;
	return 1;
}
#endif /* XINERAMA */

/*******************************************************************************
 * 处理键盘事件
*******************************************************************************/
void keypress(XEvent *e) {
	KeySym keysym;
    XKeyEvent *ev = &e->xkey;

    // 获取按下的按键编码、按键状态
	keysym = XKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0);
    // 遍历config中定义的所有按键组合
	for (int i = 0; i < LENGTH(keys); i++)
        // 按键是否相同
		if (keysym == keys[i].keysym
        // 修饰键组合是否相同
        && CLEANMASK(keys[i].mod) == CLEANMASK(ev->state)
        // 是否有处理该按键的函数，有的话就调用,并传参
        && keys[i].func)
            keys[i].func(&(keys[i].arg));
}



void
killclient(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (!selmon->sel->sendevent(wmatom[WMDelete])) {
		XGrabServer(dpy);
		XSetErrorHandler(xerrordummy);
		XSetCloseDownMode(dpy, DestroyAll);
		XKillClient(dpy, selmon->sel->win);
		XSync(dpy, False);
		XSetErrorHandler(XError);
		XUngrabServer(dpy);
	}
}

/*******************************************************************************
 * 用于管理创建的窗口
*******************************************************************************/
void manage(Window w, XWindowAttributes *wa) {
	Client *c, *t = NULL;
	Window trans = None;
	XWindowChanges wc;

	c = (Client *)ecalloc(1, sizeof(Client));
    // window的句柄（编号）
	c->win = w;
	/* geometry */
	c->x = c->oldx = wa->x;
	c->y = c->oldy = wa->y;
	c->w = c->oldw = wa->width;
	c->h = c->oldh = wa->height;
	c->oldbw = wa->border_width;

    // 更新窗口的标题
    c->updatetitle();
    // 先检查窗口w是否具有传输窗口提示（一个对话框是主窗口的传输窗口），如果有获取句柄到trans
    // 并尝试找到这个窗口
	if (XGetTransientForHint(dpy, w, &trans) && (t = Client::wintoclient(trans))) {
		c->mon = t->mon;
		c->tags = t->tags;
	}
    else {
		c->mon = selmon;
		c->applyrules();
	}
    // 检查客户端窗口的右边界是否超出了当前监视器的右边界
	if (c->x + WIDTH(c) > c->mon->wx + c->mon->ww)
		c->x = c->mon->wx + c->mon->ww - WIDTH(c);
    // 检查客户端窗口的下边界是否超出了当前监视器的下边界
	if (c->y + HEIGHT(c) > c->mon->wy + c->mon->wh)
		c->y = c->mon->wy + c->mon->wh - HEIGHT(c);
    // 确保窗口的 x 和 y 坐标不会小于当前监视器的左上角坐标，以防窗口完全移出了监视器的可见区域。
	c->x = MAX(c->x, c->mon->wx);
	c->y = MAX(c->y, c->mon->wy);
    // 边框
	c->bw = borderpx;
	wc.border_width = c->bw;

    // 设置边框宽度, CWBorderWidth 表示 wc中的border_width 字段将被应用
	XConfigureWindow(dpy, w, CWBorderWidth, &wc);
    // 设置边框颜色,pixel 用于获取颜色方案中边框颜色的像素值,并将这个像素值设置为客户端窗口的边框颜色
	XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
    // 重绘
    c->configure();
//	configure(c); /* propagates border_width, if size doesn't change */
	c->updatewindowtype();
	c->updatesizehints();
	c->updatewmhints();
	XSelectInput(dpy, w, EnterWindowMask|FocusChangeMask|PropertyChangeMask|StructureNotifyMask);
    // 不激活按钮事件,用于支持鼠标操作
	grabbuttons(c, 0);
	if (!c->isfloating)
		c->isfloating = c->oldstate = trans != None || c->isfixed;
	if (c->isfloating)
        // 将窗口置顶
		XRaiseWindow(dpy, c->win);

    // 将窗口添加到监视器的窗口链表中,确保正确的层叠顺序
	c->attach();
	c->attachstack();
    // 将窗口 c->win 添加到根窗口的 _NET_CLIENT_LIST 属性中，这是 EWMH（扩展窗口管理器提示协议）的一部分，用于跟踪所有客户端窗口。
	XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32, PropModeAppend,
		(unsigned char *) &(c->win), 1);
    // 函数将窗口移动到指定的位置和大小，通常将其偏移了 2 * sw（两倍的屏幕宽度）
	XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w, c->h); /* some windows require this */
    // 设置客户端窗口的状态为正常状态NormalState
	c->setclientstate(NormalState);
	if (c->mon == selmon)
        // 取消焦点
		unfocus(selmon->sel, 0);
	c->mon->sel = c;
    // 重新排列
	arrange(c->mon);
    // 映射到屏幕上，使其可见
	XMapWindow(dpy, c->win);
    // 设置焦点
	focus(NULL);
}

/*******************************************************************************
 * 处理映射通知事件
*******************************************************************************/
void mappingnotify(XEvent *e) {
	XMappingEvent *ev = &e->xmapping;
    // 用于刷新键盘映射。它将更新 X 服务器中的键盘映射，
    // 以反映实际键盘布局的变化。这是为了确保键盘事件被正确地映射到键盘键。
	XRefreshKeyboardMapping(ev);
    // 检查映射事件的 request 字段是否等于 MappingKeyboard。这表示该事件是与键盘映射相关的
	if (ev->request == MappingKeyboard)
        // 重新注册键盘快捷键的绑定
		grabkeys();
}

/*******************************************************************************
 * 处理 X Window 系统中的窗口映射请求事件 (XMapRequestEvent)
*******************************************************************************/
void maprequest(XEvent *e) {
    // static , wa用于存储窗口属性信息
	static XWindowAttributes wa;
	XMapRequestEvent *ev = &e->xmaprequest;

    // 声明并初始化了一个指向 XMapRequestEvent 结构体的指针 ev，它用于表示窗口映射请求事件的详细信息。
    //|| 检查属性中的 override_redirect 字段，如果该字段为真（非零），表示该窗口具有覆盖重定向属性，通常这种窗口是不受窗口管理器控制的独立顶级窗口，因此不需要进行管理，直接返回。
	if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect)
		return;
    // 获取到窗口就去管理该窗口
	if (!Client::wintoclient(ev->window))
		manage(ev->window, &wa);
}

void
monocle(Monitor *m)
{
	unsigned int n = 0;
	Client *c;

	for (c = m->clients; c; c = c->next)
		if (ISVISIBLE(c))
			n++;
	if (n > 0) /* override layout symbol */
		snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
	for (c = nexttiled(m->clients); c; c = nexttiled(c->next))
		resize(c, m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
}

/*******************************************************************************
 * 处理鼠标移动事件
*******************************************************************************/
void motionnotify(XEvent *e) {
    // 当前监视器
	Monitor *mon = NULL;
	Monitor *m;
    // 鼠标移动事件
	XMotionEvent *ev = &e->xmotion;

    // 如果点击的不是根窗口，直接返回
	if (ev->window != root)
		return;
    // 确定鼠标所在的监视器
    // 如果鼠标所在监视器和当前监视器不同
	if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
		unfocus(selmon->sel, 1);
		selmon = m;
		focus(NULL);
	}
	mon = m;
}

void
movemouse(const Arg *arg)
{
	int x, y, ocx, ocy, nx, ny;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support moving fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurMove]->cursor, CurrentTime) != GrabSuccess)
		return;
	if (!getrootptr(&x, &y))
		return;
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nx = ocx + (ev.xmotion.x - x);
			ny = ocy + (ev.xmotion.y - y);
			if (abs(selmon->wx - nx) < snap)
				nx = selmon->wx;
			else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) < snap)
				nx = selmon->wx + selmon->ww - WIDTH(c);
			if (abs(selmon->wy - ny) < snap)
				ny = selmon->wy;
			else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) < snap)
				ny = selmon->wy + selmon->wh - HEIGHT(c);
			if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
			&& (abs(nx - c->x) > snap || abs(ny - c->y) > snap))
				togglefloating(NULL);
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, nx, ny, c->w, c->h, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(dpy, CurrentTime);
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

Client *
nexttiled(Client *c)
{
	for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
	return c;
}



void
propertynotify(XEvent *e)
{
	Client *c;
	Window trans;
	XPropertyEvent *ev = &e->xproperty;

	if ((ev->window == root) && (ev->atom == XA_WM_NAME))
		updatestatus();
	else if (ev->state == PropertyDelete)
		return; /* ignore */
	else if ((c = Client::wintoclient(ev->window))) {
		switch(ev->atom) {
		default: break;
		case XA_WM_TRANSIENT_FOR:
			if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
				(c->isfloating = (Client::wintoclient(trans)) != NULL))
				arrange(c->mon);
			break;
		case XA_WM_NORMAL_HINTS:
			c->hintsvalid = 0;
			break;
		case XA_WM_HINTS:
			c->updatewmhints();
			drawbars();
			break;
		}
		if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
            c->updatetitle();
			if (c == c->mon->sel)
				drawbar(c->mon);
		}
		if (ev->atom == netatom[NetWMWindowType])
            c->updatewindowtype();
	}
}

/*******************************************************************************
 * 键盘事件：退出dwm
*******************************************************************************/
void quit(const Arg *arg) {
	running = 0;
}

Monitor *
recttomon(int x, int y, int w, int h)
{
	Monitor *m, *r = selmon;
	int a, area = 0;

	for (m = mons; m; m = m->next)
		if ((a = INTERSECT(x, y, w, h, m)) > area) {
			area = a;
			r = m;
		}
	return r;
}

void
resize(Client *c, int x, int y, int w, int h, int interact)
{
	if (applysizehints(c, &x, &y, &w, &h, interact))
		resizeclient(c, x, y, w, h);
}

void
resizeclient(Client *c, int x, int y, int w, int h)
{
	XWindowChanges wc;

	c->oldx = c->x; c->x = wc.x = x;
	c->oldy = c->y; c->y = wc.y = y;
	c->oldw = c->w; c->w = wc.width = w;
	c->oldh = c->h; c->h = wc.height = h;
	wc.border_width = c->bw;
	XConfigureWindow(dpy, c->win, CWX|CWY|CWWidth|CWHeight|CWBorderWidth, &wc);
    c->configure();
	XSync(dpy, False);
}

void
resizemouse(const Arg *arg)
{
	int ocx, ocy, nw, nh;
	Client *c;
	Monitor *m;
	XEvent ev;
	Time lasttime = 0;

	if (!(c = selmon->sel))
		return;
	if (c->isfullscreen) /* no support resizing fullscreen windows by mouse */
		return;
	restack(selmon);
	ocx = c->x;
	ocy = c->y;
	if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
		None, cursor[CurResize]->cursor, CurrentTime) != GrabSuccess)
		return;
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch(ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handler[ev.type](&ev);
			break;
		case MotionNotify:
			if ((ev.xmotion.time - lasttime) <= (1000 / 60))
				continue;
			lasttime = ev.xmotion.time;

			nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
			nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
			if (c->mon->wx + nw >= selmon->wx && c->mon->wx + nw <= selmon->wx + selmon->ww
			&& c->mon->wy + nh >= selmon->wy && c->mon->wy + nh <= selmon->wy + selmon->wh)
			{
				if (!c->isfloating && selmon->lt[selmon->sellt]->arrange
				&& (abs(nw - c->w) > snap || abs(nh - c->h) > snap))
					togglefloating(NULL);
			}
			if (!selmon->lt[selmon->sellt]->arrange || c->isfloating)
				resize(c, c->x, c->y, nw, nh, 1);
			break;
		}
	} while (ev.type != ButtonRelease);
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1, c->h + c->bw - 1);
	XUngrabPointer(dpy, CurrentTime);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
	if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
		sendmon(c, m);
		selmon = m;
		focus(NULL);
	}
}

void
restack(Monitor *m)
{
	Client *c;
	XEvent ev;
	XWindowChanges wc;

	drawbar(m);
	if (!m->sel)
		return;
	if (m->sel->isfloating || !m->lt[m->sellt]->arrange)
		XRaiseWindow(dpy, m->sel->win);
	if (m->lt[m->sellt]->arrange) {
		wc.stack_mode = Below;
		wc.sibling = m->barwin;
		for (c = m->stack; c; c = c->snext)
			if (!c->isfloating && ISVISIBLE(c)) {
				XConfigureWindow(dpy, c->win, CWSibling|CWStackMode, &wc);
				wc.sibling = c->win;
			}
	}
	XSync(dpy, False);
	while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}






void
sendmon(Client *c, Monitor *m)
{
	if (c->mon == m)
		return;
	unfocus(c, 1);
	c->detach();
	c->detachstack();
	c->mon = m;
	c->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
	c->attach();
	c->attachstack();
	focus(NULL);
	arrange(NULL);
}








void
setlayout(const Arg *arg)
{
	if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt])
		selmon->sellt ^= 1;
	if (arg && arg->v)
		selmon->lt[selmon->sellt] = (Layout *)arg->v;
	strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol, sizeof selmon->ltsymbol);
	if (selmon->sel)
		arrange(selmon);
	else
		drawbar(selmon);
}

/* arg > 1.0 will set mfact absolutely */
void
setmfact(const Arg *arg)
{
	float f;

	if (!arg || !selmon->lt[selmon->sellt]->arrange)
		return;
	f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
	if (f < 0.05 || f > 0.95)
		return;
	selmon->mfact = f;
	arrange(selmon);
}





/*******************************************************************************
 * 如果可见就隐藏，如果不可见就显示
*******************************************************************************/
void showhide(Client *c) {
	if (!c)
		return;
	if (ISVISIBLE(c)) {
		/* show clients top down */
		XMoveWindow(dpy, c->win, c->x, c->y);
		if ((!c->mon->lt[c->mon->sellt]->arrange || c->isfloating) && !c->isfullscreen)
			resize(c, c->x, c->y, c->w, c->h, 0);
		showhide(c->snext);
	} else {
		/* hide clients bottom up */
		showhide(c->snext);
		XMoveWindow(dpy, c->win, WIDTH(c) * -2, c->y);
	}
}

/*******************************************************************************
 * 安装处理终止进程的 信号处理程序
*******************************************************************************/
void sigchld(int unused) {
    // 安装SIGCHLD信号处理程序(子进程终止或者停止时发出的信号)，将sigchld和SIGCHLD关联起来
    // 如果返回 SIG_ERR(安装失败),则结束
	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		Die("can't install SIGCHLD handler:");

    // 等待并处理已经终止的进程
    // 参数1：等待任何子进程的退出
    // 参数2：不关心子进程的退出状态
    // 参数3：非阻塞模式，如果没有子进程退出，waitpid不会阻塞，而是立即返回
	while (0 < waitpid(-1, NULL, WNOHANG))
        ;
}

void
spawn(const Arg *arg)
{
	if (fork() == 0) {
		if (dpy)
			close(ConnectionNumber(dpy));
		setsid();
		execvp(((char **)arg->v)[0], (char **)arg->v);
		die("dwm: execvp '%s' failed:", ((char **)arg->v)[0]);
	}
}

void
tag(const Arg *arg)
{
	if (selmon->sel && arg->ui & TAGMASK) {
		selmon->sel->tags = arg->ui & TAGMASK;
		focus(NULL);
		arrange(selmon);
	}
}

void
tagmon(const Arg *arg)
{
	if (!selmon->sel || !mons->next)
		return;
	sendmon(selmon->sel, dirtomon(arg->i));
}

void
tile(Monitor *m)
{
	unsigned int i, n, h, mw, my, ty;
	Client *c;

	for (n = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), n++);
	if (n == 0)
		return;

	if (n > m->nmaster)
		mw = m->nmaster ? m->ww * m->mfact : 0;
	else
		mw = m->ww;
	for (i = my = ty = 0, c = nexttiled(m->clients); c; c = nexttiled(c->next), i++)
		if (i < m->nmaster) {
			h = (m->wh - my) / (MIN(n, m->nmaster) - i);
			resize(c, m->wx, m->wy + my, mw - (2*c->bw), h - (2*c->bw), 0);
			if (my + HEIGHT(c) < m->wh)
				my += HEIGHT(c);
		} else {
			h = (m->wh - ty) / (n - i);
			resize(c, m->wx + mw, m->wy + ty, m->ww - mw - (2*c->bw), h - (2*c->bw), 0);
			if (ty + HEIGHT(c) < m->wh)
				ty += HEIGHT(c);
		}
}

void
togglebar(const Arg *arg)
{
	selmon->showbar = !selmon->showbar;
	updatebarpos(selmon);
	XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww, bh);
	arrange(selmon);
}

void
togglefloating(const Arg *arg)
{
	if (!selmon->sel)
		return;
	if (selmon->sel->isfullscreen) /* no support for fullscreen windows */
		return;
	selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
	if (selmon->sel->isfloating)
		resize(selmon->sel, selmon->sel->x, selmon->sel->y,
			selmon->sel->w, selmon->sel->h, 0);
	arrange(selmon);
}

void
toggletag(const Arg *arg)
{
	unsigned int newtags;

	if (!selmon->sel)
		return;
	newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
	if (newtags) {
		selmon->sel->tags = newtags;
		focus(NULL);
		arrange(selmon);
	}
}

void
toggleview(const Arg *arg)
{
	unsigned int newtagset = selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

	if (newtagset) {
		selmon->tagset[selmon->seltags] = newtagset;
		focus(NULL);
		arrange(selmon);
	}
}

void
unfocus(Client *c, int setfocus)
{
	if (!c)
		return;
	grabbuttons(c, 0);
	XSetWindowBorder(dpy, c->win, scheme[SchemeNorm][ColBorder].pixel);
	if (setfocus) {
		XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
		XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
	}
}



/*******************************************************************************
 * unmapnotify 函数用于处理窗口取消映射通知事件。
 * 根据事件的来源和原因，将客户端的状态设置为 WithdrawnState 或者从窗口管理器中移除客户端。这有助于确保窗口的正确处理和管理。
*******************************************************************************/
void unmapnotify(XEvent *e) {
	Client *c;
	XUnmapEvent *ev = &e->xunmap;

	if ((c = Client::wintoclient(ev->window))) {
        // 这一行代码检查窗口取消映射事件是否是由客户端发送的。如果是客户端发送的事件，表示客户端主动取消了映射，因此将客户端的状态设置为 WithdrawnState，表示窗口已撤回。
		if (ev->send_event)
			c->setclientstate(WithdrawnState);
        // 如果不是客户端发送的事件，表示窗口由其他原因取消了映射，那么就调用 unmanage 函数来从窗口管理器中移除指定的客户端 c，但不销毁客户端。这意味着窗口不再管理，但仍然可以恢复到正常状态，以备后续重新映射
		else
			c->unmanage(0);
	}
}

/*******************************************************************************
 * 更新状态栏,为每个监视器创建自己独立的状态栏窗口
*******************************************************************************/
void updatebars(void) {
    // 用于遍历所有监视器
	Monitor *m;
	XSetWindowAttributes wa = {
        // 与父窗口共享背景
        .background_pixmap = ParentRelative,
//        unsigned long background_pixel;	/* background pixel */
//        Pixmap border_pixmap;	/* border of the window */
//        unsigned long border_pixel;	/* border pixel value */
//        int bit_gravity;		/* one of bit gravity values */
//        int win_gravity;		/* one of the window gravity values */
//        int backing_store;		/* NotUseful, WhenMapped, Always */
//        unsigned long backing_planes;/* planes to be preserved if possible */
//        unsigned long backing_pixel;/* value to use in restoring planes */
//        Bool save_under;		/* should bits under be saved? (popups) */
//        long event_mask;		/* set of events that should be saved */
        // 窗口关注的事件为 按钮按下事件和曝光事件
        .event_mask = ButtonPressMask|ExposureMask,
//        long do_not_propagate_mask;	/* set of events that should not propagate */
//        Bool override_redirect;	/* boolean value for override-redirect */
        // 状态栏窗口是否应该绕过窗口管理器的默认行为，True表示窗口管理器会将该窗口的控制权交给该窗口背身，不去干预或者管理
        .override_redirect = True
//        Colormap colormap;		/* color map to be associated with window */
//        Cursor cursor;		/* cursor to be displayed (or None) */
	};

    // 设置窗口的类名和实例名
	XClassHint ch = {"dwm", "dwm"};
    // 遍历所有的监视器
	for (m = mons; m; m = m->next) {
        // 如果监视器的状态栏窗口已经存在，则跳过，不重复创建
		if (m->barwin)
			continue;
        // 创建状态栏，参数包括窗口的位置、大小、深度等信息
		m->barwin = XCreateWindow(dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
				CopyFromParent, DefaultVisual(dpy, screen),
				CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
        // 为窗口设置光标
		XDefineCursor(dpy, m->barwin, cursor[CurNormal]->cursor);
        // 将窗口映射到显示中以显示状态栏
		XMapRaised(dpy, m->barwin);
        // 使其他程序可以识别设置的窗口的类名和实例名，此处为dwm
		XSetClassHint(dpy, m->barwin, &ch);
	}
}

void
updatebarpos(Monitor *m)
{
	m->wy = m->my;
	m->wh = m->mh;
	if (m->showbar) {
		m->wh -= bh;
		m->by = m->topbar ? m->wy : m->wy + m->wh;
		m->wy = m->topbar ? m->wy + bh : m->wy;
	} else
		m->by = -bh;
}

void
updateclientlist()
{
	Client *c;
	Monitor *m;

	XDeleteProperty(dpy, root, netatom[NetClientList]);
	for (m = mons; m; m = m->next)
		for (c = m->clients; c; c = c->next)
			XChangeProperty(dpy, root, netatom[NetClientList],
				XA_WINDOW, 32, PropModeAppend,
				(unsigned char *) &(c->win), 1);
}

/*******************************************************************************
 * 更新几何 TODO
*******************************************************************************/
int updategeom(void) {
    // 是否需要刷新布局信息,0表示没有需要更新的布局信息
	int dirty = 0;

// 如果使用Xinerama扩展
#ifdef XINERAMA
    // 如果Xinerama扩展可用
	if (XineramaIsActive(dpy)) {
		int i, j, n, nn;
		Client *c;
		Monitor *m;

        // 获取当前Xinerama屏幕信息存储在info中,共有nn个
		XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
		XineramaScreenInfo *unique = NULL;

		for (n = 0, m = mons; m; m = m->next, n++)
            ;

		/* only consider unique geometries as separate screens */
        // 检查是否有相同的屏幕几何信息，将唯一的屏幕信息存储在unique中
		unique = (XineramaScreenInfo *)ecalloc(nn, sizeof(XineramaScreenInfo));
		for (i = 0, j = 0; i < nn; i++)
            // 如果有相同的信息
			if (isuniquegeom(unique, j, &info[i]))
				memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
		XFree(info);

        // 新的监视器数量
		nn = j;
		/* new monitors if nn > n */
        // 如果新的监视器数量大于当前监视器数量
		for (i = n; i < nn; i++) {
			for (m = mons; m && m->next; m = m->next)
                ;
			if (m)
				m->next = createmon();
			else
				mons = createmon();
		}
		for (i = 0, m = mons; i < nn && m; m = m->next, i++)
			if (i >= n
			|| unique[i].x_org != m->mx || unique[i].y_org != m->my
			|| unique[i].width != m->mw || unique[i].height != m->mh)
			{
				dirty = 1;
				m->num = i;
				m->mx = m->wx = unique[i].x_org;
				m->my = m->wy = unique[i].y_org;
				m->mw = m->ww = unique[i].width;
				m->mh = m->wh = unique[i].height;
				updatebarpos(m);
			}
		/* removed monitors if n > nn */
		for (i = nn; i < n; i++) {
			for (m = mons; m && m->next; m = m->next);
			while ((c = m->clients)) {
				dirty = 1;
				m->clients = c->next;
				c->detachstack();
				c->mon = mons;
				c->attach();
				c->attachstack();
			}
			if (m == selmon)
				selmon = mons;
			cleanupmon(m);
		}
		free(unique);
	} else
#endif /* XINERAMA */
	{ /* default monitor setup */
        // 默认的显示器设置
		if (!mons)
			mons = createmon();
		if (mons->mw != sw || mons->mh != sh) {
			dirty = 1;
			mons->mw = mons->ww = sw;
			mons->mh = mons->wh = sh;
			updatebarpos(mons);
		}
	}
	if (dirty) {
		selmon = mons;
		selmon = wintomon(root);
	}
	return dirty;
}

void
updatenumlockmask(void)
{
	unsigned int i, j;
	XModifierKeymap *modmap;

	numlockmask = 0;
	modmap = XGetModifierMapping(dpy);
	for (i = 0; i < 8; i++)
		for (j = 0; j < modmap->max_keypermod; j++)
			if (modmap->modifiermap[i * modmap->max_keypermod + j]
				== XKeysymToKeycode(dpy, XK_Num_Lock))
				numlockmask = (1 << i);
	XFreeModifiermap(modmap);
}



/*******************************************************************************
 * 更新状态栏上的文本内容
*******************************************************************************/
void updatestatus(void) {
    // 获取root窗口的属性值，将结果存在stext数组中，属性XA_WM_NAME为窗口的名称或者标题
	if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext)))
        // 获取成功时使用获取的文本，获取失败默认使用dwm-VERSION
		strcpy(stext, "dwm-" VERSION);
    // 传递当前选中的监视器，以便重新绘制状态栏，以显示更新后的文本
	drawbar(selmon);
}




void
view(const Arg *arg)
{
	if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags])
		return;
	selmon->seltags ^= 1; /* toggle sel tagset */
	if (arg->ui & TAGMASK)
		selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
	focus(NULL);
	arrange(selmon);
}


Monitor *
wintomon(Window w)
{
	int x, y;
	Client *c;
	Monitor *m;

	if (w == root && getrootptr(&x, &y))
		return recttomon(x, y, 1, 1);
	for (m = mons; m; m = m->next)
		if (w == m->barwin)
			return m;
	if ((c = Client::wintoclient(w)))
		return c->mon;
	return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */


int
xerrordummy(Display *dpy, XErrorEvent *ee)
{
	return 0;
}


void
zoom(const Arg *arg)
{
	Client *c = selmon->sel;

	if (!selmon->lt[selmon->sellt]->arrange || !c || c->isfloating)
		return;
	if (c == nexttiled(selmon->clients) && !(c = nexttiled(c->next)))
		return;
	c->pop();
}

/*******************************************************************************
  * 检查是否有其他窗口管理器运行
 *******************************************************************************/
void Dwm::CheckOtherWm(void) {
    xerrorxlib = XSetErrorHandler([](Display *dpy, XErrorEvent *ee) ->int {
        Die("dwm: another window manager is already running");
        return -1;
    });
    /* this causes an error if some other window manager is running */
    XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
    XSync(dpy, False);
    XSetErrorHandler(XError);
    XSync(dpy, False);
}

/*******************************************************************************
 * 设置DWM的许多配置，例如字体，颜色，状态栏，窗口的事件监听等
*******************************************************************************/
void Dwm::SetUp(void) {
                  // 设置窗口属性
                  XSetWindowAttributes wa;

                  // 清理所有僵尸进程
                  sigchld(0);

                  /* init screen */
                  // 获取当前屏幕编号
                  screen = DefaultScreen(dpy);
                  // 获取当前屏幕宽度
                  sw = DisplayWidth(dpy, screen);
                  // 获取当前屏幕高度
                  sh = DisplayHeight(dpy, screen);
                  // 获取指定屏幕的根窗口
                  root = RootWindow(dpy, screen);

                  // 创建绘图上下文
                  drw = drw_create(dpy, screen, root, sw, sh);
                  // 创建字体合集
                  if (!drw_fontset_create(drw, fonts, LENGTH(fonts)))
                  Die("no fonts could be loaded.");

                  // 字体的高
                  lrpad = drw->fonts->h;
                  // 边的高度(bar height),能够容纳字体
                  bh = drw->fonts->h + 2;

                  // 更新几何
                  updategeom();

                  /* init atoms */
                  // 获取一个名为UTF8_STRING的Atom存储在utf8string中,False 表示不创建新的Atom,只在x服务器中查找现有的Atom
                  Atom utf8string = XInternAtom(dpy, "UTF8_STRING", False);
                  // 窗口管理器协议
                  wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
                  // 窗口关闭请求协议
                  wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
                  // 窗口状态协议
                  wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
                  // 获取焦点协议
                  wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
                  // 窗口管理器支持的扩展协议
                  netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
                  // 窗口的名称
                  netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
                  // 窗口状态扩展协议，用于管理窗口的状态
                  netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
                  // 用于检查窗口管理器是否运行
                  netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
                  // 全屏窗口状态
                  netatom[NetWMFullscreen] = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
                  // 当前活动的窗口
                  netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
                  // 窗口类型
                  netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
                  // 对话框窗口类型
                  netatom[NetWMWindowTypeDialog] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
                  // 获取当前运行的客户端应用程序列表
                  netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);

                  // 初始化鼠标
                  // 普通鼠标
                  cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
                  // 调整窗口大小的鼠标
                  cursor[CurResize] = drw_cur_create(drw, XC_sizing);
                  // 移动窗口的鼠标
                  cursor[CurMove] = drw_cur_create(drw, XC_fleur);

                  /* init appearance */
                  // 初始化外观,(创建颜色集合)
                  scheme = (Clr **)ecalloc(LENGTH(colors), sizeof(Clr *));
                  for (int i = 0; i < LENGTH(colors); i++)
                  scheme[i] = drw_scm_create(drw, colors[i], 3);

                  /* init bars */
                  // 更新状态栏
                  updatebars();

                  // 更新状态栏的文本信息
                  updatestatus();
                  /* supporting window for NetWMCheck */
                  // NetWMCheck的支持窗口
                  // 创建一个大小为 1x1 像素，位置在坐标 (0, 0)，不可见的窗口
                  wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
                  // 窗口的属性设置为NET_WM_CHECK类型，值设置为wmcheckwin，这是为了支持EWMH
                  XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *) &wmcheckwin, 1);
                  // 窗口的属性设置为 _NET_WM_NAME 类型，并将其值设置为 "dwm"，表示窗口管理器的名称。
                  XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
                  PropModeReplace, (unsigned char *) "dwm", 3);
                  //将窗口 wmcheckwin 的信息存储为一个属性，属性的类型是 _NET_WM_CHECK，属性的数据类型是窗口类型（XA_WINDOW），并且将其完全替换根窗口的相应属性。通常，这样的属性用于与窗口管理器通信，以支持特定的窗口管理功能或协议，例如 EWMH。
                  XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
                  PropModeReplace, (unsigned char *) &wmcheckwin, 1);
                  /* EWMH support per view */
                  //     将根窗口的属性设置为 _NET_SUPPORTED 类型，其值是一个包含各种 Atom 类型的数组，以表明窗口管理器支持的 EWMH 特性。
                  XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
                  PropModeReplace, (unsigned char *) netatom, NetLast);
                  // 删除根窗口的 _NET_CLIENT_LIST 属性，以便窗口管理器自行维护这个属性。
                  XDeleteProperty(dpy, root, netatom[NetClientList]);
                  /* select events */
                  //为根窗口设置事件监听和光标属性：
                  //
                  //    设置光标为正常状态下的光标形状。
                  //    设置事件掩码，以便根窗口能够监听并处理多种事件，包括窗口结构变化、鼠标按钮按下、鼠标指针移动、鼠标进入和离开窗口、窗口属性变化等。
                  //    使用 XChangeWindowAttributes 函数和 XSelectInput 函数分别为根窗口设置事件属性和事件监听。
                  wa.cursor = cursor[CurNormal]->cursor;
                  /*SubstructureRedirectMask：表示子窗口重定向事件，通常用于捕获窗口创建和销毁事件。
                  SubstructureNotifyMask：表示子窗口通知事件，通常用于捕获子窗口的改变事件，如改变大小或移动。
                  ButtonPressMask：表示鼠标按钮按下事件，用于捕获鼠标按键的点击事件。
                  PointerMotionMask：表示鼠标指针移动事件，用于捕获鼠标指针的移动。
                  EnterWindowMask：表示鼠标指针进入窗口事件。
                  LeaveWindowMask：表示鼠标指针离开窗口事件。
                  StructureNotifyMask：表示窗口结构变化事件，用于捕获窗口的改变，如创建、销毁、映射和取消映射。
                  PropertyChangeMask：表示属性变化事件，用于捕获窗口属性的改变。*/
                  wa.event_mask = SubstructureRedirectMask|SubstructureNotifyMask
                  |ButtonPressMask|PointerMotionMask|EnterWindowMask
                  |LeaveWindowMask|StructureNotifyMask|PropertyChangeMask;
                  // 更改根窗口的属性,参数3表示要更改的属性标志，包括事件掩码和光标
                  XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);
                  // 为根窗口（root）设置事件监听
                  XSelectInput(dpy, root, wa.event_mask);
                  //捕获键盘快捷键的输入事件，以便窗口管理器可以响应用户的键盘操作。
                  grabkeys();
                  //置窗口管理器的焦点
                  focus(NULL);
                  }

/*******************************************************************************
 * 退出dwm时释放所有资源
*******************************************************************************/
void Dwm::CleanUp(void) {
    Arg a = {.ui = (unsigned int)~0};
    Layout foo = { "", NULL };
    Monitor *m;
    size_t i;

    // 将当前桌面视图切换到一个默认的视图，以确保所有客户端都被移动到一个可见的桌面
    view(&a);
    // 将当前的layout置空
    selmon->lt[selmon->sellt] = &foo;
    // 遍历所有监视器，如果还是窗口，就取消管理这些窗口
    for (m = mons; m; m = m->next)
    while (m->stack)
    m->stack->unmanage(0);
    // 取消注册dwm监听的所有键盘快捷键
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    // 循环释放所有监视器
    while (mons)
    cleanupmon(mons);
    // 释放所有光标
    for (i = 0; i < CurLast; i++)
    drw_cur_free(drw, cursor[i]);
    // 释放所有颜色方案
    for (i = 0; i < LENGTH(colors); i++)
    free(scheme[i]);
    // 释放颜色方案数组
    free(scheme);
    // 销毁dwm使用的用于支持_NET_SUPPORTING_CHECK
    XDestroyWindow(dpy, wmcheckwin);
    // 释放drw
    drw_free(drw);
    // 确保所有未完成的X协议请求都被处理
    XSync(dpy, False);
    // 将输入焦点设置为根窗口
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    // 删除 _NET_ACTIVE_WINDOW属性
    XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

/*******************************************************************************
 * 扫描所有的窗口，并根据是否是辅助窗口来跳过或者调用manage函数
*******************************************************************************/
void Dwm::Scan(void) {
     unsigned int i, num;
     Window d1, d2, *wins = NULL;
     XWindowAttributes wa;

     // 查询根窗口下的所有窗口，并将结果存储在wins数组中,同时获取窗口的数量num
     if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
     for (i = 0; i < num; i++) {
     // 获取wins[i]窗口的属性信息,存储在wa中
     if (!XGetWindowAttributes(dpy, wins[i], &wa)
     // 如果窗口的override_redirect属性为真，或者窗口具有XGetTransientForHint提示（通常用于指示窗口是某个主窗口的辅助窗口），则跳过该窗口
     || wa.override_redirect || XGetTransientForHint(dpy, wins[i], &d1))
     continue;
     // 如果窗口的状态是可视状态 或者 窗口的状态为最小化状态，则调用manage函数对窗口进行管理
     if (wa.map_state == IsViewable || getstate(wins[i]) == IconicState)
     // 管理窗口,进行重排等操作
     manage(wins[i], &wa);
     }
     // 检查窗口的属性和状态来判断是否要管理该窗口
     for (i = 0; i < num; i++) { /* now the transients */
     // 将窗口的信息存储在wa中
     if (!XGetWindowAttributes(dpy, wins[i], &wa))
     continue;
     //
     if (XGetTransientForHint(dpy, wins[i], &d1)
     && (wa.map_state == IsViewable || getstate(wins[i]) == IconicState))
     manage(wins[i], &wa);
     }
     if (wins)
     XFree(wins);
     }
     }

Dwm::Dwm() {
    CheckOtherWm();
    SetUp();
    Scan();
}


Dwm::~Dwm() {
    CleanUp();
}

/*******************************************************************************
* dwm窗口的主事件循环
*******************************************************************************/
void Dwm::Run(void) {
                XEvent ev;
                /* main event loop */
                // 确保X服务器中的事件和窗口状态已经与客户端（dwm）同步
                // 可以确保之后的事件处理不会与之前的事件状态混淆
                XSync(dpy, False);
                // 正在运行 并且 阻塞地获取下一个事件
                while (running && !XNextEvent(dpy, &ev))
                // 如果事件的类型处理方法(函数指针)不为null
                if (handler[ev.type])
                // 处理这个方法
                handler[ev.type](&ev); /* call handler */
                }



int main(int argc, char *argv[]) {

    if (argc == 2) {
        std::string argv1(argv[1]);
        if (argv1 == "-v") {
            Die("dwm-" VERSION);
        }
        else {
            Die("usage: dwm [-v]");
        }
    }
    // 有多个参数时，提示只支持参数-v
	else if (argc != 1)
		Die("usage: dwm [-v]");
	if (!setlocale(LC_CTYPE, "") || !XSupportsLocale())
        std::cerr << "warning: no locale support" << std::endl;
	if (!(dpy = XOpenDisplay(NULL)))
		Die("dwm: cannot open display");
    Dwm dwm;
	dwm.Run();
	XCloseDisplay(dpy);
	return EXIT_SUCCESS;
}
