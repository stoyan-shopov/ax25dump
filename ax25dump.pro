#-------------------------------------------------
#
# Project created by QtCreator 2017-06-01T16:47:01
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ax25dump
TEMPLATE = app

DEFINES += QT_TEST_DRIVE=1

SOURCES += main.cxx\
        mainwindow.cxx \
    ax25.c \
    ax25-packet-received-callback.cxx

HEADERS  += mainwindow.hxx \
    ax25.h

FORMS    += mainwindow.ui

SOURCES -= \
    ax25-packet-received-callback.cxx
