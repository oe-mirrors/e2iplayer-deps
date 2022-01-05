#include <cstdio>
#include <sstream>
#include <list> 
#include <time.h>

#include "tinyxml2.h"
#include "console.h"
#include "debug.h"
#include "parser.h"

#include "ManifestParser.h"
#include "StreamReader.h"
#include "F4mProcessor.h"
#include "SimpleFunctions.h"
#include "F4mDownloader.h"

namespace f4m
{
using namespace tinyxml2;
using namespace iptv;

/**************************************************************************************************
 * Constant and globals variables
 *************************************************************************************************/
const uint32_t CF4mDownloader::TAG_HEADER_LEN = 11;
const uint32_t CF4mDownloader::PREV_TAG_SIZE  = 4;
const uint8_t  CF4mDownloader::AUDIO          = 0x08;
const uint8_t  CF4mDownloader::VIDEO          = 0x09;

const uint8_t CF4mDownloader::FRAME_TYPE_INFO     = 0x05;
const uint8_t CF4mDownloader::CODEC_ID_AVC        = 0x07;
const uint8_t CF4mDownloader::CODEC_ID_AAC        = 0x0A;
const uint8_t CF4mDownloader::AVC_SEQUENCE_HEADER = 0x00;
const uint8_t CF4mDownloader::AAC_SEQUENCE_HEADER = 0x00;
const uint8_t CF4mDownloader::AVC_SEQUENCE_END    = 0x02;

const uint32_t CF4mDownloader::MAX_RETRIES  = 3;
const uint32_t CF4mDownloader::WGET_TIMEOUT = 10;
    
CF4mDownloader::CF4mDownloader()
:m_bTerminate(false)
{
}

CF4mDownloader::~CF4mDownloader()
{
}

void CF4mDownloader::initialize(const std::string &mainUrl, const std::string &wgetCmd)
{
    m_wgetCmd = wgetCmd;
    m_mainUrl = mainUrl;
}

bool CF4mDownloader::canHandleUrl(const std::string &url)
{
    return true;
}

bool CF4mDownloader::reportStreamsInfo(std::string &streamInfo)
{
    try
    {
        printDBG("Qualities report only\n");
        std::stringstream cmd;
        cmd << "{ \"qualities\":[";
        CManifestParser parser(m_wgetCmd);
        parser.parseManifest(m_mainUrl);
        std::vector<int32_t> allBitrates = parser.getAllBitrates();
        for(uint32_t i=0; i < allBitrates.size(); ++i)
        {
            cmd << allBitrates[i];
            if(i < allBitrates.size()-1)
            {
                cmd << ", ";
            }
        }
        cmd << "] }";
        streamInfo = cmd.str();
    }
    catch(const char *err)
    {
        printExc("%s\n", err);
        return false;
    }
    return true;
}

void CF4mDownloader::writeUInt24(FILE *pOutFile, const uint32_t &value)
{
    const uint8_t *ptr = reinterpret_cast<const uint8_t*>(&value);
    uint8_t buff[3] = {ptr[2], ptr[1], ptr[0]} ;
    if(sizeof(buff) != fwrite(buff, sizeof(buff[1]), sizeof(buff), pOutFile))
        throw "WriteInt24 WRONG BUFFER SIZE";
}

void CF4mDownloader::writeFlvFileHeader(FILE *pOutFile, const ByteBuffer_t &metadata, const uint8_t audio, const uint8_t video)
{
    // write flv header  metadata
    // most part of flv header are hard coded, 
    // but there are not used any were else
    {
        uint8_t buff[] = {'F', 'L', 'V', 0x01, 0x05, 0x00, 0x00, 0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x12};
        buff[4] = (audio << 2) | (video);
        if(sizeof(buff) != fwrite(buff, sizeof(buff[1]), sizeof(buff), pOutFile))
            throw "Something wrong happen with writing FLV header A!";
    }
    writeUInt24(pOutFile, metadata.size());
    {
        uint8_t buff[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        if(sizeof(buff) != fwrite(buff, sizeof(buff[1]), sizeof(buff), pOutFile))
            throw "Something wrong happen with writing FLV header B!";
    }
    if(metadata.size() != fwrite(&metadata[0], sizeof(uint8_t), metadata.size(), pOutFile))
    {
        throw "Something wrong happen with writing FLV metadata!";
    }
    {
        uint8_t buff[] = {0x00, 0x00, 0x01, static_cast<uint8_t>(metadata.size() + TAG_HEADER_LEN) };
        if(sizeof(buff) != fwrite(buff, sizeof(buff[1]), sizeof(buff), pOutFile))
            throw "Something wrong happen with writing FLV header C!";
    }
}


void CF4mDownloader::download( const std::string &baseWgetCmd, const std::string &manifestUrl, 
                               const std::string &outFile, const std::string &tmpFile, const int32_t bitrate)
{
    downloadWithoutTmpFile(baseWgetCmd, manifestUrl, outFile, bitrate);
}

void CF4mDownloader::downloadWithoutTmpFile( const std::string &baseWgetCmd, const std::string &manifestUrl, 
                                             const std::string &outFile, const int32_t bitrate)
{
    CManifestParser parser(baseWgetCmd);
    parser.parseManifest(manifestUrl);
    
    MediaEntry media;
    parser.getMedia(media, bitrate); /* get media with given quality */
    CBufferReader bufferReader(media.bootstrap);
    
    F4VBoxHeader header;
    ReadBoxHeader(bufferReader, header);
    dumpDBG(header);
    
    F4VBootstrapInfoBox bootstrapInfoBox;
    ReadBootstrapInfobox(bufferReader, bootstrapInfoBox);
    
    if(!m_bTerminate)
    {
        dumpDBG(bootstrapInfoBox);
        
        std::string downloadUrlBase;
        if(IsHttpUrl(media.url))
        {
            downloadUrlBase = media.url;
        }
        else
        {
            downloadUrlBase = media.baseUrl + "/" + media.url;
        }
        printDBG("downloadUrlBase [%s]\n", downloadUrlBase.c_str());
        
        FILE *pOutFile = fopen(outFile.c_str(), "wb");
        if(0 == pOutFile)
        {
            throw "CF4mDownloader::downloadWithoutTmpFile problem with create out file";
        }
        
        try
        {
            writeFlvFileHeader(pOutFile, media.metadata);
            
            uint64_t totalDownloadSize = 0;
            
            /* headers should be written only once */
            bool AAC_HeaderWritten = false;
            bool AVC_HeaderWritten = false;
            
            bool isLive = bootstrapInfoBox.live;
            printDBG("isLive [%d]\n", isLive);
            bool isEndPresentationDetected = false; 
            uint32_t currentFragment  = static_cast<uint32_t>(-1);
            uint32_t lastFragment     = 0;
            updateBootstrapInfo(bootstrapInfoBox, currentFragment, lastFragment, isEndPresentationDetected);
            printDBG("currentFragment [%d] lastFragment[%d]\n", currentFragment, lastFragment);
            if(isLive && !isEndPresentationDetected ) //&& 1 != getSegmentNum(bootstrapInfoBox, currentFragment) )
            {
                currentFragment = (10 < lastFragment) ? lastFragment-10 : lastFragment;
            }
            printDBG("currentFragment [%d] lastFragment[%d]\n", currentFragment, lastFragment);
            CRawConsoleBuffer inData;
            CStrConsoleBuffer errData;
            while(!m_bTerminate && currentFragment <= lastFragment)
            {
                // prepare cmd
                std::stringstream cmd;
                cmd << baseWgetCmd << " --tries=0 --timeout=" << WGET_TIMEOUT << " -O - " << '"' << downloadUrlBase << "Seg" << getSegmentNum(bootstrapInfoBox, currentFragment) << "-Frag" << currentFragment << '"';
                
                // download fragment
                uint32_t tries = 0;
                int32_t retval = -1;
                do
                {
                    inData.clear();
                    errData.clear();
                    retval = ConsoleAppContainer::getInstance().execute(cmd.str().c_str(), inData, errData);
                    /* first, segments from the list can be no longer available 
                     * when we start downloading live stream, simple they will be skipped :) 
                     */
                    if(0 == totalDownloadSize && isLive && !isEndPresentationDetected)
                    {
                        break;
                    }
                    ++tries;
                } while(!m_bTerminate && 0 != retval && tries < MAX_RETRIES);
                
                if(0 == retval)
                {
                    totalDownloadSize += inData.m_data.size();
                    // read header
                    CBufferReader reader(inData.m_data);
                    
                    while(!m_bTerminate && reader.offset() < reader.size())
                    {
                        F4VBoxHeader header;
                        ReadBoxHeader(reader, header);
                        
                        
                        if( isLive && !isEndPresentationDetected && 0 == memcmp(header.boxType, "abst", sizeof(header.boxType)) && !IsHttpUrl(media.bootstrapUrl) )
                        {
                            /*
                            F4VBootstrapInfoBox newBootstrapInfoBox;
                            ReadBootstrapInfobox(reader, newBootstrapInfoBox);
                            dumpDBG(newBootstrapInfoBox);
                            updateBootstrapInfo(newBootstrapInfoBox, currentFragment, lastFragment, isEndPresentationDetected);
                            */
                            throw "For live stream bootstrap Url is required!";
                        }
                        else if( 0 == memcmp(header.boxType, "mdat", sizeof(header.boxType)) )
                        {
                            if( 0 < reader.offset() && 0 <  reader.size() )
                            {
                                // process mdat box
                                uint64_t totallProcessedBytes = 0;
                                while(!m_bTerminate && totallProcessedBytes < header.payloadSize)
                                {
                                    uint8_t packetType = reader.readUInt8();
                                    uint32_t packetSize = reader.readUInt24();
                                    uint32_t totalTagLen = TAG_HEADER_LEN + packetSize + PREV_TAG_SIZE;
                                    reader.seek(-4, READER_SEEK_CUR);
                                    
                                    switch(packetType)
                                    {
                                        case AUDIO:
                                        case VIDEO:
                                        {
                                            reader.seek(TAG_HEADER_LEN, READER_SEEK_CUR);
                                            
                                            /* init values for VIDEO tag */
                                            uint8_t frameInfo    = reader.readUInt8();
                                            uint8_t avPacketType = reader.readUInt8();
                                            uint8_t codecID      = frameInfo & 0x0F;
                                            
                                            bool *HeaderWritten   = &AVC_HeaderWritten;
                                            uint8_t cmpPacketType = AVC_SEQUENCE_HEADER;
                                            uint8_t cmpCodecID    = CODEC_ID_AVC;
                                            if(AUDIO == packetType)
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
                                                throw "Writing mdat box error!";
                                            }
                                            break;
                                        }
                                        default:
                                        {
                                            printDBG("Packet type [%x] skipped\n", packetType);
                                            break;
                                        }
                                    }
                                    totallProcessedBytes += totalTagLen;
                                    reader.seek(totalTagLen, READER_SEEK_CUR);
                                }
                            }
                            else
                            {
                                throw "Something wrong happen with mdat box!";
                            }
                            break;
                        }
                        else
                        {
                            std::string boxType(header.boxType, sizeof(header.boxType));
                            printDBG("Box type [%s] skipped\n", boxType.c_str());
                            reader.seek(header.payloadSize, READER_SEEK_CUR);
                        }
                    }
                }
                else
                {
                    if( !isLive || isEndPresentationDetected)
                    {
                        printDBG("Download problem cmd[%s] errData[%s]\n", cmd.str().c_str(), errData.m_data.c_str());
                        throw "";
                    }
                }
                
                // report progress
                printInf("{ \"total_download_size\":%llu }\n", totalDownloadSize);
                
                if( isLive && !isEndPresentationDetected && IsHttpUrl(media.bootstrapUrl) && currentFragment == lastFragment)
                {
                    // wait for new fragment 
                    uint32_t newLastFragment = lastFragment;
                    while(!m_bTerminate)
                    {
                        try
                        {
                            parser.refreshBootstrap(media.bootstrapUrl, media);
                            CBufferReader bufferReader(media.bootstrap);
                            F4VBoxHeader header;
                            ReadBoxHeader(bufferReader, header);
                            dumpDBG(header);
                            F4VBootstrapInfoBox newBootstrapInfoBox;
                            ReadBootstrapInfobox(bufferReader, newBootstrapInfoBox);
                            dumpDBG(newBootstrapInfoBox);
                            updateBootstrapInfo(newBootstrapInfoBox, currentFragment, newLastFragment, isEndPresentationDetected);
                            bootstrapInfoBox = newBootstrapInfoBox;
                        }
                        catch(const char *err)
                        {
                            printDBG("error [%s]\n", err);
                        }
                        
                        if(isEndPresentationDetected || newLastFragment > lastFragment)
                        {
                            lastFragment    = newLastFragment;
                            printDBG("OK currentFragment[%d] isEndPresentationDetected[%d]\n", currentFragment, isEndPresentationDetected);
                            break;
                        }
                        else
                        {
                            ::sleep(1);
                        }
                        printDBG("retry [%d] [%d] \n", newLastFragment, currentFragment);
                    }
                }
                ++currentFragment;
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

void CF4mDownloader::terminate()
{
    m_bTerminate = true;
}

uint32_t CF4mDownloader::getSegmentNum(const F4VBootstrapInfoBox &iBox, const uint32_t &iCurrentFragment)
{
    uint32_t oSegForFrag = 1; // default value when only fragment mode is used
    
    const F4VFragmentRunTableBox &fragmentRunTableItem = iBox.fragmentRunTableEntries[0];
    const FragmentRunEntryArray_t &fragTable = fragmentRunTableItem.fragmentRunEntryTable;
    
    const F4VSegmentRunTableBox &segmentRunTableItem = iBox.segmentRunTableEntries[0];
    const F4VSegmentRunEntryArray_t &segTable = segmentRunTableItem.segmentRunEntryTable;
    
    if(1 == fragTable.size() && 1 < segTable.size())
    {
        uint32_t fragment = fragTable[0].firstFragment;
        for(F4VSegmentRunEntryArray_t::const_iterator it = segTable.begin(); it != segTable.end(); ++it)
        {
                if(fragment < iCurrentFragment)
                {
                    oSegForFrag = it->firstSegment;
                }
                fragment += it->fragmentsPerSegment;
                if(fragment > iCurrentFragment)
                {
                    break;
                }
        }
    }
    printDBG("getSegmentNum SegForFrag oSegForFrag[%u] ->  iCurrentFragment[%u]\n", oSegForFrag, iCurrentFragment);
    
    return oSegForFrag;
}

void CF4mDownloader::updateBootstrapInfo(const F4VBootstrapInfoBox &iBox, uint32_t &oCurrentFragment, uint32_t &oLastFragment, bool &isEndPresentationDetected)
{
    uint32_t firstFragment    = static_cast<uint32_t>(-1);
    uint32_t lastFragment     = 0;

    const F4VFragmentRunTableBox &runTableItem = iBox.fragmentRunTableEntries[0];
    const FragmentRunEntryArray_t &fragTable = runTableItem.fragmentRunEntryTable;
    
    const F4VSegmentRunTableBox &segmentRunTableItem = iBox.segmentRunTableEntries[0];
    const F4VSegmentRunEntryArray_t &segTable = segmentRunTableItem.segmentRunEntryTable;
    
    
    if(1 == fragTable.size() && 1 < segTable.size())
    {
        uint64_t calcMediaTime = fragTable[0].firstFragmentTimestamp;
        
        if(0 == fragTable[0].fragmentDuration && 0 == fragTable[0].discontinuityIndicator)
        {
            isEndPresentationDetected = true;
            printDBG("CF4mDownloader::downloadWithoutTmpFile end presentation fragment detected\n");
        }
        else
        {
            
            uint64_t fagDuration   = fragTable[0].fragmentDuration;
            
            firstFragment = fragTable[0].firstFragment;
            lastFragment = firstFragment;
            for(F4VSegmentRunEntryArray_t::const_iterator it = segTable.begin(); it != segTable.end(); ++it)
            {
                calcMediaTime += it->fragmentsPerSegment * fagDuration;
                lastFragment += it->fragmentsPerSegment ;
                
                if(calcMediaTime > iBox.currentMediaTime)
                {
                    for(uint32_t i=0; i<it->fragmentsPerSegment && calcMediaTime > iBox.currentMediaTime; ++i)
                    {
                        calcMediaTime -= fagDuration;
                        --lastFragment;
                    }
                    break;
                }
            }
        }
        printDBG("calcMediaTime[%llu] currentMediaTime[%llu] firstFragment[%u] lastFragment[%u]\n", calcMediaTime, iBox.currentMediaTime, firstFragment, lastFragment);
    }
    else
    {
        for(FragmentRunEntryArray_t::const_iterator it = fragTable.begin(); it != fragTable.end(); ++it)
        {
            if(0 == it->fragmentDuration && 0 == it->discontinuityIndicator)
            {
                isEndPresentationDetected = true;
                printDBG("CF4mDownloader::downloadWithoutTmpFile end presentation fragment detected\n");
            }
            else
            {
                if(firstFragment > it->firstFragment)
                {
                    firstFragment = it->firstFragment; 
                }
                
                if(lastFragment < it->firstFragment)
                {
                    lastFragment = it->firstFragment; 
                }
            }
        }
    }
    
    if(static_cast<uint32_t>(-1) == oCurrentFragment)
    {
        oCurrentFragment = firstFragment;
    }

    if( (0 == iBox.update &&  0 == runTableItem.flags) )
    {
        // new table
        if(firstFragment > oCurrentFragment)
        {
            oCurrentFragment = firstFragment;
        }
        if(lastFragment > oLastFragment)
        {
            oLastFragment = lastFragment;
        }
    }
    else 
    {   
        // uodate table
        if(oLastFragment < lastFragment)
        {
            oLastFragment = lastFragment; 
        }
    }
}

}
