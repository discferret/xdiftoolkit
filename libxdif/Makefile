OBJS = Chunk.o ContainerChunk.o METAChunk.o XDIFChunk.o
CXX = g++
CFLAGS = -O2
CXXFLAGS = -std=gnu++0x

xdif.a: $(addprefix obj/, $(OBJS))
	ar crv $@ $^

obj:
	mkdir obj

obj/%.o: src/%.cpp obj
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) -o $@ $<
