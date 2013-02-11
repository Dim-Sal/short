#-------------------------------------------------
#
# Project created by QtCreator 2013-02-02T12:29:29
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = Assignment_1
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    producer.cpp \
    consumer.cpp \
    market.cpp \
    run_options.cpp
INCLUDEPATH += /home/jim/boost_1_52_0
LIBS += -L/home/jim/boost_1_52_0/stage/lib -lboost_system -lboost_thread -lboost_chrono

HEADERS += \
    producer.h \
    consumer.h \
    market.h \
    run_options.h
