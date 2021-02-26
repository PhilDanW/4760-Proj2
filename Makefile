appname1 := master
srcfiles := $(shell find . -name "master*.cpp")
objects1  := $(patsubst %.cpp, %.o, $(srcfiles))

all: $(appname1)

$(appname1): $(objects1)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(appname1) $(objects1) $(LDLIBS)

# App 2 - builds the bin_adder program
appname2 := bin_adder
srcfiles := $(shell find . -name "bin_adder*.cpp")
objects2  := $(patsubst %.cpp, %.o, $(srcfiles))

all: $(appname2)

$(appname2): $(objects2)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(appname2) $(objects2) $(LDLIBS)

clean:
	rm -f $(objects1)
	rm -f $(appname1)
	rm -f $(objects2)
	rm -f $(appname2)
