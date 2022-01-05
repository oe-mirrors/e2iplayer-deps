#ifndef _UDS_DOWNLOADER_H_
#define _UDS_DOWNLOADER_H_

#include <string>

#include "RTMPWrapper.h"
#include "SimpleDataTypes.h"
#include "F4mProcessor.h"
#include "F4mDownloader.h"

namespace f4m
{
typedef std::pair<uint64_t, std::string> ChunksRangeItem_t;
typedef std::list< ChunksRangeItem_t > ChunksRangeList_t;

struct CStreamInfo
{
    CStreamInfo()
    : streamHeight(0.0)
    , chunkId(0)
    , chunkOffset(0)
    {}
    std::string streamName;
    std::string provUrl;
    std::string  provName;
    double streamHeight;
    uint64_t chunkId;
    uint64_t chunkOffset;
    std::string chunkName; //playpath for rtmp
    ChunksRangeList_t chunksRange; // chunkNum <-> chunkHash  
};

typedef std::list<CStreamInfo> StreamsInfoList_t;

/*
enum UsdPlayingState_e
{
    eUSD_STOPPED = 0, //#0 is stopped, sending bool false
    eUSD_WAITING = 1, //#1 is playing sent bool false, waiting for downloaded chunk
    eUSD_READY   = 2, //#2 is chunk downloaded, ready to send bool true
    eUSD_PLAING  = 3, //#3 is playing sent bool true
};
*/

class CUDSDownloader : public f4m::CF4mDownloader
{
public:
    CUDSDownloader();
    
    virtual ~CUDSDownloader();
    
    virtual void initialize(const std::string &url, const std::string &wgetCmd);
    
    virtual bool canHandleUrl(const std::string &url);
    
    virtual bool reportStreamsInfo(std::string &streamInfo, const uint32_t maxTries=10);

    virtual void downloadWithoutTmpFile( const std::string &baseWgetCmd, const std::string &outFile, const std::string &fragmentUrl );

    StreamsInfoList_t getStreamsInfo(const std::string &app="channel", const uint32_t maxTries=1);
        
protected:
    void updateInfo(const std::string &fragmentUrl, uint32_t &oCurrentFragment, uint32_t &oLastFragment, bool &isEndPresentationDetected, const uint32_t maxTries=1);
    
    std::string getFragmentHash(const uint32_t currentFragment);
    
    const std::string dumpStreamsInfoList(const StreamsInfoList_t &streamInfoList);
    
    const std::string dumpStreamsInfo(const CStreamInfo &streamInfo);
    
    static const std::string USTREAM_DOMAIN;
    static const std::string LIVE_SWF_URL;
    static const std::string EMBED_PAGE_URL;
    static const std::string RECORDED_URL;
    static const std::string RTMP_URL;
    static const uint32_t    MAX_RETRIES;
    
    StreamsInfoList_t m_lastStreamsInfo;
    
    std::string m_wgetCmd;
    std::string m_password;
    std::string m_mainUrl;
    std::string m_mediaId;
    bool m_bLive;
    bool m_bRecorded;
    
    ChunksRangeList_t m_chunksHashList;
    
    rtmp::CRTMP *m_pConn;
    uint32_t m_playingState; //#0 is stopped, sending bool false
                             //#1 is playing sent bool false, waiting for downloaded chunk
                             //#2 is chunk downloaded, ready to send bool true
                             //#3 is playing sent bool true
};

}

#endif // _UDS_DOWNLOADER_H_