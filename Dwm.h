/*******************************************************************************
 * Author : yongheng
 * Data   : 2023/09/10 23:03
*******************************************************************************/

#pragma once
#include <sys/wait.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xproto.h>
#include <X11/Xutil.h>
#ifdef XINERAMA
#include <X11/extensions/Xinerama.h>
#endif /* XINERAMA */
#include <X11/Xft/Xft.h>


/* macros */
#define BUTTONMASK              (ButtonPressMask|ButtonReleaseMask)
#define CLEANMASK(mask)         (mask & ~(numlockmask|LockMask) & (ShiftMask|ControlMask|Mod1Mask|Mod2Mask|Mod3Mask|Mod4Mask|Mod5Mask))
#define INTERSECT(x,y,w,h,m)    (MAX(0, MIN((x)+(w),(m)->wx+(m)->ww) - MAX((x),(m)->wx)) \
                               * MAX(0, MIN((y)+(h),(m)->wy+(m)->wh) - MAX((y),(m)->wy)))
#define ISVISIBLE(C)            ((C->tags & C->mon->tagset[C->mon->seltags]))
#define LENGTH(X)               (sizeof X / sizeof X[0])
#define MOUSEMASK               (BUTTONMASK|PointerMotionMask)
#define WIDTH(X)                ((X)->w + 2 * (X)->bw)
#define HEIGHT(X)               ((X)->h + 2 * (X)->bw)
#define TEXTW(X)                (drw_fontset_getwidth(drw, (X)) + lrpad)

/* enums */
enum { CurNormal, CurResize, CurMove, CurLast }; /* cursor */
enum { SchemeNorm, SchemeSel }; /* color schemes */
enum { NetSupported, NetWMName, NetWMState, NetWMCheck,
    NetWMFullscreen, NetActiveWindow, NetWMWindowType,
    NetWMWindowTypeDialog, NetClientList, NetLast }; /* EWMH atoms */
enum { WMProtocols, WMDelete, WMState, WMTakeFocus, WMLast }; /* default atoms */
enum { ClkTagBar, ClkLtSymbol, ClkStatusText, ClkWinTitle,
    ClkClientWin, ClkRootWin, ClkLast }; /* clicks */

// 参数
union Arg {
    int i;
    unsigned int ui;
    float f;
    const void *v;
};

// 鼠标事件
struct Button {
    unsigned int click;     // 点击区域
    unsigned int mask;      // 键盘掩码
    unsigned int button;    // 按键编号
    void (*func)(const Arg *arg); // 处理鼠标事件的函数
    const Arg arg;// func的参数
};

struct Monitor;
struct Client;


struct Key{
    unsigned int mod;   // 修饰键，ctrl | shift | alt 等
    KeySym keysym;// 按键
    void (*func)(const Arg *);// 处理这组按键的函数
    const Arg arg;// 参数
};

struct Layout;





struct Rule{
    const char *_class; //这是用于区分窗口类型的字符串
    const char *instance;//通常与类别一起用于更精确地区分窗口。
    const char *title;//用于匹配窗口的标题。
    unsigned int tags;//指定要分配给窗口的标签，使用二进制掩码的形式表示。
    int isfloating;//表示该窗口应该被视为浮动窗口。
    int monitor;//监视器索引。指定要将窗口分配到哪个监视器上，通过监视器的索引进行指定。
};

class Dwm {
    /*******************************************************************************
      * 检查是否有其他窗口管理器运行
     *******************************************************************************/
    void CheckOtherWm();

    /*******************************************************************************
     * 设置DWM的许多配置，例如字体，颜色，状态栏，窗口的事件监听等
    *******************************************************************************/
    void SetUp();

    /*******************************************************************************
     * 退出dwm时释放所有资源
    *******************************************************************************/
    void CleanUp();

    /*******************************************************************************
     * 扫描所有的窗口，并根据是否是辅助窗口来跳过或者调用manage函数
    *******************************************************************************/
    void Scan();

public:
    Dwm();
    ~Dwm();

    /*******************************************************************************
     * dwm窗口的主事件循环
    *******************************************************************************/
    void Run(void);
};




/* function declarations */
static void buttonpress(XEvent *e);
static void clientmessage(XEvent *e);
static void configurenotify(XEvent *e);
static void configurerequest(XEvent *e);
static void destroynotify(XEvent *e);
static void drawbars(void);
static void enternotify(XEvent *e);
static void expose(XEvent *e);
static void focusin(XEvent *e);
static void focusmon(const Arg *arg);
static void focusstack(const Arg *arg);
static int getrootptr(int *x, int *y);
static long getstate(Window w);
static int gettextprop(Window w, Atom atom, char *text, unsigned int size);
static void grabkeys(void);
static void incnmaster(const Arg *arg);
static void keypress(XEvent *e);
static void killclient(const Arg *arg);
static void manage(Window w, XWindowAttributes *wa);
static void mappingnotify(XEvent *e);
static void maprequest(XEvent *e);
static void motionnotify(XEvent *e);
static void movemouse(const Arg *arg);
static void propertynotify(XEvent *e);
static void quit(const Arg *arg);
static void resize(Client *c, int x, int y, int w, int h, int interact);
static void resizeclient(Client *c, int x, int y, int w, int h);
static void resizemouse(const Arg *arg);
static void setlayout(const Arg *arg);
static void setmfact(const Arg *arg);
static void sigchld(int unused);
static void spawn(const Arg *arg);
static void tag(const Arg *arg);
static void tagmon(const Arg *arg);
static void togglebar(const Arg *arg);
static void togglefloating(const Arg *arg);
static void toggletag(const Arg *arg);
static void toggleview(const Arg *arg);
static void unmapnotify(XEvent *e);
static void updatebars(void);
static void updateclientlist(void);
static int updategeom(void);
static void updatenumlockmask(void);
static void updatestatus(void);
static void view(const Arg *arg);
static int xerrordummy(Display *dpy, XErrorEvent *ee);
static void zoom(const Arg *arg);


static void arrange(Monitor *m);
static void arrangemon(Monitor *m);
static void cleanupmon(Monitor *mon);
static Monitor *createmon(void);
static Monitor *dirtomon(int dir);
static void drawbar(Monitor *m);
static void monocle(Monitor *m);
static Monitor *recttomon(int x, int y, int w, int h);
static void restack(Monitor *m);
static void tile(Monitor *m);
static void updatebarpos(Monitor *m);




static int applysizehints(Client *c, int *x, int *y, int *w, int *h, int interact);
static void sendmon(Client *c, Monitor *m);

// TODO
static void showhide(Client *c);
static void focus(Client *c);

static Client *nexttiled(Client *c);
static void unfocus(Client *c, int setfocus);
