#ifndef _F4M_MANIFEST_PARSER_H_
#define _F4M_MANIFEST_PARSER_H_

#include <string>
#include <vector>

#include "tinyxml2.h"
#include "SimpleFunctions.h"

/*************************************************************************************************
* namespace
**************************************************************************************************/
namespace f4m
{
struct MediaEntry
{
    MediaEntry() : bitrate(0) {};
    int32_t bitrate;
    std::string streamId;
    std::string baseUrl;
    std::string url;
    std::string bootstrapUrl;
    ByteBuffer_t bootstrap;
    ByteBuffer_t metadata;

    bool operator<(const MediaEntry &obj) const
    {
        return (bitrate < obj.bitrate) ? true : false;
    }
};

class CManifestParser
{
public:
    CManifestParser(const std::string &wgetCMD);
    ~CManifestParser();

    bool parseManifest(const std::string &mainUrl);

    bool getMedia(MediaEntry &media, const int32_t bitrate = 0) const;

    void refreshBootstrap(const std::string &bootstrapUrl, MediaEntry &mediaEntry);
    
    std::vector<int32_t> getAllBitrates()
    {
        std::vector<int32_t> allBitrates;
        for(uint32_t i=0; i < m_media.size(); ++i)
        {
            allBitrates.push_back( m_media[i].bitrate );
        }
        return allBitrates;
    }
    
private:
    bool getPage(const std::string &url, std::string &data);
    bool getManifest(const std::string &url, tinyxml2::XMLDocument &xmlDoc);
    bool addMediaEntries(const std::string &url,
                            tinyxml2::XMLDocument &xml,
                            const int32_t bitrate = 0
                           );

    std::vector<MediaEntry> m_media;

    const std::string m_sWgetCMD;
};

} /* namespace f4m*/

#endif //_F4M_MANIFEST_PARSER_H_
