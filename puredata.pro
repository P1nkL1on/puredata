INCLUDEPATH += -I/usr/local/include/ -I/usr/include/OpenImageIO

LIBS += \
    -L/use/local/lib/ -lraylib -lm -lpthread -ldl -lGL -lX11 \
    -L/usr/lib/x86_64-linux-gnu -lOpenImageIO

HEADERS += \
    $$PWD/exceptions.h $$PWD/graph.h $$PWD/graph_impl.h $$PWD/node.h \
    $$PWD/nodes_impl.h $$PWD/expr.h $$PWD/view.h $$PWD/view_impl.h

SOURCES += $$PWD/graph_impl.cpp $$PWD/nodes_impl.cpp $$PWD/expr.cpp \
    $$PWD/view_impl.cpp $$PWD/main.cpp
