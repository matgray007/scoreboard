# writeToMatrix Makefile

CXX = g++
AR = ar
RM = rm -f

# Paths
RGB_MATRIX_DIR	= ../matrix
INCLUDES 		= -I$(RGB_MATRIX_DIR)/include
LIBS 			= -L$(RGB_MATRIX_DIR)/lib

# Flags
CXXFLAGS = -w -Wall -std=c++11 -O2 $(INCLUDES) `pkg-config --cflags jsoncpp` `curl-config --cflags`
LDFLAGS  = $(LIBS) -lrgbmatrix -lpthread -lm -lrt `pkg-config --libs jsoncpp` `curl-config --libs`
MAGICK_CXXFLAGS?=$(shell GraphicsMagick++-config --cppflags --cxxflags)
MAGICK_LDFLAGS?=$(shell GraphicsMagick++-config --ldflags --libs)

# Target
TARGET = sendScores.exe

# Source files
SRCS = sendScores.cc
OBJS = $(SRCS:.cc=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS) $(MAGICK_LDFLAGS)

%.o: %.cc
	$(CXX) $(CXXFLAGS) $(MAGICK_CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)