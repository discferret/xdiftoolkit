#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <list>
#include <iostream>
#include <exception>

using namespace std;

/****************************************************************************
 * Interface for a chunk. This is what every chunk must be able to do.
 ****************************************************************************/
class Chunk {
	public:
		Chunk() {};
		Chunk(const Chunk &copy) {};
		virtual ~Chunk() {};

		// get the 4-character typestring for this chunk
		virtual std::string getChunkType() const =0;
		// serialise this chunk into a bytestream
		virtual vector<uint8_t> serialise(void) const =0;
		// make a copy of this chunk
		virtual Chunk *clone() const =0;
};

/****************************************************************************
 * A Container Chunk. A Chunk that can contain other Chunks.
 ****************************************************************************/
class ContainerChunk : public Chunk {
	private:
		list<Chunk *> children;
	public:
		typedef list<Chunk *>::size_type childID_T;

		ContainerChunk() {};
		ContainerChunk(const ContainerChunk &copy) {
			// copy the children
			for (list<Chunk *>::const_iterator i = copy.children.begin(); i!=copy.children.end(); i++)
				this->children.push_back((*i)->clone());
		}
		virtual ~ContainerChunk() { this->clearChildren(); }

		virtual Chunk *clone() const =0;

		virtual std::string getChunkType() const =0;

		void addChild(Chunk *c);
		void eraseChild(childID_T id);
		void clearChildren();
		childID_T getChildCount() const;

		virtual vector<uint8_t> serialise(void) const;
};

/****************************************************************************
 * A Leaf Chunk. A Chunk that can't contain other Chunks.
 ****************************************************************************/
class LeafChunk : public Chunk {
	public:
		LeafChunk() {};
		LeafChunk(const LeafChunk &copy);
		virtual ~LeafChunk() {};

		virtual Chunk *clone() const =0;

		virtual std::string getChunkType() const =0;
		virtual vector<uint8_t> serialise(void) const =0;
};

/****************************************************************************
 * XDIF file root chunk
 ****************************************************************************/
class XDIFChunk : public ContainerChunk {
	public:
		XDIFChunk() {};
		XDIFChunk(const XDIFChunk &copy) {};
		virtual ~XDIFChunk(){};

		virtual Chunk *clone() const { return new XDIFChunk(*this); };

		virtual std::string getChunkType() const { return "XDIF"; };
};

/****************************************************************************
 * XDIF:META chunk.
 ****************************************************************************/
class METAChunk : public LeafChunk {
	public:
		vector<uint8_t> payload;

		METAChunk() {};
		METAChunk(const METAChunk &copy) {
			// copy payload data
			this->payload.insert(this->payload.end(),
					copy.payload.begin(), copy.payload.end());
		};
		virtual ~METAChunk() {};

		virtual Chunk *clone() const { return new METAChunk(*this); };

		virtual std::string getChunkType() const { return "META"; };
		virtual vector<uint8_t> serialise(void) const;
};


/****************************************************************************
 * Chunk Factory
 ****************************************************************************/
// exception for the factory
class EBadChunkType : public exception {
	virtual const char *what() { return "chunk ID invalid, cannot build!"; }
};

// A really kooky implementation of a variant of the factory design pattern
// (or possible abstract factory?)
class ChunkFactory {
	public:
		// build a new chunk of the specified type
		static Chunk *build(string id) {
			if		(id.compare("XDIF")==0) return new XDIFChunk();
			else if	(id.compare("META")==0) return new METAChunk();
			else throw new EBadChunkType();
		}
};

/****************************************************************************/

// serialise a container chunk
vector<uint8_t> ContainerChunk::serialise(void) const
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
		for (list<Chunk *>::const_iterator i=this->children.begin(); i != this->children.end(); i++) {
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
void ContainerChunk::addChild(Chunk *c)
{
	this->children.push_back(c->clone());
}

// delete child
void ContainerChunk::eraseChild(ContainerChunk::childID_T id)
{
	// get iterator that points to the child we want to delete
	list<Chunk *>::iterator i = this->children.begin();
	for (ContainerChunk::childID_T s=0; s<id; s++) i++;

	// save pointer to child
	Chunk *x = (*i);
	delete x;

	// remove child pointer from list
	this->children.erase(i);
}

// clear all children
void ContainerChunk::clearChildren()
{
	// deallocate memory used by children
	for (list<Chunk *>::iterator i = this->children.begin(); i != this->children.end(); i++) {
		delete (*i);
	}

	this->children.clear();
}

// get number of children
ContainerChunk::childID_T ContainerChunk::getChildCount() const
{
	return this->children.size();
}

/****************************************************************************/

// serialise a META chunk
vector<uint8_t> METAChunk::serialise(void) const
{
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
	data.push_back(payload.size() >> 8		& 0xff);
	data.push_back(payload.size()			& 0xff);

	// store payload
	data.insert(data.end(), payload.begin(), payload.end());

	return data;
}

/****************************************************************************/

int main(void)
{
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
	for (vector<uint8_t>::size_type i=0; i<data.size(); i++) {
		cout << data[i];
	}

	cerr << "Deleting ch..." << endl;
	delete ch;

	return 0;
}
