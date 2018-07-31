TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

HEADERS +=  ./inc/protogenerator.h \
            ./inc/datawapperegenerator.h \
            ./inc/common.h \
            ./inc/spdlog/*.h \
            ./inc/spdlog/details/*.h \
            ./inc/spdlog/sinks/*.h \
            ./inc/spdlog/fmt/*.h \
            ./inc/spdlog/fmt/bundled/*.h \
            ./inc/rapidjson/*.h \
            ./inc/rapidjson/internal/*.h \
            ./inc/rapidjson/error/*.h \
            ./inc/rapidjson/msinttypes/*.h \

DISTFILES += makefile

SOURCES += \
    ./src/protogenerator.cpp \
    ./src/datawapperegenerator.cpp \
    ./src/common.cpp \
    ./src/main.cpp \


INCLUDEPATH += ./inc/ \
