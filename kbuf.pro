TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

HEADERS +=  ./inc/protogenerator.h \
            ./inc/datawapperegenerator.h \
			./inc/common.h \
			../Base/*.h

DISTFILES += makefile

SOURCES += \
    ./src/protogenerator.cpp \
    ./src/datawapperegenerator.cpp \
    /src/common.cpp \
    ./src/main.cpp \


INCLUDEPATH += ./inc/ \
              ../
