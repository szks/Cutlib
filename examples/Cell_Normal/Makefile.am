###
##############################################################################
#
# Cutlib - Cut Information Library
#
# Copyright (c) 2010-2011 VCAD System Research Program, RIKEN.
# All rights reserved.
#
# Copyright (c) 2012-2014 Advanced Institute for Computational Science, RIKEN.
# All rights reserved.
#
##############################################################################
###

noinst_PROGRAMS=a.out

DISTCLEANFILES=*~
CLEANFILES=data/*.log

a_out_SOURCES = \
    Config.h \
    main.cpp \
    ../util/src/ConfigBase.cpp \
    ../util/src/ConfigFile.cpp \
    ../util/src/CutTest.cpp \
    ../util/src/outputVtk.cpp \
    ../util/include/ConfigBase.h \
    ../util/include/ConfigFile.h \
    ../util/include/CutTest.h \
    ../util/include/outputVtk.h \
    ../util/include/Tuple.h

a_out_CXXFLAGS = \
    -I$(top_builddir)/include \
    -I../util/include \
    @CT_CFLAGS@ @PL_CFLAGS@ @TP_CFLAGS@

a_out_LDADD = \
    -L$(top_builddir)/src \
    @CT_LIBS@ @PL_LDFLAGS@ @TP_LDFLAGS@

EXTRA_DIST=small.tpp test-large.conf test.conf Makefile_hand large.tpp

.PHONY:$(dist_noinst_DATA)

