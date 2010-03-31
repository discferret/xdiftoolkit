OBJS = Chunk.o ContainerChunk.o METAChunk.o XDIFLibraryInit.o
CXX = g++
CFLAGS = -I. -I.. -Iinclude/xdif -O2
CXXFLAGS = -std=gnu++0x

.PHONY: all clean

all: libxdif.a

clean:
	-rm -f $(addprefix obj/, $(OBJS))
	-rm -f libxdif.a

libxdif.a: $(addprefix obj/, $(OBJS))
	-rm -f libxdif.a
	ar cr $@ $^

obj/%.o: src/%.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) -o $@ $<
