#ifndef _RTMP_WRAPPER_H_
#define _RTMP_WRAPPER_H_

#include <map>
#include <list>
#include <string>
#include <memory>
#include "RTMPWrapper.h"

extern "C"
{
#include <librtmp/rtmp.h>
#include <librtmp/amf.h>
#include <librtmp/log.h>
}

namespace rtmp
{

enum RTMPObjectType
{
    RTMP_NUMBER_TYPE = 0,
    RTMP_BOOLEAN_TYPE,
    RTMP_STRING_TYPE,
    RTMP_LIST_TYPE,
    RTMP_NULL_TYPE,
    RTMP_INTEGER_TYPE,
    RTMP_OBJECTEND_TYPE
};

class RTMPItem;
typedef std::list< std::shared_ptr<RTMPItem> > RTMPItems;

class RTMPItem
{
public:
    RTMPItem(const std::string &name)
    : m_name(name)
    {
    }
    virtual ~RTMPItem()
    {
    }
    virtual RTMPObjectType getType() = 0;
    virtual std::string getName()
    {
        return m_name;
    }
protected:
    std::string m_name;
    
};

class RTMPInteger : public RTMPItem
{
public:
    RTMPInteger(const std::string &name, const int32_t value)
    : RTMPItem(name)
    , m_value(value)
    {  }
    
    virtual ~RTMPInteger()
    {
        //printDBG("->| destroy[%d] [%s]\n", getType(), getName().c_str());
    }
    
    int32_t getValue()
    {
        return m_value;
    }
    
    virtual RTMPObjectType getType()
    {
        return RTMP_INTEGER_TYPE;
    }
    
private:
    int32_t m_value;
};

class RTMPNull : public RTMPItem
{
public:
    RTMPNull(const std::string &name)
    : RTMPItem(name)
    {
    }
    
    virtual ~RTMPNull()
    {
        //printDBG("->| destroy[%d] [%s]\n", getType(), getName().c_str());
    }
    
    void getValue()
    {
    }
    
    virtual RTMPObjectType getType()
    {
        return RTMP_NULL_TYPE;
    }
}; 

class RTMPObjEnd : public RTMPItem
{
public:
    RTMPObjEnd(const std::string &name)
    : RTMPItem(name)
    {
    }
    
    virtual ~RTMPObjEnd()
    {
        //printDBG("->| destroy[%d] [%s]\n", getType(), getName().c_str());
    }
    
    void getValue()
    {
    }
    
    virtual RTMPObjectType getType()
    {
        return RTMP_OBJECTEND_TYPE;
    }
}; 


class RTMPBool : public RTMPItem
{
public:
    RTMPBool(const std::string &name, const bool value)
    : RTMPItem(name)
    , m_value(value)
    {
    }
    
    virtual ~RTMPBool()
    { 
        //printDBG("->| destroy[%d] [%s]\n", getType(), getName().c_str());
    }
    
    bool getValue()
    {
        return m_value;
    }
    
    virtual RTMPObjectType getType()
    {
        return RTMP_BOOLEAN_TYPE;
    }
    
private:
    bool m_value;
};

class RTMPNumber : public RTMPItem
{
public:
    RTMPNumber(const std::string &name, const double value)
    : RTMPItem(name)
    , m_value(value)
    {  }
    
    virtual ~RTMPNumber()
    {
        //printDBG("->| destroy[%d] [%s]\n", getType(), getName().c_str());
    }
    
    double getValue()
    {
        return m_value;
    }
    
    virtual RTMPObjectType getType()
    {
        return RTMP_NUMBER_TYPE;
    }
    
private:
    double m_value;
};

class RTMPString : public RTMPItem
{
public:
    RTMPString(const std::string &name, const std::string &value)
    : RTMPItem(name)
    , m_value(value)
    { }
    
    virtual ~RTMPString()
    {
        //printDBG("->| destroy[%d] [%s]\n", getType(), getName().c_str());
    }
    
    std::string getValue()
    {
        return m_value;
    }
    
    virtual RTMPObjectType getType()
    {
        return RTMP_STRING_TYPE;
    }
    
private:
    std::string m_value;
};


class RTMPList : public RTMPItem
{
public:
    RTMPList(const std::string &name)
    : RTMPItem(name)
    { }
    
    virtual ~RTMPList()
    {
        //printDBG("->| destroy[%d] [%s]\n", getType(), getName().c_str());
    }
    
    virtual RTMPObjectType getType()
    {
        return RTMP_LIST_TYPE;
    }
    
    RTMPItems& getValue()
    {
        return m_items;
    }

    virtual void append(std::shared_ptr<RTMPItem> item)
    {
        if(0 != item.get())
        {
            m_items.push_back(item);
        }
    }
    
    virtual std::shared_ptr<RTMPItem> operator[](const std::string &name)
    {
        std::shared_ptr<RTMPItem> obj;
        for(RTMPItems::iterator it = m_items.begin(); it != m_items.end(); ++it)
        {
            if((*it)->getName() == name)
            {
                obj = (*it);
                break;
            }
        }
        return obj;
    }
    
    virtual std::shared_ptr<RTMPItem> operator[](const uint32_t &idx)
    {
        std::shared_ptr<RTMPItem> obj;
        uint32_t i = 0;
        for(RTMPItems::iterator it = m_items.begin(); it != m_items.end(); ++it)
        {
            if(idx == i)
            {
                obj = (*it);
                break;
            }
            ++i;
        }
        return obj;
    }
private:
    RTMPItems m_items;
};

bool GetStringItem(std::shared_ptr<RTMPItem> valueItem, std::string &value);
bool GetStringItem(std::shared_ptr<RTMPItem> rtmpItem, const std::string &name, std::string &value);
bool GetNumberItem(std::shared_ptr<RTMPItem> valueItem, double &value);
bool GetNumberItem(std::shared_ptr<RTMPItem> rtmpItem, const std::string &name, double &value);
bool GetBoolItem(std::shared_ptr<RTMPItem> valueItem, bool &value);
bool GetBoolItem(std::shared_ptr<RTMPItem> rtmpItem, const std::string &name, bool &value);
bool GetListItem(std::shared_ptr<RTMPItem> valueItem, RTMPItems *&value);
bool GetListItem(std::shared_ptr<RTMPItem> rtmpItem, const std::string &name, RTMPItems *&value);

bool GetIntegerItem(std::shared_ptr<RTMPItem> valueItem, int32_t &value);
bool GetIntegerItem(std::shared_ptr<RTMPItem> rtmpItem, const std::string &name, int32_t &value);

typedef std::pair< std::string, std::string>    RTMPOption_t;       //first key, second value
typedef std::list< RTMPOption_t >               RTMPOptionsList_t;   //first key, second value

typedef std::map< double, uint8_t >  InvokeResults_t;

class CRTMP
{
public:
    explicit CRTMP(const std::string &rtmpUrl, const RTMPOptionsList_t &rtmpParams = RTMPOptionsList_t() );
    
    virtual ~CRTMP();

    virtual void connect();
    
    virtual void close();
    
    virtual void terminate();

    virtual void set_options(const RTMPOptionsList_t &rtmpOprions);
    
    virtual void set_option(const RTMPOption_t &option);
    
    virtual std::shared_ptr<RTMPList> handleServerInvoke(const std::string &strMethod, const uint32_t timeout=static_cast<uint32_t>(-1));
    
    virtual bool remotePlayingMethod(const bool bPlayparm, const std::string &m_mediaId);
    
    bool isConnected();
private:
    void set_option(const std::string &key, const std::string &value);
    bool read_packet(RTMPPacket &rtmpPacket);
    bool send_packet(RTMPPacket &rtmpPacket, const bool block=true);

    void startInternalTimeout(const uint32_t timeout);
    bool isInternalTimeout();

    uint32_t m_internalTimeout;
    uint32_t m_startInternalTimeout;
    bool m_bInternalTimeout;
    
    bool m_bTerminate;
    RTMP *m_rtmp;
    std::string m_rtmpUrl;
    RTMPOptionsList_t m_rtmpParams;
    InvokeResults_t m_invoke_results;
    uint32_t m_numInvokes;
    
};

}

#endif // _RTMP_WRAPPER_H_