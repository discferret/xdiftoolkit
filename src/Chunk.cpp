#include <vector>
#include <string>
#include <iostream>
#include <map>

#include "Chunk.h"
#include "SerialisedPayload.h"
#include "xdifExceptions.h"

using namespace std;

// "construct on first use" idiom/pattern -- C++FAQ 10.13
_x_ChunkFactory &chunkFactory()
{
	static _x_ChunkFactory singleton;
	return singleton;
}

vector<uint8_t> Chunk::serialise(void) const
{
	vector<uint8_t> data;

	// get chunk type
	string chunktype = this->getChunkType();

	// check length of chunk type
	if (chunktype.size() != 4) throw new EBadChunkType();

	// serialise the chunk type
	for (string::size_type i=0; i<chunktype.size(); i++) {
		data.push_back(chunktype[i]);
	}

	// serialise the payload data
	SerialisedPayload payload = this->serialisePayload();

	// store the flag byte and 3 reserved bytes
	data.push_back(payload.hasChildren ? 0x80 : 0);
	data.push_back(0);
	data.push_back(0);
	data.push_back(0);
	
	// store the chunk length (MSB to LSB -- big endian)
	data.push_back((payload.data.size() >> 24) & 0xff);
	data.push_back((payload.data.size() >> 16) & 0xff);
	data.push_back((payload.data.size() >> 8 ) & 0xff);
	data.push_back((payload.data.size()      ) & 0xff);

	// store payload
	data.insert(data.end(), payload.data.begin(), payload.data.end());

	return data;
}

Chunk *Chunk::deserialise(vector<uint8_t> data)
{
	vector<uint8_t>::iterator it = data.begin();
	bool has_children = false;

	// read chunk type
	char chunktype[5];
	for (int i=0; i<4; i++) {
		chunktype[i] = *it;
		it++;
	}
	chunktype[4] = '\0';

	cerr << "deserialise: chunktype " << chunktype << endl;

	// read flag byte
	if ((*it) & 0x80) has_children = true;
	it++;

	if (has_children)	 cerr << "deserialise: has children" << endl;
					else cerr << "deserialise: no  children" << endl;

	// skip over reserved bytes
	it += 3;

	// read chunk size
	vector<uint8_t>::size_type sz = 0;
	for (int i=0; i<4; i++) {
		sz = (sz << 8) + (*it);
		it++;
	}

	// iterator now points to the start of the data payload. create a
	// SerialisedPayload obj and pass the data along
	SerialisedPayload sp;
	sp.hasChildren = has_children;
	sp.data.insert(sp.data.end(), it, it+sz);

	return chunkFactory().create(chunktype, sp);
}


