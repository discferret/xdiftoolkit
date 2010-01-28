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

using namespace std;

/****************************************************************************
 * Exceptions
 ****************************************************************************/
class EBadChunkType : public exception {
	virtual const char *what() { return "invalid chunk ID"; }
};

class EPayloadError : public exception {
	virtual const char *what() { return "payload error"; }
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
		// deserialise the packet payload into a chunk
		virtual Chunk *deserialisePayload(string chunkID, SerialisedPayload data) const =0;
	public:
		Chunk() {};
		Chunk(const Chunk &copy) {};
		virtual ~Chunk() {};

		// get the 4-character typestring for this chunk
		virtual std::string getChunkType() const =0;
		// serialise this chunk into a bytestream
		vector<uint8_t> serialise(void) const;
		// deserialise a new chunk from a bytestream
		static Chunk *deserialise(vector<uint8_t> data);

		// make a copy of this chunk
		// this is used to get around the need for dynamic_casts and RTTI
		// and is a tidier solution, even if it fits more with Java conventions
		// than C++ ones.
		virtual Chunk *clone() const =0;

		// create a new, empty object of this type
		virtual Chunk *create() const =0;
		// create a new object of this type and deserialise the payload
		virtual Chunk *create(SerialisedPayload payload) const =0;
};

static class _x_ChunkFactory {
	private:
		// typing saver!
		typedef std::map<std::string, Chunk *> _T;

		// "construct on first use" idiom/pattern -- C++FAQ 10.13
		_T& creationMap(void)
		{
			static _T singleton;
			return singleton;
		}

	public:
		// ctor
		_x_ChunkFactory()
		{
			// This is utterly pointless (maps start out empty), but it makes
			// it VERY clear to the compiler / linker that the singleton
			// object must be destroyed AFTER we've finished with it.
			// See C++ FAQ sec 10.14 for more info on this. Ugh.
			creationMap().clear();
		};

		// dtor
		~_x_ChunkFactory()
		{
			// make sure all the prototypes in creationMap are freed
			// admittedly this is mostly to appease the beast that is Valgrind...
			while (!creationMap().empty()) {
				_T::iterator i = creationMap().begin();
				// map->erase() invalidates the iterator, so keep a copy of the
				// chunk pointer to work from
				Chunk *x = (*i).second;
				// remove the chunk from the map
				creationMap().erase(i);
				// destroy the chunk
				delete x;
			}
		}

		// Dump a list of class prototypes
		void dump() {
			cerr << creationMap().size() << " prototypes in chunk factory:" << endl;
			for (_T::iterator i = creationMap().begin(); i!=creationMap().end(); i++) {
				cerr << "\tChunktype '" << (*i).first << "' => " <<
					(*i).second << endl;
			}
		}

		// register a new class with the factory
		void registerClass(string chunkID, Chunk *prototype)
		{
			// only add if there isn't an object of this type in the map
			if (creationMap().count(chunkID) == 0) {
				// add to the map
				creationMap()[chunkID] = prototype;
			}
		}

		// create a chunk of a specified type
		Chunk *create(string chunkID)
		{
			// make sure there's a chunk of this type in the map
			if (creationMap().count(chunkID) == 0) throw new EBadChunkType();

			// build a chunk based on the prototype
			return creationMap()[chunkID]->create();
		}

		// create a chunk of a specified type and deserialise the payload
		Chunk *create(string chunkID, SerialisedPayload payload)
		{
			// make sure there's a chunk of this type in the map
			if (creationMap().count(chunkID) == 0) throw new EBadChunkType();

			// build a chunk based on the prototype
			return creationMap()[chunkID]->create(payload);
		}
} chunkFactory;

/****************************************************************************
 * A Container Chunk. A Chunk that can contain other Chunks.
 ****************************************************************************/
class ContainerChunk : public Chunk {
	private:
		list<Chunk *> children;
	protected:
		// serialise only the packet payload
		virtual SerialisedPayload serialisePayload() const;
		// deserialise the packet payload into a chunk
		virtual Chunk *deserialisePayload(string chunkID, SerialisedPayload data) const;
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
		virtual Chunk *create(SerialisedPayload payload) const =0;
		virtual Chunk *create() const =0;

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
		// deserialise the packet payload into a chunk
		virtual Chunk *deserialisePayload(string chunkID, SerialisedPayload data) const =0;
	public:
		LeafChunk() {};
		LeafChunk(const LeafChunk &copy);
		virtual ~LeafChunk() {};

		virtual Chunk *clone() const =0;
		virtual Chunk *create(SerialisedPayload payload) const =0;
		virtual Chunk *create() const =0;

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
		virtual Chunk *create(SerialisedPayload payload) const { return deserialisePayload("XDIF", payload); };
		virtual Chunk *create() const { return new XDIFChunk(); };

		virtual std::string getChunkType() const { return "XDIF"; };
};

// NOTE: should be in the .cpp file, not the header
static class _x_XDIFChunkInitialiser {
	public:
		_x_XDIFChunkInitialiser() {
			cerr << "XDIF chunkinit" << endl;
			chunkFactory.registerClass("XDIF", new XDIFChunk());
		}
} _xv_XDIFChunkInitialiser;

/****************************************************************************
 * XDIF:META chunk.
 ****************************************************************************/
class METAChunk : public LeafChunk {
	protected:
		// serialise only the packet payload
		virtual SerialisedPayload serialisePayload(void) const;
		// deserialise the packet payload into a chunk
		virtual Chunk *deserialisePayload(string chunkID, SerialisedPayload data) const;
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
		virtual Chunk *create(SerialisedPayload payload) const { return deserialisePayload("META", payload); };
		virtual Chunk *create() const { return new METAChunk(); };

		virtual std::string getChunkType() const { return "META"; };
};

// NOTE: should be in the .cpp file, not the header
static class _x_METAChunkInitialiser {
	public:
		_x_METAChunkInitialiser() {
			cerr << "META chunkinit" << endl;
			chunkFactory.registerClass("META", new METAChunk());
		}
} _xv_METAChunkInitialiser;


/****************************************************************************/

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

	return chunkFactory.create(chunktype, sp);
}


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

// deserialise a container chunk
Chunk *ContainerChunk::deserialisePayload(string chunkID, SerialisedPayload data) const
{
	// TODO
	cerr << "container chunk deserialise" << endl;

	// Check that this chunk either has children or is empty.
	if (!(data.data.empty() || data.hasChildren)) throw new EPayloadError();

	// If we have no children to process, return an empty chunk
	if (!data.hasChildren) return chunkFactory.create(chunkID);

	// create a new container chunk using the factory
	ContainerChunk *ch = dynamic_cast<ContainerChunk *>(chunkFactory.create(chunkID));

	// Now onto the real business of deserialisation...
	// Start at the beginning of the data buffer
	vector<uint8_t>::iterator it = data.data.begin();
	while (it != data.data.end()) {
		// save pointer to start of chunk
		vector<uint8_t>::iterator i_chunkstart = it;
		bool has_children = false;

		// read chunk type of first child
		char chunktype[5];
		for (int i=0; i<4; i++) {
			chunktype[i] = *it;
			it++;
		}
		chunktype[4] = '\0';

		cerr << "container deserialise: chunktype " << chunktype << endl;

		// read flag byte
		if ((*it) & 0x80) has_children = true;
		it++;

		// skip over reserved bytes
		it += 3;

		// read chunk size
		vector<uint8_t>::size_type sz = 0;
		for (int i=0; i<4; i++) {
			sz = (sz << 8) + (*it);
			it++;
		}

		// now we know where the data ends. slice!
		SerialisedPayload sliced_data;
		sliced_data.data.insert(sliced_data.data.end(), it, it+sz);

		// update root iterator
		it += sz;

		// create the child!
		ch->addChild(chunkFactory.create(chunktype, sliced_data));
	}

	// return the decoded chunk
	return ch;
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


/****************************************************************************/

int main(void)
{
	// dump list of chunk prototypes
	chunkFactory.dump();

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
