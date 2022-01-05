#ifndef _F4M_DOWNLOADER_H_
#define _F4M_DOWNLOADER_H_

#include <list>
#include <string>
#include "SimpleDataTypes.h"
#include "F4mProcessor.h"

namespace f4m
{

class CF4mDownloader
{
public:
    CF4mDownloader();
    
    virtual ~CF4mDownloader();
    
    void initialize(const std::string &mainUrl, const std::string &wgetCMD);
    
    virtual bool canHandleUrl(const std::string &url);
    
    virtual bool reportStreamsInfo(std::string &streamInfo);

    virtual void download( const std::string &baseWgetCmd, const std::string &manifestUrl, 
                   const std::string &outFile, const std::string &tmpFile, const int32_t bitrate );

    virtual void downloadWithoutTmpFile( const std::string &baseWgetCmd, const std::string &manifestUrl, 
                                 const std::string &outFile, const int32_t bitrate );
    virtual void terminate();
    
protected:
    void writeUInt24(FILE *pOutFile, const uint32_t &value);
    void writeFlvFileHeader(FILE *pOutFile, const ByteBuffer_t &metadata, const uint8_t audio=1, const uint8_t video=1);
    uint32_t getSegmentNum(const F4VBootstrapInfoBox &iBox, const uint32_t &iCurrentFragment);
    void updateBootstrapInfo(const F4VBootstrapInfoBox &iBox, uint32_t &oCurrentFragment, uint32_t &oLastFragment, bool &isEndPresentationDetected);

    static const uint32_t TAG_HEADER_LEN;
    static const uint32_t PREV_TAG_SIZE;
    static const uint8_t  AUDIO;
    static const uint8_t  VIDEO;

    static const uint8_t FRAME_TYPE_INFO;
    static const uint8_t CODEC_ID_AVC;
    static const uint8_t CODEC_ID_AAC;
    static const uint8_t AVC_SEQUENCE_HEADER;
    static const uint8_t AAC_SEQUENCE_HEADER;
    static const uint8_t AVC_SEQUENCE_END;

    static const uint32_t MAX_RETRIES;
    static const uint32_t WGET_TIMEOUT;
    
    bool m_bTerminate;
    
    std::string m_wgetCmd;
    std::string m_mainUrl;
};

}

#endif // _F4M_DOWNLOADER_H_