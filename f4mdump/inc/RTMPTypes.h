#ifndef _RTMP_TYPES_H_
#define _RTMP_TYPES_H_

#include <map>
#include <list>
#include <string>
#include <memory>
#include "RTMPWrapper.h"
#include "SimpleDataTypes.h"
#include "StreamReader.h"

extern "C"
{
#include <librtmp/rtmp.h>
#include <librtmp/amf.h>
#include <librtmp/log.h>
}

namespace rtmp
{

enum RTMPScriptDataTypes
{
    SCRIPT_DATA_TYPE_NUMBER      =0x00, 
    SCRIPT_DATA_TYPE_BOOLEAN     =0x01,
    SCRIPT_DATA_TYPE_STRING      =0x02, 
    SCRIPT_DATA_TYPE_OBJECT      =0x03,
    SCRIPT_DATA_TYPE_RESERVED    =0x04, 
    SCRIPT_DATA_TYPE_NULL        =0x05,
    SCRIPT_DATA_TYPE_UNDEFINED   =0x06,
    SCRIPT_DATA_TYPE_REFERENCE   =0x07,
    SCRIPT_DATA_TYPE_ECMAARRAY   =0x08, 
    SCRIPT_DATA_TYPE_OBJECTEND   =0x09,
    SCRIPT_DATA_TYPE_STRICTARRAY =0x0A, 
    SCRIPT_DATA_TYPE_DATE        =0x0B,
    SCRIPT_DATA_TYPE_LONGSTRING  =0x0C,
    
    SCRIPT_DATA_TYPE_AMF3        =0x11,
};

enum RTMPAMF3Types
{
    AMF3_TYPE_UNDEFINED     =0x00, 
    AMF3_TYPE_NULL          =0x01, 
    AMF3_TYPE_FALSE         =0x02, 
    AMF3_TYPE_TRUE          =0x03,
    AMF3_TYPE_INTEGER       =0x04, 
    AMF3_TYPE_DOUBLE        =0x05, 
    AMF3_TYPE_STRING        =0x06, 
    AMF3_TYPE_XML_DOC       =0x07,
    AMF3_TYPE_DATE          =0x08, 
    AMF3_TYPE_ARRAY         =0x09, 
    AMF3_TYPE_OBJECT        =0x0A, 
    AMF3_TYPE_XML           =0x0B,
    AMF3_TYPE_BYTE_ARRAY    =0x0C, 
    AMF3_TYPE_VECTOR_INT    =0x0D, 
    AMF3_TYPE_VECTOR_UINT   =0x0E,
    AMF3_TYPE_VECTOR_DOUBLE =0x0F, 
    AMF3_TYPE_VECTOR_OBJECT =0x10, 
    AMF3_TYPE_DICT          =0x11,
};

enum RTMPValues
{
    AMF3_EMPTY_STRING         = 0x01,
    AMF3_DYNAMIC_OBJECT       = 0x0b,
    AMF3_CLOSE_DYNAMIC_OBJECT = 0x01,
    AMF3_CLOSE_DYNAMIC_ARRAY  = 0x01,
    AMF3_MIN_INTEGER          = -268435456,
    AMF3_MAX_INTEGER          = 268435455,
};

struct CRTMPObjectTraits
{
    explicit CRTMPObjectTraits(bool dynamic=false)
    :m_dynamic(dynamic)
    {
    }
    
    std::string m_className;
    std::list< std::string > m_membersNames;
    bool m_dynamic;
};


typedef std::vector< std::shared_ptr<RTMPList> > AMF3ObjCache_t;

typedef std::vector< std::string > AMF3StrCache_t;

typedef std::vector< CRTMPObjectTraits > AMF3TraitsCache_t;

typedef std::map< std::string, CRTMPObjectTraits > AMF3TraitsLookup_t;



class CRTMPAMFDecoder
{
public:
    CRTMPAMFDecoder( );
    
    virtual ~CRTMPAMFDecoder();

    virtual std::shared_ptr<RTMPList> parse(RTMPPacket &rtmpPacket);

private:
    std::shared_ptr<RTMPItem> parseAMF3(const std::string& name="");
    int32_t parseAMF3Integer();
    std::string parseAMF3String();
    std::shared_ptr<RTMPList> parseAMF3Object(const std::string &name);
    std::shared_ptr<RTMPList> parseAMF3Array(const std::string &name);
    
    std::shared_ptr<RTMPItem> parseAMF(const std::string& name="");
    
    std::shared_ptr<RTMPList> parseScriptDataObject(const std::string &name);
    std::shared_ptr<RTMPList> parseScriptDataStrictArray(const std::string &name);
    std::shared_ptr<RTMPList> parseScriptDataECMAArray(const std::string &name);
    
    iptv::CBufferReader *m_reader;
    
    AMF3StrCache_t m_strCache;
    AMF3ObjCache_t m_objCache;
    AMF3TraitsCache_t m_traitsCache;
    
    AMF3TraitsLookup_t m_traitsLookup;
};

}

#endif // _RTMP_TYPES_H_