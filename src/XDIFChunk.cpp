#include "XDIFChunk.h"

// NOTE: should be in the .cpp file, not the header
static class _x_XDIFChunkInitialiser {
	public:
		_x_XDIFChunkInitialiser() {
			std::cerr << "XDIF chunkinit" << std::endl;
			chunkFactory().registerClass("XDIF", new XDIFChunk());
		}
} _xv_XDIFChunkInitialiser;


