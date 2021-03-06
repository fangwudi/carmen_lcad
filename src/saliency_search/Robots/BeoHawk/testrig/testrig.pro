######################################################################
# Automatically generated by qmake (2.01a) Fri Feb 19 01:01:34 2010
######################################################################

TEMPLATE = app
TARGET = testrig
DEPENDPATH +=	.
INCLUDEPATH += 	. \
		/usr/local/include \
		/opt/ARToolKit/include \
		/usr/share/qt4/include \
		/usr/share/qt4/include/Qt \
		/usr/share/qt4/include/QtCore \
		/usr/include/qwt-qt4

QMAKE_LIBDIR += /usr/local/lib \
		/opt/ARToolKit/lib

LIBS += -Wl,-Bstatic -lAR -Wl,-Bdynamic -lcv -lhighgui -lqwt-qt4 -lfftw3

# Input
HEADERS += Atomic6DOF.h DataThreads.h TestrigGUI.h ui_testrig.h SimpleRectangleFinder.h KalmanFilter.h Control.h
FORMS += ui_testrig.ui
SOURCES += Atomic6DOF.cc DataThreads.cc main.cc TestrigGUI.cpp SimpleRectangleFinder.cpp KalmanFilter.cc Control.cc

CONFIG += qt thread debug
