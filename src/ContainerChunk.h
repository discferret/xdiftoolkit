#ifndef XDIF_INCL_ContainerChunk
#define XDIF_INCL_ContainerChunk

#include <list>
#include <string>
#include "Chunk.h"
#include "SerialisedPayload.h"

/****************************************************************************
 * A Container Chunk. A Chunk that can contain other Chunks.
 ****************************************************************************/
class ContainerChunk : public Chunk {
   private:
	   std::list<Chunk *> children;
   protected:
	   // serialise only the packet payload
	   virtual SerialisedPayload serialisePayload() const;
	   // deserialise the packet payload into a chunk
	   virtual Chunk *deserialisePayload(std::string chunkID, SerialisedPayload data) const;
   public:
	   typedef std::list<Chunk *>::size_type childID_T;

	   ContainerChunk() {};
	   ContainerChunk(const ContainerChunk &copy) {
		   // copy the children
		   for (std::list<Chunk *>::const_iterator i = copy.children.begin(); i!=copy.children.end(); i++)
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

#endif

