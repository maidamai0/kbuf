#
# C++ Compiler
CXX := g++
#
#
# Flags
#
#   Compiler
CXXFLAGS += -c -g -Wall -std=c++11
#
# Directories
#
#   Sources
SRCDIR := src
#
#   Objects
OBJDIR := obj
#
#
################################################################################
SRCS := $(wildcard $(SRCDIR)/*.cpp)
OBJS := $(subst $(SRCDIR)/,$(OBJDIR)/,$(SRCS:.cpp=.o))

CXXFLAGS +=  -I./inc -I../

.PHONY: all clean

all: ../json/schema/kb

../json/schema/kb: $(OBJS)
	$(CXX) $(OBJS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f $(OBJDIR)/*.o ../json/schema/kb
