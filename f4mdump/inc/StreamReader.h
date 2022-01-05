#ifndef _STREAM_READER_H_
#define _STREAM_READER_H_

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include "SimpleDataTypes.h"
#include "StreamReader.h"

namespace iptv
{

enum EOffsetwhence
{
    READER_SEEK_SET=0,
    READER_SEEK_CUR=1,
    READER_SEEK_END=2,
};

class IStreamReader
{
public:
    virtual int64_t read(void *buf, uint32_t count) = 0;
    virtual int64_t seek(int32_t offset, EOffsetwhence whence) = 0;
    virtual int64_t size() = 0;
    virtual int64_t offset() = 0;
    
    virtual uint8_t  readUInt8();
    virtual uint16_t readUInt16();
    virtual uint32_t readUInt24();
    virtual uint32_t readUInt32();
    virtual uint64_t readUInt64();
    virtual double   readDouble();
    virtual std::string readString();
    virtual std::string readString(const uint32_t len);
};

class CBufferReader : public IStreamReader
{
public:
    CBufferReader(const ByteBuffer_t &buffer);
    CBufferReader(const uint8_t *pBuffer, const uint32_t size);
    virtual ~CBufferReader();
    
    virtual int64_t read(void *buf, uint32_t count);
    virtual int64_t seek(int32_t offset, EOffsetwhence whence);
    virtual int64_t size();
    virtual int64_t offset();
private:
    const uint8_t *m_pBuffer;
    const int64_t  m_size;
          int64_t  m_offset;
    
};

};


#endif //_STREAM_READER_H_
