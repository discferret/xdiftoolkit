#include "Chunk.h"
#include "XDIFChunk.h"
#include "METAChunk.h"
#include "XDIFLibraryInit.h"

void XDIFLibraryInit(void)
{
	chunkFactory().registerClass("XDIF", new XDIFChunk());
	chunkFactory().registerClass("META", new METAChunk());
}
