# Just a simple makefile to build both the server and client
CXX := c++
CXXFLAGS := -O3 -std=c++11
LDFLAGS := -pthread
INCLUDE := -I

SRCDIR := Checkers-JPearl/src
OBJDIR := Checkers-JPearl/obj/Linux
BINDIR := Builds/Linux
	
MAINPROGRAM := Checkers-JPearl
CLIENTPROGRAM := CheckersClient-JPearl
	
MAINEXCLUDEOBJECTS := clientmain
CLIENTOBJECTS := dummy_client connection clientmain

## END INPUT VARIABLES ##

# Get all relevant files	
SRCFILES := $(wildcard $(SRCDIR)/*.cpp)
OBJFILES := $(addprefix $(OBJDIR)/,$(notdir $(SRCFILES:.cpp=.o)))

# Turn program file list into pattern list
MAINEXCLUDEOBJECTS := $(addprefix %,$(addsuffix .o,$(MAINEXCLUDEOBJECTS)))
CLIENTOBJECTS := $(addprefix %,$(addsuffix .o,$(CLIENTOBJECTS)))

# Ensures all programs are made in parameterless make call
all: $(addprefix $(BINDIR)/,$(MAINPROGRAM) $(CLIENTPROGRAM))

# Links main program
$(BINDIR)/$(MAINPROGRAM): $(filter-out $(MAINEXCLUDEOBJECTS), $(OBJFILES))
	$(CXX) $(LDFLAGS) $^ -o $@ 

# Links client program
$(BINDIR)/$(CLIENTPROGRAM): $(filter $(CLIENTOBJECTS), $(OBJFILES))
	$(CXX) $(LDFLAGS) $^ -o $@ 
	

# Compiles all source files
$(OBJDIR)/%.o: $(addprefix $(SRCDIR)/,%.cpp) | makedirs
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@ 

makedirs:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)
	
clean:
	-rm $(OBJDIR)/*
	-rm $(BINDIR)/*