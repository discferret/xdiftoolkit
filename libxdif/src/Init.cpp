#include "Chunk.h"
#include "XDIFChunk.h"
#include "METAChunk.h"
#include "Init.h"

void XDIFLibraryInit(void)
{
	chunkFactory().registerClass("XDIF", new XDIFChunk());
	chunkFactory().registerClass("META", new METAChunk());
}
