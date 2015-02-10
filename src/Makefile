#
#    File:   Makefile.example
#    Author: The SRI DECIPHER (TM) System
#    Date:   Thu Sep  9 12:04:47 1993
#
#    Description:
#	This is the example makefile to start from when adding new
#       modules to the DECIPHER System.  To use this makefile, first
#       copy it to your directory as the file "Makefile".  Second,
#       replace the word "Example" in the text below with the real name
#	of your library.  Next replace the the example filenames with
#	the names of your actual declarations and source files in the
#	appropriate variable definitions.  Finally clean up by deleting
#	any lines not relevant to your module and updating this header
#       to describe your new module.  Do not forget to use the proper
#       RCS keywords!
#
#    Copyright (c) 1993, SRI International.  All Rights Reserved.
#
#    $Header: /home/yisu/cvsroot/rflm/src/Makefile,v 1.4 2009/04/04 23:53:47 yisu Exp $
#

SRILM=/Users/yisu/local/srilm-1.5.10

# Include common SRILM variable definitions.
include $(SRILM)/common/Makefile.common.variables

# This should enable locale-specific string collation for vocabulary sorting
# (it will slow things down somewhat).
ADDITIONAL_CXXFLAGS = 

# Flags for generating "compact" data structures
COMPACT_FLAGS += -DUSE_SARRAY -DUSE_SARRAY_TRIE -DUSE_SARRAY_MAP2

# Define variables.

TEMPLATE_SOURCES = \

# Example Library declarations files.

EXTERNAL_LIB_HEADERS = \
	$(SRCDIR)/DecisionTree.h \
	$(SRCDIR)/NgramDecisionTree.h \
	$(SRCDIR)/MultipassNgramDT.h \
	$(SRCDIR)/MultipassNgramDTLM.h \
	$(SRCDIR)/Question.h \
	$(SRCDIR)/NgramQuestion.h \
	$(SRCDIR)/NgramDecisionTreeLM.h \
	$(SRCDIR)/Socket.h \
	$(SRCDIR)/ServerSocket.h \
	$(SRCDIR)/ClientSocket.h \
	$(SRCDIR)/ClientLM.h \
	$(SRCDIR)/LMServer.h \
	$(SRCDIR)/Protocol.h

INTERNAL_LIB_HEADERS = 

# Example Library source files.
LIB_SOURCES = 				\
	$(SRCDIR)/DecisionTree.cc \
	$(SRCDIR)/NgramDecisionTree.cc \
	$(SRCDIR)/NgramDecisionTreeInt.cc \
	$(SRCDIR)/MultipassNgramDT.cc \
	$(SRCDIR)/MultipassNgramDTInt.cc \
	$(SRCDIR)/NgramQuestion.cc \
	$(SRCDIR)/NgramDecisionTreeLM.cc \
	$(SRCDIR)/MultipassNgramDTLM.cc \
	$(SRCDIR)/Socket.cc \
	$(SRCDIR)/ServerSocket.cc \
	$(SRCDIR)/ClientSocket.cc \
	$(SRCDIR)/ClientLM.cc \
	$(SRCDIR)/LMServer.cc \
	$(SRCDIR)/Protocol.cc

# Example Library object files.
LIB_OBJECTS = $(patsubst $(SRCDIR)/%.cc,$(OBJDIR)/%.o,\
		$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(LIB_SOURCES)))

# Example Library.
LIBRARY = $(OBJDIR)/librflm$(LIBSUFFIX).a

ADDITIONAL_INCLUDES += \
	$(TCL_INCLUDE)

ADDITIONAL_LDFLAGS += \
	-u $(SYMBOL_PREFIX)matherr

OTHER_LIBRARIES = \
	-loolm \
	-lflm \
	-ldstruct \
	-lmisc

ADDITIONAL_LIBRARIES += \
	$(OTHER_LIBRARIES) \
	$(TCL_LIBRARY) \
	-lm

# Example programs.

TEST_PROGRAM_NAMES = \
	testDecisionTree \
	testSocket

REAL_PROGRAM_NAMES = \
	rfngram \
	rfngram-count

PROGRAM_NAMES = \
	$(TEST_PROGRAM_NAMES) \
	$(REAL_PROGRAM_NAMES)

PROGRAMS = $(PROGRAM_NAMES:%=$(BINDIR)/%$(EXE_SUFFIX))

PROGRAM_SOURCES = $(foreach prog,$(PROGRAM_NAMES),\
			$(wildcard $(SRCDIR)/$(prog).c) \
			$(wildcard $(SRCDIR)/$(prog).cc))
PROGRAM_OBJECTS = $(PROGRAM_NAMES:%=$(OBJDIR)/%.o)


# Libraries to be linked with the Example programs.
LIBRARIES = $(LIBRARY) \
	    $(ADDITIONAL_LIBRARIES)

# All of the types of files.

ALL_HEADERS = $(EXTERNAL_LIB_HEADERS) \
	      $(INTERNAL_LIB_HEADERS)

ALL_SOURCES = $(LIB_SOURCES) \
	      $(PROGRAM_SOURCES)

ALL_OBJECTS = $(LIB_OBJECTS) \
	      $(PROGRAM_OBJECTS)

ALL_LIBRARIES = $(LIBRARY)

ALL_PROGRAMS = $(PROGRAMS)

ALL_PROGRAM_NAMES = $(PROGRAM_NAMES)


# Define pseudo-targets.


# Make sure the library does not get deleted if the make is interrupted.
.PRECIOUS:	$(LIBRARY)


# Define targets.


all:	$(PROGRAMS)

$(LIBRARY):	$(LIB_OBJECTS)
	$(ARCHIVE) $@ $^ $(DEMANGLE_FILTER)
	$(RANLIB) $@ $(DEMANGLE_FILTER)

$(PROGRAMS): $(LIBRARY) $(OTHER_LIBRARIES)

# Variables and Targets for released system

EXPORTED_HEADERS   = $(EXTERNAL_LIB_HEADERS)
EXPORTED_LIBRARIES = $(LIBRARY)
EXPORTED_PROGRAMS  = $(REAL_PROGRAM_NAMES:%=$(BINDIR)/%$(EXE_SUFFIX))

release:	release-headers release-libraries release-programs

# Include common SRILM target definitions.
include $(SRILM)/common/Makefile.common.targets
