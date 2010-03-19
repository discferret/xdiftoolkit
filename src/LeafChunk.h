#ifndef XDIF_INCL_LeafChunk
#define XDIF_INCL_LeafChunk

#include <string>
#include "Chunk.h"

/****************************************************************************
 * A Leaf Chunk. A Chunk that can't contain other Chunks.
 ****************************************************************************/
class LeafChunk : public Chunk {
	protected:
		// serialise only the packet payload
		virtual SerialisedPayload serialisePayload(void) const =0;
		// deserialise the packet payload into a chunk
		virtual Chunk *deserialisePayload(std::string chunkID, SerialisedPayload data) const =0;
	public:
		LeafChunk() {};
		LeafChunk(const LeafChunk &copy);
		virtual ~LeafChunk() {};

		virtual Chunk *clone() const =0;
		virtual Chunk *create(SerialisedPayload payload) const =0;
		virtual Chunk *create() const =0;

		virtual std::string getChunkType() const =0;
};

#endif

