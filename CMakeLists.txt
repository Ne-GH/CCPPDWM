cmake_minimum_required(VERSION 3.26)
project(CCPPDWM)

set(VERSION "0.0.1")
add_compile_definitions(VERSION="${VERSION}")
set(CMAKE_CXX_STANDARD 20)

include_directories(Dwm Monitor Client Drw Fnt Cur .)


add_executable(${PROJECT_NAME}
        drw.h
        drw.cpp
        util.h
        util.cpp
        dwm.cpp
)
set(X11INC /usr/X11R6/include)
set(FREETYPEINC /usr/include/freetype2)
# 设置编译选项
target_include_directories(${PROJECT_NAME} PRIVATE ${X11INC} ${FREETYPEINC})

#set(X11LIB /usr/X11R6/lib)
set(X11LIB)
#set(X11INC /usr/X11R6/include)
set(X11INC)
set(FREETYPELIBS -lfontconfig -lXft)
# 设置链接库
target_link_libraries(${PROJECT_NAME} PRIVATE ${X11LIB} X11 ${XINERAMALIBS} ${FREETYPELIBS} Xrender)
