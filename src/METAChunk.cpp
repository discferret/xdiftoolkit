#include <list>
#include <vector>
#include <string>
#include <iostream>

#include "SerialisedPayload.h"
#include "METAChunk.h"

using namespace std;

// NOTE: should be in the .cpp file, not the header
static class _x_METAChunkInitialiser {
	public:
		_x_METAChunkInitialiser() {
			std::cerr << "META chunkinit" << std::endl;
			chunkFactory().registerClass("META", new METAChunk());
		}
} _xv_METAChunkInitialiser;


/****************************************************************************/

// serialise a META chunk
SerialisedPayload METAChunk::serialisePayload(void) const
{
	SerialisedPayload sp;

	// set "has children" flag
	sp.hasChildren = false;

	// store payload
	sp.data.insert(sp.data.end(), payload.begin(), payload.end());

	return sp;
}

// deserialise a META chunk
Chunk *METAChunk::deserialisePayload(string chunkID, SerialisedPayload data) const
{
	cerr << "DeserialiseMeta: " << endl << "\t";
	for (size_t i=0; i<data.data.size(); i++) {
		cerr << (char)data.data[i] << " ";
	}
	cerr << endl;

	return new METAChunk();
}

