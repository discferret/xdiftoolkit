#include <list>
#include <vector>
#include <string>
#include <iostream>

#include "ContainerChunk.h"

using namespace std;

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
	if (!data.hasChildren) return chunkFactory().create(chunkID);

	// create a new container chunk using the factory
	ContainerChunk *ch = dynamic_cast<ContainerChunk *>(chunkFactory().create(chunkID));

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
		ch->addChild(chunkFactory().create(chunktype, sliced_data));
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


