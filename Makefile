#Philip Wright
#Last Updated: 8:00PM 2/25/2021
#makefile to compile master and bin_adder source files
exec1 := master
source := $(shell find . -name "master*.cpp")
objs1  := $(patsubst %.cpp, %.o, $(source))

all: $(exec1)

$(exec1): $(objs1)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(exec1) $(objs1) $(LDLIBS)

# App 2 - builds the bin_adder program
exec2 := bin_adder
source := $(shell find . -name "bin_adder*.cpp")
objs2  := $(patsubst %.cpp, %.o, $(source))

all: $(exec2)

$(exec2): $(objs2)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $(exec2) $(objs2) $(LDLIBS)

clean:
	rm -f $(objs1)
	rm -f $(exec1)
	rm -f $(obj2)
	rm -f $(exec2)
