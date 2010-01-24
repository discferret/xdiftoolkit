#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <iostream>
#include <exception>

using namespace std;

class Chunk {
	private:
		vector<Chunk> children;
		char chunktype[4];
	public:
		vector<uint8_t> payload;

		Chunk();

		virtual vector<uint8_t> serialise(void);
		void addChild(const Chunk c);
		void setChunktype(const char *ct);
};

class EBadChunktype : public exception {
	virtual const char* what() { return "bad chunk type specified"; }
};

// ctor for Chunk
Chunk::Chunk()
{
	// clear chunk type
	for (size_t i=0; i<sizeof(chunktype); i++) {
		chunktype[i] = ' ';
	}
}

// serialise a chunk
vector<uint8_t> Chunk::serialise(void)
{
	// allocate local vector for data store
	vector<uint8_t> data;

	// store the chunk type
	for (size_t i=0; i<sizeof(this->chunktype); i++) {
		data.push_back(chunktype[i]);
	}

	// 8 placeholder bytes for the length
	for (int i=0; i<8; i++) {
		data.push_back('\0');
	}

	// keep track of chunk size
	uint64_t chunksize = 0;

	// any children?
	if (this->children.size() > 0) {
		// set "Chunk Has Children" bit
		data.at(3) |= 0x80;

		// serialise children (depth-first traversal)
		for (vector<Chunk>::size_type i=0; i<this->children.size(); i++) {
			vector<uint8_t> x = this->children.at(i).serialise();
			data.insert(data.end(), x.begin(), x.end());
			chunksize += x.size();
		}
	} else {
		// no, store the data instead
		data.insert(data.end(), this->payload.begin(), this->payload.end());
		chunksize += this->payload.size();
	}

	// store the chunk length
	data[ 4] = chunksize >> 56;
	data[ 5] = chunksize >> 48;
	data[ 6] = chunksize >> 40;
	data[ 7] = chunksize >> 32;
	data[ 8] = chunksize >> 24;
	data[ 9] = chunksize >> 16;
	data[10] = chunksize >> 8;
	data[11] = chunksize;

	// return the serialised data
	return data;
}

void Chunk::addChild(const Chunk c)
{
	this->children.push_back(c);
}

void Chunk::setChunktype(const char *ct)
{
	// check chunktype length
	if (strlen(ct) != 4) throw new EBadChunktype();

	// make sure chunktype is valid (printables only)
	for (size_t i=0; i<4; i++) {
		if (ct[i] >= 0x80) throw new EBadChunktype();
	}

	// now copy the chunktype into the buffer
	for (size_t i=0; i<4; i++) {
		this->chunktype[i] = ct[i];
	}
}

int main(void)
{
	Chunk ch;
	ch.setChunktype("xDIF");

	Chunk *meta = new Chunk();
	meta->setChunktype("META");
	meta->payload.push_back('f');
	meta->payload.push_back('o');
	meta->payload.push_back('o');
	meta->payload.push_back('b');
	meta->payload.push_back('a');
	meta->payload.push_back('r');
	ch.addChild(*meta);
	delete meta;

	// dump data to stdout
	vector<uint8_t> data = ch.serialise();
	for (vector<uint8_t>::size_type i=0; i<data.size(); i++) {
		cout << data[i];
	}

	return 0;
}
