#ifndef _SIMPLE_DATA_TYPES_H_
#define _SIMPLE_DATA_TYPES_H_

#include <vector>
#include <list>
#include <string>
#include <memory>

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include <inttypes.h>

/**
*	Definition of data types
*/

typedef std::vector<uint8_t>		ByteBuffer_t;
typedef std::vector<std::string>	StringArray_t;
typedef std::list<std::string>  	StringList_t;

#endif //_SIMPLE_DATA_TYPES_H_
