QT       += core core-private sql testlib

CONFIG += testcase

TARGET = sqlfstest
TEMPLATE = app

include($$PWD/src/sqlfs.pri)

SOURCES = \
    sqlfileengine.cpp \
    sqlfstest.cpp

HEADERS = \
    sqlfileengine.h \
    sqlfstest.h
