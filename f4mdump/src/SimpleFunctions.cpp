#include <stdio.h>
#include <cstring>
#include <cerrno>
#include <fcntl.h>

#include "b64.h"
#include "SimpleDataTypes.h"
#include "SimpleFunctions.h"
#include "StringHelper.h"
#include "debug.h"
#include "console.h"

#include <vector>

/*****************************************************************************
 * Using namespace(s)
 ****************************************************************************/
using namespace std;

ByteBuffer_t SubBuffer(const ByteBuffer_t &buff, const size_t &startPos, const size_t &subSize)
{
    size_t buffSize = buff.size();
    if(buffSize > startPos && subSize > 0)
    {
        size_t endPos = ((startPos + subSize) <  buffSize) ? startPos + subSize -1 : buffSize - 1;
        return ByteBuffer_t(&buff[startPos], &buff[endPos] + 1);
    }
    return ByteBuffer_t();
}

ByteBuffer_t StrToBuff(const string &str)
{
    ByteBuffer_t buffer;
    buffer.resize( str.size() );
    for(size_t i=0; i < str.size(); ++i)
    {
        buffer[i] = static_cast<uint8_t>( str[i] );
    }
    return buffer;
}

void AppendToBuff(ByteBuffer_t &baseBuff, const ByteBuffer_t &buffToAppend) 
{ 
    baseBuff.insert( baseBuff.end(), buffToAppend.begin(), buffToAppend.end() ); 
}

void LogInfo(const string msg)
{
    printDBG("%s\n", msg.c_str());
}

void LogDebug(const std::string msg)
{
    printDBG("%s\n", msg.c_str());
}

void LogError(const string msg)
{
    printDBG("%s\n", msg.c_str());
}

std::string StringFormat(const char* fmt, ...) 
{
    static char textString[4096];

    va_list args;
    va_start( args, fmt );
    vsnprintf( textString, sizeof(textString), fmt, args );
    va_end( args );
    //std::string retStr = textString;
    return textString;
}

bool ReadWholeFile(const string &filePath, ByteBuffer_t &buffer)
{
    if(!filePath.empty())
    {
        FILE *pFile = fopen(filePath.c_str(), "rb");
        if(pFile)
        {
            // get file size to determinate needed buffer size
            fseek(pFile, 0, SEEK_END);
            size_t fileSize = ftell(pFile);
            buffer.resize(fileSize);
            // mov to beginin
            rewind(pFile);

            size_t readSize = fread(&buffer[0], 1, fileSize, pFile);
            fclose(pFile);

            if(fileSize == readSize)
            {
                return true;
            }
            else
            {
                LogError(StringFormat("ReadWholeFile problem with reading file: [%s] FileSize[%d] ReadSize[%d]", filePath.c_str(), fileSize, readSize));
            }
        }
        else
        {
            LogError(StringFormat("ReadWholeFile problem with opening file: [%s]", filePath.c_str()));
        }
    }
    else
    {
        LogError(StringFormat("ReadWholeFile empty file path!"));
    }

    return false;
}


size_t replace_fwrite(const void* buff, size_t size, size_t count, FILE* file)
{
    size_t pos = 101;

    size_t cursor = ftell(file);
    if(cursor <= pos && (cursor + count) >= pos )
    {
        ;//int a = 10;
    }

    return fwrite(buff, size, count, file);
}


bool IsHttpUrl(const string &url)
{
    return CStringHelper::startsWith(url, "http://");
}

void b64decode(const string &str, ByteBuffer_t &buff)
{
    size_t  cb  = b64_decode(str.c_str(), str.size(), NULL, 0);
    buff.resize(cb);
    memset(&buff[0], 0, buff.size());

    cb = b64_decode(str.c_str(), str.size(), &buff[0], cb);
    buff.resize(cb);
}


