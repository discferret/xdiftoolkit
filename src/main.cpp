#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <list>
#include <iostream>
#include <exception>

using namespace std;

class Chunk {
	private:
		list<Chunk *> children;
	public:
		typedef list<Chunk *>::size_type childID_T;

		virtual ~Chunk(){ cerr<<"Destructor: Chunk"<<endl;}

		virtual std::string getChunkType() const =0;

		void addChild(Chunk *c);
		void eraseChild(childID_T id);
		childID_T getChildCount();
		void clearChildren();

		virtual vector<uint8_t> serialise(void);
};

class XDIFChunk : public Chunk {
	public:
		virtual std::string getChunkType() const { return "XDIF"; };
		virtual ~XDIFChunk(){ cerr<<"Destructor: XDIFChunk"<<endl;}

		// serialise is left as-is because this node contains other nodes,
		// not payload data
};

class METAChunk : public Chunk {
	public:
		vector<uint8_t> payload;
		
		virtual ~METAChunk(){ cerr<<"Destructor: METAChunk"<<endl;}

		virtual std::string getChunkType() const { return "META"; };
		virtual vector<uint8_t> serialise(void) {
			vector<uint8_t> data;

			// store the chunk type
			string chunktype = this->getChunkType();
			for (string::size_type i=0; i<chunktype.size(); i++) {
				data.push_back(chunktype[i]);
			}

			// store payload size
			data.push_back(payload.size() >> 56	& 0xff);
			data.push_back(payload.size() >> 48	& 0xff);
			data.push_back(payload.size() >> 40	& 0xff);
			data.push_back(payload.size() >> 32	& 0xff);
			data.push_back(payload.size() >> 24	& 0xff);
			data.push_back(payload.size() >> 16	& 0xff);
			data.push_back(payload.size() >> 8	& 0xff);
			data.push_back(payload.size()		& 0xff);

			// store payload
			data.insert(data.end(), payload.begin(), payload.end());

			return data;
		}
};

// exception for the factory
class EBadChunkType : public exception {
	virtual const char *what() { return "chunk ID invalid, cannot build!"; }
};

// A really kooky implementation of a variant of the factory design pattern
// (or possible abstract factory?)
class ChunkFactory {
	// build a new chunk of the specified type
	static Chunk *build(string id) {
		if		(id.compare("XDIF")==0) return new XDIFChunk();
		else if	(id.compare("META")==0) return new METAChunk();
		else throw new EBadChunkType();
	}

	// clone an existing chunk
	static Chunk *build(Chunk &c) {
		if		(c.getChunkType().compare("XDIF")==0)	return new XDIFChunk(dynamic_cast<XDIFChunk&>(c));
		else if	(c.getChunkType().compare("META")==0)	return new METAChunk(dynamic_cast<METAChunk&>(c));
		else throw new EBadChunkType();
	}
};

/****************************************************************************/

/*
// dtor for Chunk
Chunk::~Chunk()
{
}
*/
// serialise a chunk
vector<uint8_t> Chunk::serialise(void)
{
	// allocate local vector for data store
	vector<uint8_t> data;

	// store the chunk type
	string chunktype = this->getChunkType();
	for (string::size_type i=0; i<chunktype.size(); i++) {
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
		for (list<Chunk *>::iterator i=this->children.begin(); i != this->children.end(); i++) {
			vector<uint8_t> x = (*i)->serialise();
			data.insert(data.end(), x.begin(), x.end());
			chunksize += x.size();
		}
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

// add a new child
void Chunk::addChild(Chunk *c)
{
	this->children.push_back(c);
}

// delete child
void Chunk::eraseChild(Chunk::childID_T id)
{
	list<Chunk *>::iterator i = this->children.begin();
	for (Chunk::childID_T s=0; s<id; s++) i++;
	this->children.erase(i);
}

// get number of children
Chunk::childID_T Chunk::getChildCount()
{
	return this->children.size();
}

// clear all children
void Chunk::clearChildren()
{
	this->children.clear();
}

int main(void)
{
	Chunk *ch = new XDIFChunk();

	METAChunk *meta = new METAChunk();
	meta->payload.push_back('f');
	meta->payload.push_back('o');
	meta->payload.push_back('o');
	meta->payload.push_back('b');
	meta->payload.push_back('a');
	meta->payload.push_back('r');
	ch->addChild(meta);
//	delete meta;

	// dump data to stdout
	vector<uint8_t> data = ch->serialise();
	for (vector<uint8_t>::size_type i=0; i<data.size(); i++) {
		cout << data[i];
	}

	cerr << "Deleting ch..." << endl;
	delete ch;
	cerr << "Deleting meta..." << endl;
	delete meta;

	return 0;
}
