/*******************************************************************************
 * Author : yongheng
 * Data   : 2023/09/10 23:19
*******************************************************************************/

#pragma once
#include "Monitor.h"
struct Monitor;
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
// 保存窗口的各种属性和状态
struct Client {
    char name[256]; //窗口的名称或标题，最多可以包含 256 个字符。
    float mina, maxa;//窗口的最小和最大宽高比。
    int x, y, w, h;//窗口的位置和大小。x 和 y 表示窗口的左上角坐标，w 和 h 表示窗口的宽度和高度。
    int oldx, oldy, oldw, oldh;//窗口的旧位置和大小，用于跟踪窗口的变化。
    int basew, baseh; //basew, baseh：窗口的基本宽度和高度。
    int incw, inch; // incw, inch：窗口的宽度和高度的增量值。
    int maxw, maxh, minw, minh; //maxw, maxh：窗口的最大宽度和高度。minw, minh：窗口的最小宽度和高度。
    int  hintsvalid;//hintsvalid：标志表示窗口的大小提示信息是否有效。
    int bw, oldbw;  //窗口的边框宽度，以及旧的边框宽度。
    unsigned int tags;//窗口的标签，用于将窗口分组到不同的标签组中。
    int isfixed, isfloating, isurgent;//是否被固定（不可移动和调整大小），是否浮动（不受平铺布局限制）是否处于紧急状态
    int neverfocus; //是否不应该获得焦点。
    int oldstate; // 窗口的旧状态，通常用于在窗口恢复到正常状态时还原。
    int isfullscreen;//是否处于全屏状态
    Client *next;//指向下一个客户端窗口的指针，用于构建窗口链表。
    Client *snext;//指向下一个浮动窗口的指针，用于构建浮动窗口链表。
    Monitor *mon;//指向监视器的指针，表示窗口所在的监视器。
    Window win;//窗口的 X11 窗口句柄。

public:
    /*******************************************************************************
     * 通过window编号在所有窗口中找到指向这个窗口的指针
    *******************************************************************************/
    static Client* wintoclient(Window w) ;
    
    /*******************************************************************************
     * 设置特定窗口的几何属性,重新绘制特定窗口
    *******************************************************************************/
    void configure();
    void updatetitle();
    void updatewindowtype();
    void updatewmhints();
    void setfocus();
    void attachstack();
    void applyrules();
    void attach();
    void detach();
    void detachstack();
    void pop();
    void updatesizehints();


    void focus();
    void showhide();

    Client *nexttiled();
    void unfocus( int setfocus);
    void unmanage(int destroyed);
    int sendevent(Atom proto);
    void sendmon(Monitor *m);
    void setclientstate(long state);
    void setfullscreen(int fullscreen);
    void seturgent(int urg);
    void grabbuttons(int focused);
    int applysizehints(int *x, int *y, int *w, int *h, int interact);
    Atom getatomprop(Atom prop);
};
