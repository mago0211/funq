#-------------------------------------------------
#
# Project created by QtCreator 2014-02-07T10:16:42
#
#-------------------------------------------------

QT       += core

QT       -= gui

DESTDIR     = ../bin

isEmpty(PREFIX) { PREFIX = /usr/local }
target.path = $$PREFIX/bin/
INSTALLS += target

TARGET = funq
CONFIG   += console
CONFIG   -= app_bundle
DEFINES  += EXECUTABLE
TEMPLATE = app


SOURCES += main.cpp \
    attacher.cpp

HEADERS += \
    attacher.h