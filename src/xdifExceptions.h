#ifndef XDIF_INCL_xdifExceptions
#define XDIF_INCL_xdifExceptions

#include <exception>

/****************************************************************************
 * Exceptions
 ****************************************************************************/
class EBadChunkType : public std::exception {
	virtual const char *what() { return "invalid chunk ID"; }
};

class EPayloadError : public std::exception {
	virtual const char *what() { return "payload error"; }
};

#endif
