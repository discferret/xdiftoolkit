#ifndef XDIF_INCL_Chunk
#define XDIF_INCL_Chunk

#include <map>
#include <vector>
#include <string>
#include "SerialisedPayload.h"
#include "xdifExceptions.h"

/****************************************************************************
 * Interface for a chunk. This is what every chunk must be able to do.
 ****************************************************************************/
class Chunk {
	protected:
		// serialise only the packet payload
		virtual SerialisedPayload serialisePayload() const =0;
		// deserialise the packet payload into a chunk
		virtual Chunk *deserialisePayload(std::string chunkID, SerialisedPayload data) const =0;
	public:
		Chunk() {};
		Chunk(const Chunk &copy) {};
		virtual ~Chunk() {};

		// get the 4-character typestring for this chunk
		virtual std::string getChunkType() const =0;
		// serialise this chunk into a bytestream
		std::vector<uint8_t> serialise(void) const;
		// deserialise a new chunk from a bytestream
		static Chunk *deserialise(std::vector<uint8_t> data);

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

class _x_ChunkFactory {
	private:
		// typing saver!
		typedef std::map<std::string, Chunk *> _T;

		_T creationMap;

	public:
		// ctor
		_x_ChunkFactory()
		{
			// This is utterly pointless (maps start out empty), but it makes
			// it VERY clear to the compiler / linker that the singleton
			// object must be destroyed AFTER we've finished with it.
			// See C++ FAQ sec 10.14 for more info on this. Ugh.
			creationMap.clear();
		};

		// dtor
		~_x_ChunkFactory()
		{
			// make sure all the prototypes in creationMap are freed
			// admittedly this is mostly to appease the beast that is Valgrind...
			while (!creationMap.empty()) {
				_T::iterator i = creationMap.begin();
				// map->erase() invalidates the iterator, so keep a copy of the
				// chunk pointer to work from
				Chunk *x = (*i).second;
				// remove the chunk from the map
				creationMap.erase(i);
				// destroy the chunk
				delete x;
			}
		}

		// Dump a list of class prototypes
		void dump() {
			std::cerr << creationMap.size() << " prototypes in chunk factory:" << std::endl;
			for (_T::iterator i = creationMap.begin(); i!=creationMap.end(); i++) {
				std::cerr << "\tChunktype '" << (*i).first << "' => " <<
					(*i).second << std::endl;
			}
		}

		// register a new class with the factory
		void registerClass(std::string chunkID, Chunk *prototype)
		{
			// only add if there isn't an object of this type in the map
			if (creationMap.count(chunkID) == 0) {
				// add to the map
				creationMap[chunkID] = prototype;
			}
		}

		// create a chunk of a specified type
		Chunk *create(std::string chunkID)
		{
			// make sure there's a chunk of this type in the map
			if (creationMap.count(chunkID) == 0) throw new EBadChunkType();

			// build a chunk based on the prototype
			return creationMap[chunkID]->create();
		}

		// create a chunk of a specified type and deserialise the payload
		Chunk *create(std::string chunkID, SerialisedPayload payload)
		{
			// make sure there's a chunk of this type in the map
			if (creationMap.count(chunkID) == 0) throw new EBadChunkType();

			// build a chunk based on the prototype
			return creationMap[chunkID]->create(payload);
		}
};

_x_ChunkFactory &chunkFactory();

#endif

