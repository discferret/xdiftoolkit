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
 * Exceptions
 ****************************************************************************/
class EBadChunkType : public exception {
	virtual const char *what() { return "invalid chunk ID"; }
};

/****************************************************************************
 * Serialised payload (with flags)
 ****************************************************************************/
class SerialisedPayload {
	public:
		// ctor
		SerialisedPayload() {
			this->hasChildren = false;
		};

		// TRUE if the payload contains serialised chunks
		// (that is, this chunk is a Container Chunk)
		// default = false
		bool hasChildren;
		// Payload data
		vector<uint8_t> data;
};

/****************************************************************************
 * Interface for a chunk. This is what every chunk must be able to do.
 ****************************************************************************/
class Chunk {
	protected:
		// serialise only the packet payload
		virtual SerialisedPayload serialisePayload() const =0;
	public:
		Chunk() {};
		Chunk(const Chunk &copy) {};
		virtual ~Chunk() {};

		// get the 4-character typestring for this chunk
		virtual std::string getChunkType() const =0;
		// serialise this chunk into a bytestream
		virtual vector<uint8_t> serialise(void) const {
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

			// store the chunk length
			// MSB to LSB -- note that MSbit is set if chunk contains children
			data.push_back(((payload.data.size() >> 56) & 0x7f) | (payload.hasChildren ? 0x80 : 0));
			data.push_back( (payload.data.size() >> 48) & 0xff);
			data.push_back( (payload.data.size() >> 40) & 0xff);
			data.push_back( (payload.data.size() >> 32) & 0xff);
			data.push_back( (payload.data.size() >> 24) & 0xff);
			data.push_back( (payload.data.size() >> 16) & 0xff);
			data.push_back( (payload.data.size() >> 8 ) & 0xff);
			data.push_back( (payload.data.size()      ) & 0xff);

			// store payload
			data.insert(data.end(), payload.data.begin(), payload.data.end());

			return data;
		}

		// deserialise a new chunk from a bytestream
//		virtual Chunk *deserialise(vector<uint8_t>) const =0;
		// make a copy of this chunk
		virtual Chunk *clone() const =0;
};

/****************************************************************************
 * A Container Chunk. A Chunk that can contain other Chunks.
 ****************************************************************************/
class ContainerChunk : public Chunk {
	private:
		list<Chunk *> children;
	protected:
		// serialise only the packet payload
		virtual SerialisedPayload serialisePayload() const;
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
};

/****************************************************************************
 * A Leaf Chunk. A Chunk that can't contain other Chunks.
 ****************************************************************************/
class LeafChunk : public Chunk {
	protected:
		// serialise only the packet payload
		virtual SerialisedPayload serialisePayload(void) const =0;
	public:
		LeafChunk() {};
		LeafChunk(const LeafChunk &copy);
		virtual ~LeafChunk() {};

		virtual Chunk *clone() const =0;

		virtual std::string getChunkType() const =0;
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
	protected:
		// serialise only the packet payload
		virtual SerialisedPayload serialisePayload(void) const;
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
};


/****************************************************************************
 * Chunk Factory
 ****************************************************************************/
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
SerialisedPayload ContainerChunk::serialisePayload() const
{
	// allocate local data store
	SerialisedPayload sp;

	// any children?
	if (this->children.size() > 0) {
		// set "Chunk Has Children" bit
		sp.hasChildren = true;

		// serialise children (depth-first traversal)
		for (list<Chunk *>::const_iterator i=this->children.begin(); i != this->children.end(); i++) {
			vector<uint8_t> x = (*i)->serialise();
			sp.data.insert(sp.data.end(), x.begin(), x.end());
		}
	}

	// return the serialised data
	return sp;
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
SerialisedPayload METAChunk::serialisePayload(void) const
{
	SerialisedPayload sp;

	// set "has children" flag
	sp.hasChildren = false;

	// store payload
	sp.data.insert(sp.data.end(), payload.begin(), payload.end());

	return sp;
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
