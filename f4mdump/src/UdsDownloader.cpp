#include <cstdio>
#include <time.h>

#include <sstream>
#include <string>
#include <list> 
#include <algorithm>
#include <cstdio>
#include <cstdlib> //for randint
#include <sstream>

#include "console.h"
#include "debug.h"
#include "parser.h"

#include "SimpleFunctions.h"
#include "StringHelper.h"
#include "StreamReader.h"
#include "F4mProcessor.h"
#include "UdsDownloader.h"

extern bool terminated;

namespace f4m
{
using namespace std;
using namespace rtmp;
using namespace iptv;

/**************************************************************************************************
 * Constant and globals variables
 *************************************************************************************************/
const std::string CUDSDownloader::USTREAM_DOMAIN = "ustream.tv"; //"http://static-cdn1.ustream.tv/swf/live/viewer.rsl:505.swf"; //
const std::string CUDSDownloader::LIVE_SWF_URL   = "http://static-cdn1.ustream.tv/swf/live/viewer.qrsl:65.swf?rmalang=en_US";
const std::string CUDSDownloader::EMBED_PAGE_URL = "https://www.ustream.tv/embed/";
const std::string CUDSDownloader::RECORDED_URL   = "http://tcdn.ustream.tv/video/";
const std::string CUDSDownloader::RTMP_URL       = "rtmp://r%u-1-%s-channel-live.ums.ustream.tv:1935/ustream"; //"rtmp://r%u.1.%s.channel.live.ums.ustream.tv:80/ustream"; 
const uint32_t    CUDSDownloader::MAX_RETRIES    = 10; // maximum number of retry downloading fragment before given up

static uint32_t randint(const uint32_t a, const uint32_t b)
{
    static bool isInited = false;
    if(!isInited)
    {
        isInited = true;
        time_t t;
        srand((unsigned) time(&t));
    }
    
    if(b > a)
    {
        return a + (rand() % (b - a + 1));
    }
    else
    {
        // assert(false);
        return a;
    }
}
    
CUDSDownloader::CUDSDownloader()
: m_bLive(false)
, m_bRecorded(false)
, m_pConn(NULL)
, m_playingState(0)
{
    /* nothing to do here */
}

CUDSDownloader::~CUDSDownloader()
{
    /* nothing to do here */
}

void CUDSDownloader::initialize(const std::string &mainUrl, const std::string &wgetCmd)
{
    m_password = "";
    m_wgetCmd = wgetCmd;
    m_mainUrl = mainUrl;
    std::vector<std::string> tmpVect;
    CStringHelper::splitString(m_mainUrl+"/", tmpVect, '/', false);
    
    if(0 < tmpVect.size())
    {
        m_mediaId = tmpVect[tmpVect.size()-1];
    }
    
    if( std::string::npos != m_mainUrl.find("/embed/") || 
        std::string::npos != m_mainUrl.find("/channel/id/") )
    {
        m_bLive = true;
        m_bRecorded = false;
    }
    else if( std::string::npos != m_mainUrl.find("/recorded/") )
    {
        m_bLive = false;
        m_bRecorded = true;
    }
    else
    {
        m_bLive = false;
        m_bRecorded = false;
    }
    
    printDBG("CUDSDownloader::initialize m_mainUrl[%s], m_bLive[%d], m_bRecorded[%d] \n", m_mainUrl.c_str(), m_bLive, m_bRecorded);
}

bool CUDSDownloader::canHandleUrl(const std::string &url)
{
    return (std::string::npos != url.find(USTREAM_DOMAIN)) ? true : false;
}

bool CUDSDownloader::reportStreamsInfo(std::string &streamInfo, const uint32_t maxTries/*=10*/)
{
    if(m_bLive)
    {
        m_lastStreamsInfo = getStreamsInfo("channel", maxTries);
    }
    else if(m_bRecorded)
    {
        m_lastStreamsInfo = getStreamsInfo("recorded", maxTries);
    }
    streamInfo = dumpStreamsInfoList(m_lastStreamsInfo);
    return true;
}

void CUDSDownloader::updateInfo(const std::string &fragmentUrl, uint32_t &oCurrentFragment, uint32_t &oLastFragment, bool &isEndPresentationDetected, const uint32_t maxTries/*=1*/)
{
    ChunksRangeList_t hashItemsList;
    
    StreamsInfoList_t streamInfoList = getStreamsInfo("channel", maxTries);
    StreamsInfoList_t::iterator tmpIt = streamInfoList.begin();
    
    uint32_t chunkId = 0;
    while(tmpIt != streamInfoList.end())
    {
        if( fragmentUrl == (tmpIt->provUrl + tmpIt->chunkName) )
        {
            hashItemsList = tmpIt->chunksRange;
            chunkId = tmpIt->chunkId;
            m_chunksHashList.clear();
            break;
        }
        ++tmpIt;
    }
    
    ChunksRangeList_t::iterator newIt = hashItemsList.begin();
    while(newIt != hashItemsList.end())
    {
        m_chunksHashList.push_back(*newIt);
        ++newIt;
    }
    
    if(m_chunksHashList.size() && chunkId > 0)
    {
        m_chunksHashList.sort();
        m_chunksHashList.unique();
    
        uint32_t firstFragment = m_chunksHashList.front().first;
        uint32_t lastFragment  = m_chunksHashList.back().first;
    
        if(static_cast<uint32_t>(-1) == oCurrentFragment || oCurrentFragment < firstFragment)
        {
            oCurrentFragment = firstFragment;
        }
        
        if(oLastFragment < lastFragment)
        {
            oLastFragment = lastFragment; 
        }
        
        if(oCurrentFragment > oLastFragment)
        {
            oCurrentFragment = oLastFragment;
        }
    }
}

std::string CUDSDownloader::getFragmentHash(const uint32_t currentFragment)
{
    ChunksRangeItem_t currHashItem;
    ChunksRangeList_t::iterator it = m_chunksHashList.begin();
    while(it!=m_chunksHashList.end())
    {
        if(it->first <= currentFragment)
        {
        
            currHashItem = *it; 
            it = m_chunksHashList.erase(it); // to old so, we remove it.
        }
        else
        {
            break;
        }
    }
    if(0 < currHashItem.second.size())
    {
        m_chunksHashList.push_front(currHashItem); // hash for current fragment may be used for next one
    }
    
    return currHashItem.second;
}

void CUDSDownloader::downloadWithoutTmpFile( const std::string &baseWgetCmd, const std::string &outFile, const std::string &fragmentUrl )
{
    const bool skipBadPacket = true;
    if(!m_bTerminate)
    {
        std::string downloadUrlBase = fragmentUrl;
        CStringHelper::replace(downloadUrlBase, "https://", "http://");
        CStringHelper::replace(downloadUrlBase, "_%_", "_%u_");
        CStringHelper::replace(downloadUrlBase, "_%.", "_%s.");
        
        printDBG("downloadUrlBase [%s]\n", downloadUrlBase.c_str());
        
        FILE *pOutFile = fopen(outFile.c_str(), "wb");
        if(0 == pOutFile)
        {
            throw "CUDSDownloader::downloadWithoutTmpFile problem with create out file";
        }
        
        try
        {
            writeFlvFileHeader(pOutFile, ByteBuffer_t());
            
            uint64_t totalDownloadSize = 0;
            
            /* headers should be written only once */
            bool AAC_HeaderWritten = false;
            bool AVC_HeaderWritten = false;
            
            bool isLive = m_bLive;
            printDBG("isLive [%d]\n", isLive);
            bool isEndPresentationDetected = false; 
            bool forceUpdate = false;
            uint32_t currentFragment  = static_cast<uint32_t>(-1);
            uint32_t lastFragment     = 0;
            updateInfo(fragmentUrl, currentFragment, lastFragment, isEndPresentationDetected, 20);
            
            printDBG("currentFragment [%d] lastFragment[%d]\n", currentFragment, lastFragment);
            /*
            if(isLive && !isEndPresentationDetected)
            {
                currentFragment = (10 < lastFragment) ? lastFragment-10 : lastFragment;
            }
            */
            printDBG("currentFragment [%d] lastFragment[%d]\n", currentFragment, lastFragment);
            CRawConsoleBuffer inData;
            CStrConsoleBuffer errData;
            while(!m_bTerminate && currentFragment <= lastFragment)
            {
                // prepare cmd
                std::stringstream cmd;
                cmd << baseWgetCmd << " --tries=0 --timeout=" << WGET_TIMEOUT << " -O - " << '"' << StringFormat(downloadUrlBase.c_str(), currentFragment, getFragmentHash(currentFragment).c_str()) << '"';
                
                // download fragment
                uint32_t tries = 0;
                int32_t retval = -1;
                do
                {
                    inData.clear();
                    errData.clear();
                    retval = ConsoleAppContainer::getInstance().execute(cmd.str().c_str(), inData, errData);
                    printDBG("Downloading [%d] [%s]\n", currentFragment, getFragmentHash(currentFragment).c_str());
                    /* first, segments from the list can be no longer available 
                     * when we start downloading live stream, simple they will be skipped :) 
                     */
                    if(0 == retval || (0 == totalDownloadSize && isLive && !isEndPresentationDetected))
                    {
                        break;
                    }
                    ::sleep(1);
                    ++tries;
                } while(!m_bTerminate && tries < MAX_RETRIES);
                
                if(0 == retval)
                {
                    CBufferReader reader(inData.m_data);
                       
                    while(!m_bTerminate && reader.offset() < reader.size())
                    {
                        uint8_t packetType = reader.readUInt8();
                        uint32_t packetSize = reader.readUInt24();
                        uint32_t totalTagLen = TAG_HEADER_LEN + packetSize + PREV_TAG_SIZE;
                        reader.seek(-4, READER_SEEK_CUR);
                        
                        if((reader.offset() + totalTagLen) > reader.size())
                        {
                            std::string errorInfo = StringFormat("{\"error\":\"Bad packet size detected!\", \"packet_type\": \"0x%x\", \"fragment_size\":\"%lld\", \"offset\":\"%lld\", \"tag_len\":\"%lld\"", packetType, static_cast<int64_t>(reader.size()), static_cast<int64_t>(reader.offset()), static_cast<int64_t>(totalTagLen));
                           
                            // try to fix size of packet
                            uint32_t tmp = (reader.offset() + totalTagLen) - reader.size();
                            if( !skipBadPacket && tmp < packetSize)
                            {
                                packetSize -= tmp;
                                const uint8_t *ptr = reinterpret_cast<const uint8_t*>(&packetSize);
                                inData.m_data[reader.offset()+1] = ptr[2];
                                inData.m_data[reader.offset()+2] = ptr[1];
                                inData.m_data[reader.offset()+3] = ptr[0];
                                printInf("%s, \"action\":\"correcting packet\"}\n", errorInfo.c_str());
                            }
                            else
                            {
                                printInf("%s, \"action\":\"skipping packet\"}\n", errorInfo.c_str());
                                break;
                            }
                        }
                        
                        switch(packetType)
                        {
                            case 0x08: //CF4mDownloader::AUDIO:
                            case 0x09: //CF4mDownloader::VIDEO:
                            {
                                reader.seek(TAG_HEADER_LEN, READER_SEEK_CUR);
                                
                                /* init values for VIDEO tag */
                                uint8_t frameInfo    = reader.readUInt8();
                                uint8_t avPacketType = reader.readUInt8();
                                uint8_t codecID      = frameInfo & 0x0F;
                                
                                bool *HeaderWritten   = &AVC_HeaderWritten;
                                uint8_t cmpPacketType = AVC_SEQUENCE_HEADER;
                                uint8_t cmpCodecID    = CODEC_ID_AVC;
                                if(0x08 == packetType) //(CF4mDownloader::AUDIO == packetType)
                                {
                                    codecID = (frameInfo & 0xF0) >> 4;
                                    HeaderWritten = &AAC_HeaderWritten;
                                    cmpPacketType = AAC_SEQUENCE_HEADER;
                                    cmpCodecID    = CODEC_ID_AAC;
                                }
                                
                                reader.seek(-(TAG_HEADER_LEN+2), READER_SEEK_CUR);
                                
                                if(cmpCodecID == codecID)
                                {
                                    if(cmpPacketType == avPacketType)
                                    {
                                        if((*HeaderWritten))
                                        {
                                            printDBG("Skipping sequence header\n");
                                            break;
                                        }
                                        else
                                        {
                                            printDBG("Writing sequence header\n");
                                            *HeaderWritten = true;
                                        }
                                    }
                                    else if(!(*HeaderWritten))
                                    {
                                        printDBG("Discarding packet received before sequence header\n");
                                        break;
                                    }
                                }
                                
                                if(totalTagLen != fwrite(&inData.m_data[reader.offset()], sizeof(uint8_t), totalTagLen, pOutFile))
                                {
                                    throw "Writing AUDIO/VIDEO error!";
                                }
                                totalDownloadSize += totalTagLen;
                                break;
                            }
                            /*
                            case 0x12: // SCRIPT DATA
                            {
                                printDBG("SCRIPT DATA SIZE[%u] type[%x] nextType[%x] offset[%d]\n", totalTagLen, inData.m_data[0], inData.m_data[totalTagLen], (int) reader.offset());
                                if(totalTagLen != fwrite(&inData.m_data[reader.offset()], sizeof(uint8_t), totalTagLen, pOutFile))
                                {
                                    throw "Writing SCRIPT DATA error!";
                                }
                                break;
                            }
                            */
                            default:
                            {
                                printDBG("Packet type [%x] skipped\n", packetType);
                                break;
                            }
                        }
                        reader.seek(totalTagLen, READER_SEEK_CUR);
                    }
                }
                else
                {
                    if( !isLive || isEndPresentationDetected)
                    {
                        printDBG("Download problem cmd[%s] errData[%s]\n", cmd.str().c_str(), errData.m_data.c_str());
                        throw "";
                    }
                    else
                    {
                        forceUpdate = true;
                    }
                }
                
                if(totalDownloadSize > 0 && 0 == m_playingState)
                {
                    m_playingState = 1;
                    if(m_pConn)
                    {
                        m_pConn->remotePlayingMethod(true, m_mediaId);
                    }
                }
                // report progress
                printInf("{ \"total_download_size\":%llu }\n", totalDownloadSize);
                
                if(currentFragment < lastFragment)
                {
                    ++currentFragment;
                }
                
                if( forceUpdate || (isLive && !isEndPresentationDetected /*&&  2 > (lastFragment - currentFragment)*/))
                {
                    // wait for new fragment 
                    uint32_t newLastFragment = lastFragment;
                    do
                    {
                        try
                        {   
                            uint32_t maxTries = (forceUpdate || currentFragment == lastFragment) ? 5 : 1; 
                            updateInfo(fragmentUrl, currentFragment, newLastFragment, isEndPresentationDetected, maxTries);
                        }
                        catch(const char *err)
                        {
                            printExc("error [%s]\n", err);
                        }
                        
                        if(isEndPresentationDetected || newLastFragment > lastFragment)
                        {
                            lastFragment = newLastFragment;
                            printDBG("OK currentFragment[%d] isEndPresentationDetected[%d]\n", currentFragment, isEndPresentationDetected);
                            break;
                        }
                        else
                        {
                            ::sleep(1);
                        }
                        printDBG("retry [%d] [%d] \n", newLastFragment, currentFragment);
                    }
                    while(!m_bTerminate && currentFragment == lastFragment);
                }
            }
        }
        catch(const char *err)
        {
            fclose(pOutFile);
            throw err;
        }
        fclose(pOutFile);
    }
}

StreamsInfoList_t CUDSDownloader::getStreamsInfo(const std::string &app/*=channel*/, const uint32_t maxTries/*=1*/)
{
    StreamsInfoList_t streamInfoList;

    try
    {
        int32_t tries = 0;
        printDBG("%s %d\n", __FUNCTION__, __LINE__);
        while(0 == streamInfoList.size() && maxTries > tries && !terminated )
        {
            string swfUrl(LIVE_SWF_URL);
            string pageUrl(m_mainUrl);
            string rtmpUrl = StringFormat(RTMP_URL.c_str(), randint(0, 0xffffff), m_mediaId.c_str());
            
            printDBG("CUDSDownloader::getStreamsInfo rtmpUrl[%s]\n", rtmpUrl.c_str());
            
            RTMPOptionsList_t rtmpParams;
            rtmpParams.push_back(RTMPOption_t("pageUrl", pageUrl));
            rtmpParams.push_back(RTMPOption_t("swfUrl", swfUrl));
            rtmpParams.push_back(RTMPOption_t("conn", "O:1"));
            rtmpParams.push_back(RTMPOption_t("conn", string("NS:application:")+app));
            rtmpParams.push_back(RTMPOption_t("conn", string("NS:password:")+m_password));
            rtmpParams.push_back(RTMPOption_t("conn", string("NS:media:")+m_mediaId));
            rtmpParams.push_back(RTMPOption_t("conn", "O:0"));
            ++tries;
            try
            {
                if( NULL != m_pConn && (!m_pConn->isConnected() || tries > 2) )
                {
                    printDBG("%s, restart connection\n", __FUNCTION__);
                    delete m_pConn;
                    m_pConn = NULL;
                }
                
                if ( NULL == m_pConn )
                {
                    m_pConn = new CRTMP(rtmpUrl, rtmpParams);
                    m_pConn->connect();
                    m_playingState = 0;
                    m_pConn->remotePlayingMethod(false, m_mediaId);
                }
                
                std::shared_ptr<RTMPList> list = m_pConn->handleServerInvoke("moduleInfo", 10);
                bool bError = false;
                if(list.get())
                {
                    //printDBG("->|%s, %d size[%u] l[%s] l[%s] l[%s] l[%s]\n", __FUNCTION__, __LINE__, list->getValue().size(), (*list)[0]->getName().c_str(), (*list)[1]->getName().c_str(), (*list)[2]->getName().c_str(), (*list)[3]->getName().c_str());
                    RTMPItems *pStream = 0;
                    if(GetListItem(list, "stream",  pStream))
                    {
                        for(RTMPItems::iterator provider=pStream->begin(); !bError &&  provider!=pStream->end(); ++provider)
                        {
                            CStreamInfo infoItem;
                            
                            GetStringItem(*provider, "url", infoItem.provUrl);
                            if("" == infoItem.provUrl)
                            {
                                GetStringItem(*provider, "varnishUrl", infoItem.provUrl);
                            }
                            printDBG("%s %d stream provider [%s]\n", __FUNCTION__, __LINE__, infoItem.provUrl.c_str());
                            
                            GetStringItem(*provider, "name", infoItem.provName);
                            
                            RTMPItems *pStreams = 0;
                            if(GetListItem(*provider, "streams",  pStreams))
                            {
                                uint32_t idx=0;
                                for(RTMPItems::iterator streamInfo=pStreams->begin(); !bError &&  streamInfo!=pStreams->end(); ++streamInfo)
                                {
                                    double tmp = 0;
                                    int32_t tmp2 = 0;
                                    if( GetNumberItem(*streamInfo, "chunkId", tmp) )
                                    {
                                        infoItem.chunkId = static_cast<uint64_t>(tmp);
                                    }
                                    else if( GetIntegerItem(*streamInfo, "chunkId", tmp2) )
                                    {
                                        infoItem.chunkId = static_cast<uint64_t>(tmp2);
                                    }
                                    
                                    if(GetNumberItem(*streamInfo, "offset", tmp))
                                    {
                                        infoItem.chunkOffset = static_cast<uint64_t>(tmp);
                                    }
                                    else if( GetIntegerItem(*streamInfo, "offset", tmp2) )
                                    {
                                        infoItem.chunkOffset = static_cast<uint64_t>(tmp2);
                                    }
                                    
                                    GetStringItem(*streamInfo, "streamName", infoItem.chunkName);
                                    
                                    RTMPItems *pChanks = 0;
                                    if(GetListItem(*streamInfo, "chunkRange",  pChanks))
                                    {
                                        for(RTMPItems::iterator chank=pChanks->begin(); chank!=pChanks->end(); ++chank)
                                        {
                                            ChunksRangeItem_t tmp;
                                            GetStringItem((*chank), tmp.second);
                                            tmp.first = CStringHelper::aton<uint64_t>((*chank)->getName().c_str());
                                            if(infoItem.chunksRange.end() == std::find(infoItem.chunksRange.begin(), infoItem.chunksRange.end(), tmp))
                                            {
                                                infoItem.chunksRange.push_back(tmp);
                                            }
                                        }
                                    }
                                    
                                    if(GetNumberItem(*streamInfo, "height", tmp))
                                    {
                                        infoItem.streamHeight = tmp;
                                    }
                                    else if( GetIntegerItem(*streamInfo, "height", tmp2) )
                                    {
                                        infoItem.streamHeight = static_cast<double>(tmp2);
                                    }
                                    
                                    GetStringItem(*streamInfo, "description", infoItem.streamName);
                                    
                                    if( 0 == infoItem.streamName.size() )
                                    {
                                        if( 0 < infoItem.streamHeight )
                                        {
                                            bool isTranscoded = false;
                                            GetBoolItem(*streamInfo, "isTranscoded", isTranscoded);
                                            if( isTranscoded )
                                            {
                                                infoItem.streamName = StringFormat("%llup+", static_cast<uint64_t>(infoItem.streamHeight));
                                            }
                                            else
                                            {
                                                infoItem.streamName = StringFormat("%llup", static_cast<uint64_t>(infoItem.streamHeight));
                                            }
                                        }
                                        else
                                        {
                                            infoItem.streamName = "live";
                                        }
                                    }
                                    StreamsInfoList_t::iterator it = streamInfoList.begin();
                                    for(; it != streamInfoList.end(); ++it)
                                    {
                                        if(it->streamName == infoItem.streamName)
                                        {
                                            std::string provNameClean = infoItem.provName;
                                            CStringHelper::replaceAll(provNameClean, "uhs_", "");
                                            infoItem.streamName += StringFormat("_alt_%s", provNameClean.c_str());
                                            break;
                                        }
                                    }
                                    streamInfoList.push_back(infoItem);
                                    ++idx;
                                }
                            }
                            else
                            {
                                printDBG("Problem with getting pStreamInfo\n");
                                bError = true;
                                break;
                            }
                        }
                    }
                    else
                    {
                        std::string offline;
                        if( GetStringItem(list, "stream", offline) && offline == "offline")
                        {
                            break;
                        }
                        printDBG("Problem with getting stream info\n");
                    }
                    
                }
                else
                {
                    printDBG("Problem with getting module info\n");
                }
            }
            catch(const char *err)
            {
                printExc("Exception[%s]\n", err);
            }
        }
    }
    catch(const char *err)
    {
        printExc("Exception[%s]\n", err);
    }
    
    return streamInfoList;
}

const std::string CUDSDownloader::dumpStreamsInfoList(const StreamsInfoList_t &streamInfoList)
{
    printDBG("CUDSDownloader::dumpStreamsInfoList\n");
    printDBG("==================================================================\n");
    std::string out = "{\"stream_info_list\":[";
    StreamsInfoList_t::const_iterator it = streamInfoList.begin();
    for(; it != streamInfoList.end();)
    {
        out += dumpStreamsInfo(*it);
        ++it;
        if(it != streamInfoList.end())
        {
            out += ", \n";
        }
        else
        {
            out += " \n";
        }
    }
    out += "]\n}";
    printDBG("==================================================================\n");
    return out;
}

const std::string CUDSDownloader::dumpStreamsInfo(const CStreamInfo &streamInfo)
{
    std::string out = StringFormat("{\"stream_name\":\"%s\", \n\"chunk_name\":\"%s\", \n\"prov_url\":\"%s\", \n\"prov_name\":\"%s\", \n\"stream_height\":%lf, \n\"chunk_offset\":%llu, \"chunk_id\":%llu, \n\"chunks_range\":[\n", \
                                      streamInfo.streamName.c_str(), streamInfo.chunkName.c_str(), streamInfo.provUrl.c_str(), streamInfo.provName.c_str(), streamInfo.streamHeight, streamInfo.chunkOffset, streamInfo.chunkId);
    ChunksRangeList_t::const_iterator it2 = streamInfo.chunksRange.begin();
    for(; it2 != streamInfo.chunksRange.end();)
    {
        out += StringFormat("{\"chunk_id\":%llu, \"hash\":\"%s\"}", it2->first, it2->second.c_str());
        ++it2;
        if(it2 != streamInfo.chunksRange.end())
        {
            out += ", \n";
        }
        else
        {
            out += " \n";
        }
    }
    out += "]\n}";
    return out;
}

}
