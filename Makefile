CC          = gcc
CXX         = g++
EXEC        = ./
CXXFLAGS    = -Wall -Wextra -std=c++17

LDLIBS	    += -lpthread

INC         =   .
SRCS        +=  $(INC)/main.cpp
SRCS        +=  $(INC)/utilities.cpp
SRCS        +=  $(INC)/recorder.cpp
SRCS        +=  $(INC)/SDCard.cpp

OBJDIR      = build
OBJS        = $(patsubst $(INC)/%.cpp, $(OBJDIR)/%.o, $(SRCS))

TARGET      = main

INCLUDES    = -I$(INC)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^ $(LDLIBS)

$(OBJDIR)/%.o: $(INC)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c -o $@ $< $(LDLIBS)

.PHONY: clean
clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: run
run: $(TARGET)
	@sudo $(EXEC)$(TARGET)
