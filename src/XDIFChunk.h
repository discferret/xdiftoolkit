#ifndef XDIF_INCL_XDIFChunk
#define XDIF_INCL_XDIFChunk

#include <string>
#include <iostream>

#include "ContainerChunk.h"

/****************************************************************************
 * XDIF file root chunk
 ****************************************************************************/
class XDIFChunk : public ContainerChunk {
	public:
		XDIFChunk() {};
		XDIFChunk(const XDIFChunk &copy) {};
		virtual ~XDIFChunk(){};

		virtual Chunk *clone() const { return new XDIFChunk(*this); };
		virtual Chunk *create(SerialisedPayload payload) const { return deserialisePayload("XDIF", payload); };
		virtual Chunk *create() const { return new XDIFChunk(); };

		virtual std::string getChunkType() const { return "XDIF"; };
};

#endif

