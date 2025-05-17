###############################################################################
#                                                                             #
# MAKEFILE for calendar                                                       #
#                                                                             #
# (c) Guy Wilson 2021                                                         #
#                                                                             #
###############################################################################

# Directories
SOURCE = src
BUILD = build
DEP = dep

# What is our target
TARGET = lottery

# Tools
CPP = gcc
LINKER = gcc

# postcompile step
PRECOMPILE = @ mkdir -p $(BUILD) $(DEP)
# postcompile step
POSTCOMPILE = @ mv -f $(DEP)/$*.Td $(DEP)/$*.d

CPPFLAGS = -c -O2 -Wall -pedantic
DEPFLAGS = -MT $@ -MMD -MP -MF $(DEP)/$*.Td

# Libraries
COMPILE.c = $(CPP) $(CPPFLAGS) $(DEPFLAGS) -o $@
LINK.o = $(LINKER) -o $@

CSRCFILES = $(wildcard $(SOURCE)/*.c)
OBJFILES = $(patsubst $(SOURCE)/%.c, $(BUILD)/%.o, $(CSRCFILES))
DEPFILES = $(patsubst $(SOURCE)/%.c, $(DEP)/%.d, $(CSRCFILES))

all: $(TARGET)

# Compile C/C++ source files
#
$(TARGET): $(OBJFILES)
	$(LINK.o) $^ $(EXTLIBS)

$(BUILD)/%.o: $(SOURCE)/%.c
$(BUILD)/%.o: $(SOURCE)/%.c $(DEP)/%.d
	$(PRECOMPILE)
	$(COMPILE.c) $<
	$(POSTCOMPILE)

.PRECIOUS = $(DEP)/%.d
$(DEP)/%.d: ;

-include $(DEPFILES)

clean:
	rm -r $(BUILD)
	rm -r $(DEP)
	rm $(TARGET)
