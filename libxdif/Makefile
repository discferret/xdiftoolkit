OBJS = Chunk.o ContainerChunk.o METAChunk.o XDIFLibraryInit.o
CXX = g++
CFLAGS = -O2
CXXFLAGS = -std=gnu++0x

libxdif.a: $(addprefix obj/, $(OBJS))
	ar crv $@ $^

obj/%.o: src/%.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) -o $@ $<
