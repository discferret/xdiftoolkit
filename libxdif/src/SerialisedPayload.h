#ifndef XDIF_INCL_SerialisedPayload
#define XDIF_INCL_SerialisedPayload

#include <vector>
#include <cstdint>

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
		std::vector<uint8_t> data;
};

#endif

