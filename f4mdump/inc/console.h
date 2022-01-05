#include <cstring>
#include <string>
#include "SimpleDataTypes.h"
#include "SimpleDataTypes.h"

class IConsoleBuffer
{
public:
    virtual void append(const uint8_t *buffer, const uint32_t &size) = 0;
    virtual void clear() = 0;
};

class CStrConsoleBuffer : public IConsoleBuffer
{
public:
    void append(const uint8_t *buffer, const uint32_t &size)
    {
        if(buffer)
        {
            m_data += std::string(reinterpret_cast<const char*>(buffer), size);
        }
        else
        {
            throw "CStrConsoleBuffer::append NULL pointer";
        }
    }
    void clear() 
    {
        m_data = "";
    }
    std::string m_data;
};

class CRawConsoleBuffer : public IConsoleBuffer
{
public:
    void append(const uint8_t *buffer, const uint32_t &size)
    {
        if(buffer)
        {
            m_data.insert( m_data.end(), buffer, buffer+size );
        }
        else
        {
            throw "CRawConsoleBuffer::append NULL pointer";
        }
    }
    void clear() 
    {
        m_data.clear();
    }
    ByteBuffer_t m_data;
};



class ConsoleAppContainer
{
    public:
        static ConsoleAppContainer& getInstance()
        {
            static ConsoleAppContainer instance;
            return instance;
        }

        virtual ~ConsoleAppContainer();
        
        void terminate();
        
        int execute(const std::string &cmd, IConsoleBuffer &inData, IConsoleBuffer &errData);
        
    private:
        ConsoleAppContainer();
        ConsoleAppContainer(ConsoleAppContainer const&);
        void operator=(ConsoleAppContainer const&);
    
        bool m_bTerm;
        
        bool readData(const int fd, IConsoleBuffer &outBuffer);
};