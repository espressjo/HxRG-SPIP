TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += thread

QMAKE_LFLAGS +=-Wl,--rpath=/opt/HxRG-SPIP/lib

SOURCES += \
        ../src/main.cpp \
        ../src/macie_control.cpp \
        ../src/init_handler.cpp \
        ../src/hxrg_init_config.cpp \
        ../fits2ramp-utils/posemeter/src/posemeter.cpp \
        ../src/hxrg_get_status.cpp \
        ../src/uics_base64.cpp \
        ../src/autocheck.cpp \
        ../fits2ramp-utils/fitslib2/src/fl2_header.cpp \
        ../src/inst_time.cpp \
        ../fits2ramp-utils/refpxcorr/src/refpxcorr2.cpp \
        ../src/argument_parser.cpp \
        ../fits2ramp-utils/fitslib2/src/fitsheader.cpp \
        ../fits2ramp-utils/fits2ramp/src/fits2ramp3.cpp \
        ../fits2ramp-utils/fitslib2/src/readim.cpp \
        ../src/pheader.cpp


		
HEADERS += \
    ../src/insthandle.h \
    ../fits2ramp-utils/fits2ramp/src/fits2ramp2.h \
    ../fits2ramp-utils/fitslib2/src/readim.h \
    ../fits2ramp-utils/fits2ramp/src/fits2ramp3.h \
    ../fits2ramp-utils/refpxcorr/src/medianfilter.h \
    ../fits2ramp-utils/fits2ramp/src/medflt.h \
    ../src/hxrg_get_status.h \
    ../src/macie_control.h \
    ../src/init_handler.h \
    ../src/hxrg_init_config.h \
    ../src/autocheck.h \
    ../src/uics_base64.h \
    ../fits2ramp-utils/posemeter/src/posemeter.h \
    ../src/regdef.h \
    ../fits2ramp-utils/fitslib2/src/fl2_header.h \
    ../fits2ramp-utils/fitslib2/src/fitslib2.h \
    ../src/nirpsexpstatus.h \
    ../fits2ramp-utils/refpxcorr/src/refpxcorr.h \
    ../fits2ramp-utils/refpxcorr/src/refpxcorr2.h \
    ../src/inst_time.h \
    ../src/argument_parser.h \
    ../fits2ramp-utils/fitslib2/src/fitsheader.h \
    ../src/hxrg_conf.h
   


LIBS += -L/opt/HxRG-SPIP/lib -lMACIE
LIBS += -L/opt/HxRG-SPIP/lib -lm
LIBS += -L$$PWD/../UICS/lib -luics
LIBS += -L$$PWD/../fits2ramp-utils/lib -lf2r

INCLUDEPATH += \
        ../UICS/src/ \
        ../UICS/lib/ \
        ../fits2ramp-utils/lib \
        ../fits2ramp-utils/fits2ramp/src/ \
        /opt/HxRG-SPIP/lib \
        ../fits2ramp-utils/fitslib2/src/ \
        ../fits2ramp-utils/refpxcorr/src/ \
        ../fits2ramp-utils/posemeter/src/


DEPENDPATH += \
        ../UICS/src/ \
        ../lib \
        ../fits2ramp-utils/lib \
        ../UICS/lib/ \
        ../fits2ramp-utils/fits2ramp/src/ \
        ../fits2ramp-utils/fitslib2/src/ \
        ../fits2ramp-utils/refpxcorr/src/ \
        ../fits2ramp-utils/posemeter/src/


