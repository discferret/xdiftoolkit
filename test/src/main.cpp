/******
 * Let it be known that I really don't think this API is optimal. A better
 * way to do this would probably be to make Chunk::deserialisePayload
 * work on the object itself instead of returning a Chunk*, then tweak
 * Chunk::deserialise to create a new object then deserialise into it.
 *
 * Although that may be slightly less clean? Hm.
 *
 * Also, deserialise() and (probably) deserialisePayload should be made static,
 * as they have no effect on the class itself. Then again maybe not?
 *
 * TODO: add checks for sizeof(packet) > 32bit maxval (uint32_max)
 * TODO: make deserialisePayload() take an Iterator, which would reduce
 * the memory usage of this code SIGNIFICANTLY on large XDIF files.
 */


#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <list>
#include <map>
#include <iostream>
#include <exception>

#include "xdifExceptions.h"
#include "SerialisedPayload.h"
#include "XDIFChunk.h"
#include "METAChunk.h"
#include "ContainerChunk.h"
#include "LeafChunk.h"
#include "Init.h"

using namespace std;

/****************************************************************************/

int main(void)
{
	// set up XDIF library
	XDIFLibraryInit();

	// dump list of chunk prototypes
	chunkFactory().dump();

	ContainerChunk *ch = new XDIFChunk();

	METAChunk *meta = new METAChunk();
	meta->payload.push_back('f');
	meta->payload.push_back('o');
	meta->payload.push_back('o');
	meta->payload.push_back('b');
	meta->payload.push_back('a');
	meta->payload.push_back('r');
	ch->addChild(meta);
//	delete meta;
	cerr << "Deleting meta..." << endl;
	delete meta;

	// dump data to stdout
	vector<uint8_t> data = ch->serialise();
/*	for (vector<uint8_t>::size_type i=0; i<data.size(); i++) {
		cout << data[i];
	}
*/
	// now try and deserialise it
	cerr << "deserialising..." << endl;
	Chunk *c = Chunk::deserialise(data);
	cerr << "chunktype: " << c->getChunkType() << endl;

	cerr << "Deleting c..." << endl;
	delete c;

	cerr << "Deleting ch..." << endl;
	delete ch;

	return 0;
}
