#ifndef XDIF_INCL_METAChunk
#define XDIF_INCL_METAChunk

#include <vector>
#include <iostream>
#include "LeafChunk.h"

/****************************************************************************
 * XDIF:META chunk.
 ****************************************************************************/
class METAChunk : public LeafChunk {
	protected:
		// serialise only the packet payload
		virtual SerialisedPayload serialisePayload(void) const;
		// deserialise the packet payload into a chunk
		virtual Chunk *deserialisePayload(std::string chunkID, SerialisedPayload data) const;
	public:
		std::vector<uint8_t> payload;

		METAChunk() {};
		METAChunk(const METAChunk &copy) {
			// copy payload data
			this->payload.insert(this->payload.end(),
					copy.payload.begin(), copy.payload.end());
		};
		virtual ~METAChunk() {};

		virtual Chunk *clone() const { return new METAChunk(*this); };
		virtual Chunk *create(SerialisedPayload payload) const { return deserialisePayload("META", payload); };
		virtual Chunk *create() const { return new METAChunk(); };

		virtual std::string getChunkType() const { return "META"; };
};

#endif

