#-------------------------------------------------
#
# Project created by QtCreator 2013-03-03T12:22:01
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = Assignment_3
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += /home/jim/boost_1_52_0
LIBS += -L/home/jim/boost_1_52_0/stage/lib -lboost_system -lboost_thread -lboost_chrono

SOURCES += main.cpp \
    publisher.cpp \
    subscriber.cpp \
    communication.cpp

HEADERS += \
    publisher.h \
    subscriber.h \
    communication.h \
    options.h
