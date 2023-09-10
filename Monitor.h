/*******************************************************************************
 * Author : yongheng
 * Data   : 2023/09/10 23:22
*******************************************************************************/


#pragma once
#include "Client.h"
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
struct Client;
struct Monitor;

struct Layout{
    const char *symbol;
    void (*arrange)(Monitor *);
};

struct Monitor {
    char ltsymbol[16];//一个字符串数组，用于存储监视器的布局符号（layout symbol）。这通常是一个用于表示当前布局模式的短字符串，例如 "T" 表示平铺布局，"M" 表示主区域布局，等等。
    float mfact;//一个浮点数，表示主区域的宽度与整个屏幕宽度之比。通常用于控制平铺布局中主区域的大小。
    int nmaster;//一个整数，表示主区域中主客户端的数量，通常用于平铺布局。
    int num;//一个整数，表示监视器的编号或索引。
    int by; //一个整数，表示状态栏（bar）的垂直位置，通常是状态栏距离屏幕顶部或底部的距离。              /* bar geometry */
    int mx, my, mw, mh; // 左上角坐标,监视器的宽高
    int wx, wy, ww, wh;  // 窗口的左上坐标和宽高
    unsigned int seltags;//一个无符号整数，表示当前选定的标签集。
    unsigned int sellt; //一个无符号整数，表示当前选定的布局模式。
    unsigned int tagset[2];//一个整数数组，表示两个标签集，通常用于在不同的工作区之间切换。
    int showbar;//一个整数，表示是否显示状态栏。通常用于控制状态栏的可见性。
    int topbar; //一个整数，表示状态栏是否位于屏幕的顶部（1）或底部（0）。
    Client *clients;    // 一个指向客户端窗口的链表，表示当前监视器上的所有客户端窗口。
    Client *sel;    // 一个指向当前选定的客户端窗口的指针。
    Client *stack; //一个指向客户端窗口堆栈的链表，通常用于管理窗口的层叠顺序。
    Monitor *next; // 一个指向下一个监视器的指针，通常用于连接多个监视器。
    Window barwin; // 任务栏的窗口句柄。
    const Layout *lt[2]; // 一个指向布局（Layout）结构体的数组，通常包含两种不同的布局模式，例如平铺布局和主区域布局。
};;
