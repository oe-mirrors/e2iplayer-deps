#include "StreamReader.h"
#include "debug.h"

namespace iptv
{

uint8_t IStreamReader::readUInt8()
{
    assert(1 == sizeof(uint8_t));
    uint8_t value = 0;
    if( 1 == read(&value, 1) )
    {
        return value;
    }
    else
    {
        throw "ReadByte WRONG BUFFER SIZE";
    }
    return 0;
}

uint16_t IStreamReader::readUInt16()
{
    assert(2 == sizeof(uint16_t));
    uint8_t  buff[2] = {0};
    uint16_t value = 0;
    uint8_t *ptr = reinterpret_cast<uint8_t*>(&value);
    if( sizeof(buff) == read(buff, sizeof(buff)) )
    {
        ptr[1] = buff[0];
        ptr[0] = buff[1];
        return value;
    }
    else
    {
        throw "ReadInt24 WRONG BUFFER SIZE";
    }
    return 0;
}

uint32_t IStreamReader::readUInt24()
{
    uint8_t  buff[3] = {0};
    uint32_t value = 0;
    uint8_t *ptr = reinterpret_cast<uint8_t*>(&value);
    if( sizeof(buff) == read(buff, sizeof(buff)) )
    {
        ptr[2] = buff[0];
        ptr[1] = buff[1];
        ptr[0] = buff[2];
        return value;
    }
    else
    {
        throw "ReadInt24 WRONG BUFFER SIZE";
    }
    return 0;
}


uint32_t IStreamReader::readUInt32()
{
    assert(4 == sizeof(uint32_t));
    uint8_t  buff[4] = {0};
    uint32_t value = 0;
    uint8_t *ptr = reinterpret_cast<uint8_t*>(&value);
    if( sizeof(buff) == read(buff, sizeof(buff)) )
    {
        ptr[3] = buff[0];
        ptr[2] = buff[1];
        ptr[1] = buff[2];
        ptr[0] = buff[3];
        return value;
    }
    else
    {
        throw "ReadInt32 WRONG BUFFER SIZE";
    }
    return 0;
}

uint64_t IStreamReader::readUInt64()
{
    assert(8 == sizeof(uint64_t));
    uint8_t  buff[8] = {0};
    uint64_t value = 0;
    uint8_t *ptr = reinterpret_cast<uint8_t*>(&value);
    if( sizeof(buff) == read(buff, sizeof(buff)) )
    {
        ptr[7] = buff[0];
        ptr[6] = buff[1];
        ptr[5] = buff[2];
        ptr[4] = buff[3];
        ptr[3] = buff[4];
        ptr[2] = buff[5];
        ptr[1] = buff[6];
        ptr[0] = buff[7];
        return value;
    }
    else
    {
        throw "ReadInt64 WRONG BUFFER SIZE";
    }
    return 0;
}

double IStreamReader::readDouble()
{
    assert(8 == sizeof(double));
    uint8_t  buff[8] = {0};
    double value = 0;
    uint8_t *ptr = reinterpret_cast<uint8_t*>(&value);
    if( sizeof(buff) == read(buff, sizeof(buff)) )
    {
        ptr[7] = buff[0];
        ptr[6] = buff[1];
        ptr[5] = buff[2];
        ptr[4] = buff[3];
        ptr[3] = buff[4];
        ptr[2] = buff[5];
        ptr[1] = buff[6];
        ptr[0] = buff[7];
        return value;
    }
    else
    {
        throw "readDouble WRONG BUFFER SIZE";
    }
    return 0;
}

std::string IStreamReader::readString(const uint32_t len)
{
    std::vector<char> tmp;
    tmp.resize(len);
    if(len != read(&tmp[0], len))
    {   
        throw "readString WRONG BUFFER SIZE";
    }
    return std::string(&tmp[0], tmp.size());
}

std::string IStreamReader::readString()
{
    std::string value = "";
    char tmp = '\0';
    while(true)
    {
        if(1 == read(&tmp, 1))
        {   
            if('\0' != tmp)
            {
                value += tmp;
            }
            else
            {
                break;
            }
        }
        else
        {
            throw "readString WRONG BUFFER SIZE";
        }
    }
    return value;
}

CBufferReader::CBufferReader(const ByteBuffer_t &buffer)
: m_offset(0)
, m_pBuffer(&buffer[0])
, m_size(buffer.size())
{
}

CBufferReader::CBufferReader(const uint8_t *pBuffer, const uint32_t size)
: m_offset(0)
, m_pBuffer(pBuffer)
, m_size(size)
{
    ASSERT(m_pBuffer);
    if(!m_pBuffer)
    {
        throw "CBufferReader pBuffer cannot be NULL";
    }
}

CBufferReader::~CBufferReader()
{
}

int64_t CBufferReader::read(void *buf, uint32_t count)
{
    uint32_t readSize = 0;
    while(count > readSize && m_offset < m_size)
    {
        reinterpret_cast<uint8_t*>(buf)[readSize] = m_pBuffer[m_offset];
        ++m_offset;
        ++readSize;
    }
    return readSize;
}

int64_t CBufferReader::seek(int32_t offset, EOffsetwhence whence)
{
    int64_t ret = -1;
    switch(whence)
    {
    case READER_SEEK_SET:
    {
        if(0 <= offset && offset <= m_size)
        {
            m_offset = offset;
            ret = m_offset;
        }
        break;
    }
    case READER_SEEK_CUR:
    {
        int64_t newOffset = m_offset + offset;
        if(newOffset <= m_size && newOffset >= 0)
        {
            m_offset = newOffset;
            ret = newOffset;
        }
        break;
    }
    case READER_SEEK_END:
    {
        int64_t newOffset = m_size + m_offset;
        if(newOffset <= m_size && newOffset >= 0)
        {
            m_offset = newOffset;
            ret = newOffset;
        }
        break;
    }
    default:
        break;
    }
    return ret;
}

int64_t CBufferReader::size()
{
    return m_size;
}

int64_t CBufferReader::offset()
{
    return m_offset;
}

}