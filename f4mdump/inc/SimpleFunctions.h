
#ifndef _SIMPLE_FUNCTIONS_H_
#define _SIMPLE_FUNCTIONS_H_

/*************************************************************************************************
* Includes
**************************************************************************************************/
#include "SimpleDataTypes.h"

#include <string>
#include <cstdarg>
#include <cstdio>

ByteBuffer_t SubBuffer(const ByteBuffer_t &buff, const size_t &startPos, const size_t &subSize);
ByteBuffer_t StrToBuff(const std::string &str);
void AppendToBuff(ByteBuffer_t &baseBuff, const ByteBuffer_t &buffToAppend);


void LogInfo(const std::string msg);
void LogDebug(const std::string msg);
void LogError(const std::string msg);

std::string StringFormat(const char* format, ...) ;
bool ReadWholeFile(const std::string &filePath, ByteBuffer_t &buffer);

size_t replace_fwrite(const void* buff, size_t size, size_t count, FILE* file);

bool IsHttpUrl(const std::string &url);
void b64decode(const std::string &str, ByteBuffer_t &buff);

#endif //_SIMPLE_FUNCTIONS_H_
