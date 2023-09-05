# dwm - dynamic window manager
# See LICENSE file for copyright and license details.

# dwm version
VERSION = 6.4

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

X11INC = /usr/X11R6/include
X11LIB = /usr/X11R6/lib

# Xinerama, comment if you don't want it
XINERAMALIBS  = -lXinerama
XINERAMAFLAGS = -DXINERAMA

# freetype
FREETYPELIBS = -lfontconfig -lXft
FREETYPEINC = /usr/include/freetype2
# OpenBSD (uncomment)
#FREETYPEINC = ${X11INC}/freetype2
#MANPREFIX = ${PREFIX}/man

# includes and libs
INCS = -I${X11INC} -I${FREETYPEINC}
LIBS = -L${X11LIB} -lX11 ${XINERAMALIBS} ${FREETYPELIBS}

# flags
CPPFLAGS = -std=c++20 -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_POSIX_C_SOURCE=200809L -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS}
# CPPFLAGS = -std=c++20 -DVERSION=\"${VERSION}\" ${XINERAMAFLAGS}
#CFLAGS   = -g -std=c99 -pedantic -Wall -O0 ${INCS} ${CPPFLAGS}
# CFLAGS   = -std=c99 -pedantic -Wall -Wno-deprecated-declarations -Os ${INCS} ${CPPFLAGS}
CFLAGS   = -w -pedantic -Wno-deprecated-declarations -Os ${INCS} ${CPPFLAGS}
LDFLAGS  = ${LIBS}

# Solaris
#CFLAGS = -fast ${INCS} -DVERSION=\"${VERSION}\"
#LDFLAGS = ${LIBS}

# compiler and linker
CC = g++


# 源文件有drw.c dwm.c util.c
SRC = drw.cpp dwm.cpp util.cpp

# 将SCR中所有的.c后缀改为.o
OBJ = ${SRC:.cpp=.o}

all: options dwm

# 选项
options:
	@echo dwm build options:
	@echo "CFLAGS   = ${CFLAGS}"
	@echo "LDFLAGS  = ${LDFLAGS}"
	@echo "CC       = ${CC}"

    # 编译.c文件为.o
.cpp.o:
    # -c 只编译不链接
    # &< 表示第一个依赖，即源文件的名称
	${CC} -c ${CFLAGS} $<

# config.h 是 OBJ的依赖
${OBJ}: config.h

# $@是生成目标也就是 config.h
config.h:
	cp config.def.h $@

# 生成dwm
dwm:${OBJ}
	${CC} -o $@ ${OBJ} ${LDFLAGS}

# 清除动作# 删除当前路径下的dwm和 源文件压缩包(这个压缩包的删除不是必要的)
clean:
	rm -f dwm ${OBJ} dwm-${VERSION}.tar.gz

# 打包,将于娜吗和可执行文件打包
dist: clean
	mkdir -p dwm-${VERSION}
	cp -R LICENSE Makefile README config.def.h config.mk\
            dwm.1 drw.h util.h ${SRC} dwm.png transient.c dwm-${VERSION}
	tar -cf dwm-${VERSION}.tar dwm-${VERSION}
	gzip dwm-${VERSION}.tar
	rm -rf dwm-${VERSION}

# 安装# DESTDIR 是默认安装目录，可以通过make install DESTDIR=<$CUSTOM_PREFIX>修改# 默认安装在了 "" /usr/local / bin下# 添加man
install: all
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f dwm ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/dwm
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < dwm.1 > ${DESTDIR}${MANPREFIX}/man1/dwm.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/dwm.1

# 卸载
uninstall:
	rm -f ${DESTDIR}${PREFIX}/bin/dwm\
		${DESTDIR}${MANPREFIX}/man1/dwm.1

# .PHONY防止当前作用域下的文件和命令冲突# 没有.PHONY时使用文件，有.PHONY时使用命令
.PHONY: all options clean dist install uninstall
