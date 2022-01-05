#include <cstdlib>
#include <cstring>
#include <ctime>
#include "debug.h"
#include "RTMPTypes.h"
#include "StreamReader.h"

namespace rtmp
{

using namespace std;
using namespace iptv;

    CRTMPAMFDecoder::CRTMPAMFDecoder()
    :m_reader(0)
    {
    
    }
    
    CRTMPAMFDecoder::~CRTMPAMFDecoder()
    {
        if(m_reader)
        {
            delete m_reader;
        }
    }
    
    //////////////////////////////////////////////////////////////////////////
    // parse
    //////////////////////////////////////////////////////////////////////////
    std::shared_ptr<RTMPList> CRTMPAMFDecoder::parse(RTMPPacket &rtmpPacket)
    {
        std::shared_ptr<RTMPList> objList;
        
        if(!rtmpPacket.m_body || 0 >= rtmpPacket.m_nBodySize)
        {
            return objList;
        }
        printDBG(":  CRTMPAMFDecoder::parse m_nBodySize[%lu]\n", (unsigned long)rtmpPacket.m_nBodySize);
        
        m_reader = new CBufferReader(reinterpret_cast<uint8_t*>(rtmpPacket.m_body), rtmpPacket.m_nBodySize);
        ASSERT(m_reader);
        while(m_reader->size() > m_reader->offset())
        {
            std::shared_ptr<RTMPItem> item = parseAMF();
            /*
            if(!item)
            {
                break;
            }
            */

            if(0 == objList.get() && 0 != item.get())
            {
                objList = std::make_shared<RTMPList>("");
            }
            
            if(0 != objList.get() && 0 != item.get())
            {
                objList->append(item);
            }
        }
        return objList;
    }
    
    std::shared_ptr<RTMPItem> CRTMPAMFDecoder::parseAMF(const std::string& name)
    {
        std::shared_ptr<RTMPItem> item;
        
        uint8_t type = m_reader->readUInt8();
        printDBG(":  CRTMPAMFDecoder::parse type[0x%02x]\n", (unsigned)type);
        switch(type)
        {
        case SCRIPT_DATA_TYPE_NUMBER:
        {
            // we catch here exception as temporary workaround
            try
            {
                item = std::make_shared<RTMPNumber>(name, m_reader->readDouble() );
            }
            catch(...)
            {
                item = std::make_shared<RTMPNumber>(name, 0.0);
            }
            printDBG(":  SCRIPT_DATA_TYPE_NUMBER[%lf]\n",  std::static_pointer_cast< RTMPNumber >(item)->getValue());
        break;
        }
        case SCRIPT_DATA_TYPE_BOOLEAN:
        {
            item = std::make_shared<RTMPBool>(name, m_reader->readUInt8() );
            printDBG(":  SCRIPT_DATA_TYPE_BOOLEAN[%u]\n",  (unsigned) std::static_pointer_cast< RTMPBool >(item)->getValue());
        break;
        }
        case SCRIPT_DATA_TYPE_STRING:
        {
            item = std::make_shared<RTMPString>(name, m_reader->readString(m_reader->readUInt16()) );
            printDBG(":  SCRIPT_DATA_TYPE_STRING[%s]\n",  std::static_pointer_cast< RTMPString >(item)->getValue().c_str());
        break;
        }
        case SCRIPT_DATA_TYPE_AMF3:
        {
            item = parseAMF3(name);
            printDBG(":  AMF3\n");
        break;
        }
        case SCRIPT_DATA_TYPE_REFERENCE:
        {
            uint16_t offset = m_reader->readUInt16();
            printDBG(":  SCRIPT_DATA_TYPE_REFERENCE\n");
        break;
        }
        case SCRIPT_DATA_TYPE_DATE:
        {
            double timestamp = m_reader->readDouble();
            int16_t offset = static_cast<int16_t>(m_reader->readUInt16());
            printDBG(":  SCRIPT_DATA_TYPE_DATE\n");
        break;
        }
        case SCRIPT_DATA_TYPE_LONGSTRING:
        {
            item = std::make_shared<RTMPString>(name, m_reader->readString(m_reader->readUInt32()) );
            printDBG(":  SCRIPT_DATA_TYPE_LONGSTRING[%s]\n",  std::static_pointer_cast< RTMPString >(item)->getValue().c_str());
        break;
        }
        case SCRIPT_DATA_TYPE_OBJECT:
        {
            printDBG(":  SCRIPT_DATA_TYPE_OBJECT name[%s]\n", name.c_str());
            item = parseScriptDataObject(name);
        break;
        }
        case SCRIPT_DATA_TYPE_ECMAARRAY:
        {
            printDBG(":  SCRIPT_DATA_TYPE_ECMAARRAY name[%s]\n", name.c_str());
            item = parseScriptDataECMAArray(name);
        break;
        }
        case SCRIPT_DATA_TYPE_OBJECTEND:
        {
            printDBG(":  SCRIPT_DATA_TYPE_OBJECTEND name[%s]\n", name.c_str());
            item = std::make_shared<RTMPObjEnd>("");
        break;
        }
        case SCRIPT_DATA_TYPE_STRICTARRAY:
        {
            printDBG(":  SCRIPT_DATA_TYPE_STRICTARRAY name[%s]\n", name.c_str());
            item = parseScriptDataStrictArray(name);
        break;
        }
        case SCRIPT_DATA_TYPE_NULL:
        {
            printDBG(":  NULL\n");
            item = std::make_shared<RTMPNull>("");
        break;
        }
        case SCRIPT_DATA_TYPE_UNDEFINED:
        {
            printDBG(":  UNDEFINED\n");
            item = std::make_shared<RTMPNull>("");
        break;
        }
        default:
        {
            printDBG(":  CRTMPAMFDecoder::parse unknown type[0x%02x] -> return\n", (unsigned)type);
            break;
        }
        };
        return item;
    }
    
    std::shared_ptr<RTMPList> CRTMPAMFDecoder::parseScriptDataObject(const std::string &name)
    {
        std::shared_ptr<RTMPList> obj = std::make_shared<RTMPList>(name);
        
        while(true)
        {
            std::string key = m_reader->readString(m_reader->readUInt16());
            
            std::shared_ptr<RTMPItem> item = parseAMF(key);
            if(!item || RTMP_OBJECTEND_TYPE == item->getType())
            {
                break;
            }
            obj->append(item);
        }
        return obj;
    }
    
    std::shared_ptr<RTMPList> CRTMPAMFDecoder::parseScriptDataStrictArray(const std::string &name)
    {
        std::shared_ptr<RTMPList> obj = std::make_shared<RTMPList>(name);
        uint32_t length = m_reader->readUInt32();

        for(uint32_t i=0; i<length; ++i)
        {
            std::shared_ptr<RTMPItem> item = parseAMF("");
            /*
            if(!item)
            {
                break;
            }
            */
            obj->append(item);
        }
            
        return obj;
    }
    
    std::shared_ptr<RTMPList> CRTMPAMFDecoder::parseScriptDataECMAArray(const std::string &name)
    {
        std::string key;
        std::shared_ptr<RTMPList> obj = std::make_shared<RTMPList>(name);
        uint32_t length = m_reader->readUInt32();

        for(uint32_t i=0; i<length; ++i)
        {
            key = m_reader->readString(m_reader->readUInt16());
            std::shared_ptr<RTMPItem> item = parseAMF(key);
            /*
            if(!item)
            {
                break;
            }
            */
            obj->append(item);
        }
            
        return obj;
    }
    
    std::shared_ptr<RTMPItem> CRTMPAMFDecoder::parseAMF3(const std::string& name)
    {
        std::shared_ptr<RTMPItem> item;
        
        uint8_t type = m_reader->readUInt8();
        printDBG(":  CRTMPAMFDecoder::parseAMF3 name[%s] type[0x%02x]\n", name.c_str(), (unsigned)type);
        switch(type)
        {
        case AMF3_TYPE_UNDEFINED:
        {
            printDBG(":  UNDEFINED\n");
            item = std::make_shared<RTMPNull>(name);
        break;
        }
        case AMF3_TYPE_NULL:
        {
            printDBG(":  NULL\n");
            item = std::make_shared<RTMPNull>(name);
        break;
        }
        
        case AMF3_TYPE_FALSE:
        {
            item = std::make_shared<RTMPBool>(name, false );
            printDBG(":  FALSE\n");
        break;
        }
        case AMF3_TYPE_TRUE:
        {
            item = std::make_shared<RTMPBool>(name, true );
            printDBG(":  TRUE\n");
        break;
        }
        case AMF3_TYPE_INTEGER:
        {
            item = std::make_shared<RTMPInteger>(name, parseAMF3Integer());
            printDBG(":  INTEGER[%d]\n", std::static_pointer_cast< RTMPInteger >(item)->getValue());
        break;
        }
        case AMF3_TYPE_DOUBLE:
        {
            item = std::make_shared<RTMPNumber>(name, m_reader->readDouble() );
            printDBG(":  DOUBLE[%lf]\n",  std::static_pointer_cast< RTMPNumber >(item)->getValue());
        break;
        }
        case AMF3_TYPE_STRING:
        {
            item = std::make_shared<RTMPString>(name, parseAMF3String());
            printDBG(":  STRING[%s]\n", std::static_pointer_cast< RTMPString >(item)->getValue().c_str());
        break;
        }
        case AMF3_TYPE_ARRAY:
        {
            item = parseAMF3Array(name);
        break;
        }
        case AMF3_TYPE_OBJECT:
        {
            item = parseAMF3Object(name);
        break;
        }
        default:
        {
            printDBG(":  CRTMPAMFDecoder::parseAMF3 unknown type[0x%02x] -> return\n", (unsigned)type);
            break;
        }
        };
        return item;
    }
    
    int32_t CRTMPAMFDecoder::parseAMF3Integer()
    {
        int32_t rval = 0;
        uint8_t byte_count = 0;
        uint8_t byte = m_reader->readUInt8();
        
        while( (byte & 0x80) != 0 && byte_count < 3 )
        {
            rval <<= 7;
            rval |= byte & 0x7f;

            byte = m_reader->readUInt8();
            ++byte_count;
        }
    
        if( byte_count < 3 )
        {
            rval <<= 7;
            rval |= byte & 0x7F;
        }
        else
        {
            rval <<= 8;
            rval |= byte & 0xff;
        }

        if( (rval & 0x10000000) != 0)
        {
            rval -= 0x20000000;
        }
        
        return rval;
    }
    
    std::string CRTMPAMFDecoder::parseAMF3String()
    {
        std::string rval;
        int32_t header = parseAMF3Integer();
        if(0 == (header & 1))
        {
            uint32_t idx = static_cast<uint32_t>(header >> 1);
            if( idx>=m_strCache.size() )
            {
                throw "No such index in m_strCache";
            }
            return m_strCache[idx];
        }
        else
        {
            uint32_t size = static_cast<uint32_t>(header >> 1);
            rval = m_reader->readString(size);

            if(rval.size())
            {
                m_strCache.push_back(rval);
            }
        }
        return rval;
    }
    
    std::shared_ptr<RTMPList> CRTMPAMFDecoder::parseAMF3Object(const std::string &name)
    {
        int32_t header = parseAMF3Integer();
        std::shared_ptr<RTMPList> obj;
        
        if(0 == (header & 1))
        {
            uint32_t idx = static_cast<uint32_t>(header >> 1);
            if( idx>=m_objCache.size() )
            {
                throw "No such index in m_objCache";
            }
            return m_objCache[idx];
        }
        else
        {
            CRTMPObjectTraits traits;
            
            header >>= 1;
            if(0 == (header & 1))
            {
                uint32_t idx = static_cast<uint32_t>(header >> 1);
            if( idx>=m_traitsCache.size() )
            {
                throw "No such index in m_traitsCache";
            }
                traits = m_traitsCache[idx];
            }
            else
            {
                bool externalizable = (header & 2) != 0;
                bool dynamic = (header & 4) != 0;
                uint32_t members_len = static_cast<uint32_t>( header >> 3 );
                
                std::string class_name = parseAMF3String();
                printDBG("CRTMPAMFDecoder::parseAMF3Object class_name[%s] members_len[%u] externalizable[%u], dynamic[%u]\n", class_name.c_str(), members_len, (unsigned)externalizable, (unsigned)dynamic);

                std::list<std::string> members;
                std::string tmpStr;
                for(uint32_t i=0; i<members_len; ++i)
                {
                    tmpStr = parseAMF3String();
                    members.push_back(tmpStr);
                    printDBG("CRTMPAMFDecoder::parseAMF3Object member_name[%s] \n", tmpStr.c_str());
                }
                    
                if(class_name.empty())
                {
                    traits = CRTMPObjectTraits(true);
                }
                else if( m_traitsLookup.find(class_name) != m_traitsLookup.end())
                {
                    traits = m_traitsLookup[class_name];
                    m_traitsCache.push_back(traits);
                }
                else
                {
                    traits.m_className    = class_name;
                    traits.m_membersNames = members;
                    traits.m_dynamic      = dynamic;
                    m_traitsCache.push_back(traits);
                }
                
                if(traits.m_className.empty())
                {
                    class_name = name;
                }
                else
                {
                    class_name = traits.m_className;
                }
                
                obj = std::make_shared<RTMPList>(class_name);
                std::list< std::string >::iterator it = traits.m_membersNames.begin();
                for(; it != traits.m_membersNames.end(); ++it)
                {
                    obj->append(parseAMF3(*it));
                }
                
                if(traits.m_dynamic)
                {   
                    while(true)
                    {
                        tmpStr = parseAMF3String();
                        if(tmpStr.empty())
                        {
                            break;
                        }
                        obj->append(parseAMF3(tmpStr));
                    }
                }
            }
        }
        
        return obj;
    }
    
    
    std::shared_ptr<RTMPList> CRTMPAMFDecoder::parseAMF3Array(const std::string &name)
    {
        int32_t header = parseAMF3Integer();
        std::shared_ptr<RTMPList> obj;
        
        if(0 == (header & 1))
        {
            uint32_t idx = static_cast<uint32_t>(header >> 1);
            if( idx>=m_objCache.size() )
            {
                throw "No such index in m_objCache";
            }
            return m_objCache[idx];
        }
        else
        {
            obj = std::make_shared<RTMPList>(name);
            std::string key;
            while(true)
            {
                key = parseAMF3String();
                if(key.empty())
                {
                    break;
                }
                std::shared_ptr<RTMPItem> item = parseAMF3(key);
                /*
                if(!item)
                {
                    break;
                }
                */
                obj->append(item);
            }
            
            uint32_t elem_counts = static_cast<uint32_t>(header >> 1);
            for(uint32_t i=0; i<elem_counts; ++i)
            {
                std::shared_ptr<RTMPItem> item = parseAMF3("array_elem");
                /*
                if(!item)
                {
                    break;
                }
                */
                obj->append(item);
            }
        }

        return obj;
    }
} /* namespace rtmp*/


