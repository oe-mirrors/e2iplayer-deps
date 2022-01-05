#include "F4mProcessor.h"
#include "debug.h"

namespace f4m
{
using namespace iptv;

void ReadFragmentRunTableBox(IStreamReader &buffer, F4VFragmentRunTableBox &box)
{
    box.version   = buffer.readUInt8();
    box.flags     = buffer.readUInt24();
    box.timeScale = buffer.readUInt32();

    uint8_t qualityEntryCount = buffer.readUInt8();
    for(uint8_t i=0; i<qualityEntryCount; ++i)
    {
        box.qualitySegmentUrlModifiers.push_back( buffer.readString() );
    }
    
    uint32_t fragmentRunEntryCount = buffer.readUInt32();
    for(uint32_t i=0; i<fragmentRunEntryCount; ++i)
    {
        F4VFragmentRunEntry fragmentRunEntry;
        fragmentRunEntry.firstFragment           = buffer.readUInt32();
        fragmentRunEntry.firstFragmentTimestamp  = buffer.readUInt64();
        fragmentRunEntry.fragmentDuration        = buffer.readUInt32();
        fragmentRunEntry.discontinuityIndicator  = (0 == fragmentRunEntry.fragmentDuration) ? buffer.readUInt8() : 0;
        box.fragmentRunEntryTable.push_back(fragmentRunEntry);
    }
}

void ReadSegmentRunTableBox(IStreamReader &buffer, F4VSegmentRunTableBox &box)
{
    box.version = buffer.readUInt8();
    box.flags   = buffer.readUInt24();
    
    uint8_t qualityEntryCount = buffer.readUInt8();
    for(uint8_t i=0; i<qualityEntryCount; ++i)
    {
        box.qualitySegmentUrlModifiers.push_back( buffer.readString() );
    }
    
    uint32_t segmentRunEntryCount = buffer.readUInt32();
    for(uint32_t i=0; i<segmentRunEntryCount; ++i)
    {
        F4VSegmentRunEntry segmentRunEntry;
        segmentRunEntry.firstSegment = buffer.readUInt32();;
        segmentRunEntry.fragmentsPerSegment = buffer.readUInt32();;
        box.segmentRunEntryTable.push_back(segmentRunEntry);
    }
}

void ReadBootstrapInfobox(IStreamReader &buffer, F4VBootstrapInfoBox &box)
{
    box.version = buffer.readUInt8();
    box.flags   = buffer.readUInt24();
    box.bootstrapinfoVersion = buffer.readUInt32();
    
    uint8_t tmp = buffer.readUInt8();
    box.profile = (tmp >> 6 ) & 0x03; // 2bits
    box.live    = (tmp >> 5 ) & 0x01; // 1bit
    box.update  = (tmp >> 4 ) & 0x01; // 1bit
    
    box.timeScale = buffer.readUInt32();
    box.currentMediaTime = buffer.readUInt64();
    box.smpteTimeCodeOffset = buffer.readUInt64();
    box.movieIdentifier = buffer.readString();
    
    uint8_t serverEntryCount = buffer.readUInt8();
    for(uint8_t i=0; i<serverEntryCount; ++i)
    {
        box.serverEntryTable.push_back( buffer.readString() );
    }
    
    uint8_t qualityEntryCount = buffer.readUInt8();
    for(uint8_t i=0; i<qualityEntryCount; ++i)
    {
        box.qualityEntryTable.push_back( buffer.readString() );
    }
    
    box.drmData  = buffer.readString();
    box.metaData = buffer.readString();
    
    uint8_t segmentRunTableCount = buffer.readUInt8();
    for(uint8_t i=0; i<segmentRunTableCount; ++i)
    {
        F4VBoxHeader header;
        ReadBoxHeader(buffer, header);
        dumpDBG(header);//SULGE
        if(0 == memcmp(&header.boxType, SegmentRunTableType, sizeof(SegmentRunTableType)))
        {
            F4VSegmentRunTableBox segmentRunTableBox;
            ReadSegmentRunTableBox(buffer, segmentRunTableBox);
            box.segmentRunTableEntries.push_back(segmentRunTableBox);
        }
        else
        {
            throw "F4VReader::readBootstrapInfobox SegmentRunTableType wrong box type";
        }
    }
    
    uint8_t fragmentRunTableCount = buffer.readUInt8();
    for(uint8_t i=0; i<fragmentRunTableCount; ++i)
    {
        F4VBoxHeader header;
        ReadBoxHeader(buffer, header);
        dumpDBG(header);//SULGE
        if(0 == memcmp(&header.boxType, FragmentRunTableType, sizeof(FragmentRunTableType)))
        {
            F4VFragmentRunTableBox fragmentRunTableBox;
            ReadFragmentRunTableBox(buffer, fragmentRunTableBox);
            box.fragmentRunTableEntries.push_back(fragmentRunTableBox);
        }
        else
        {
            throw "ReadBootstrapInfobox FragmentRunTableType wrong box type";
        }
    }
}

void ReadBoxHeader(IStreamReader &buffer, F4VBoxHeader &header)
{
    header.payloadSize = buffer.readUInt32();
    if( sizeof(header.boxType) != buffer.read(header.boxType, sizeof(header.boxType)) )
    {
        throw "ReadBoxHeader";
    }

    if(1 == header.payloadSize) // extended size available
    {
        header.payloadSize  = buffer.readUInt64();
        header.payloadSize -= 16;
    }
    else
    {
        header.payloadSize -= 8;
    }
}

///////////////////////////////////////////////////////////////////////////////////

void dumpDBG(const F4VBoxHeader &box)
{
    printDBG("\t\t==============================================\n");
    printDBG("\t\tF4VBoxHeader\n");
    printDBG("\t\t==============================================\n");
    printDBG("\t\tpayloadSize:\t%llu\n", box.payloadSize);
    printDBG("\t\tboxType:\t");
    for(uint8_t i=0; i<4; ++i)
    {
        printDBG("%c", box.boxType[i]);
    }
    printDBG("\n\t\t==============================================\n");
}

void dumpDBG(const F4VFragmentRunEntry &box)
{
    printDBG("\t\t==============================================\n");
    printDBG("\t\tF4VFragmentRunEntry\n");
    printDBG("\t\t==============================================\n");
    printDBG("\t\tfirstFragment:\t%u\n", (uint32_t)box.firstFragment);
    printDBG("\t\tfirstFragmentTimestamp:\t%llu\n", (uint64_t)box.firstFragmentTimestamp);
    printDBG("\t\tfragmentDuration:\t%u\n", (uint32_t)box.fragmentDuration);
    printDBG("\t\tdiscontinuityIndicator:\t%u\n", (uint32_t)box.discontinuityIndicator);
    printDBG("\t\t==============================================\n");
}

void dumpDBG(const F4VSegmentRunEntry &box)
{
    printDBG("\t\t==============================================\n");
    printDBG("\t\tF4VSegmentRunEntry\n");
    printDBG("\t\t==============================================\n");
    printDBG("\t\tfirstSegment:\t%u\n", (uint32_t)box.firstSegment);
    printDBG("\t\tfragmentsPerSegment:\t%u\n", (uint32_t)box.fragmentsPerSegment);
    printDBG("\t\t==============================================\n");
}

void dumpDBG(const F4VSegmentRunTableBox &box)
{
    printDBG("\t==============================================\n");
    printDBG("\tF4VSegmentRunTableBox\n");
    printDBG("\t==============================================\n");
    printDBG("\tversion:\t%u\n", (uint32_t)box.version);
    printDBG("\tflags:\t%u\n", (uint32_t)box.flags);
    printDBG("\tqualitySegmentUrlModifiers:\n");
    for(uint32_t i=0; i< box.qualitySegmentUrlModifiers.size(); ++i)
    {
        printDBG("\t\t%d: %s\n", i+1, box.qualitySegmentUrlModifiers[i].c_str());
    }
    
    printDBG("\tsegmentRunEntryTable:\n");
    for(uint32_t i=0; i< box.segmentRunEntryTable.size(); ++i)
    {
        dumpDBG(box.segmentRunEntryTable[i]);
    }
}

void dumpDBG(const F4VFragmentRunTableBox &box)
{
    printDBG("\t==============================================\n");
    printDBG("\tF4VFragmentRunTableBox\n");
    printDBG("\t==============================================\n");
    printDBG("\tversion:\t%u\n", (uint32_t)box.version);
    printDBG("\tflags:\t%u\n", (uint32_t)box.flags);
    printDBG("\ttimeScale:\t%u\n", (uint32_t)box.timeScale);
    printDBG("\tqualitySegmentUrlModifiers:\n");
    for(uint32_t i=0; i< box.qualitySegmentUrlModifiers.size(); ++i)
    {
        printDBG("\t\t%d: %s\n", i+1, box.qualitySegmentUrlModifiers[i].c_str());
    }
    
    printDBG("\tfragmentRunEntryTable:\n");
    for(uint32_t i=0; i< box.fragmentRunEntryTable.size(); ++i)
    {
        dumpDBG(box.fragmentRunEntryTable[i]);
    }
}


void dumpDBG(const F4VBootstrapInfoBox &box)
{
    printDBG("==============================================\n");
    printDBG("F4VBootstrapInfoBox\n");
    printDBG("==============================================\n");
    printDBG("version:\t%u\n", (uint32_t)box.version);
    printDBG("flags:\t%u\n", (uint32_t)box.flags);
    printDBG("bootstrapinfoVersion:\t%u\n", (uint32_t)box.bootstrapinfoVersion);
    printDBG("profile:\t%u\n", (uint32_t)box.profile);
    printDBG("live:\t%u\n", (uint32_t)box.live);
    printDBG("update:\t%u\n", (uint32_t)box.update);
    printDBG("timeScale:\t%u\n", (uint32_t)box.timeScale);
    printDBG("currentMediaTime:\t%llu\n", (uint64_t)box.currentMediaTime);
    printDBG("smpteTimeCodeOffset:\t%llu\n", (uint64_t)box.smpteTimeCodeOffset);
    printDBG("movieIdentifier:\t%s\n", box.movieIdentifier.c_str());
    printDBG("serverEntryTable:\n");
    for(uint32_t i=0; i< box.serverEntryTable.size(); ++i)
    {
        printDBG("\t%d: %s\n", i+1, box.serverEntryTable[i].c_str());
    }
    printDBG("qualityEntryTable:\n");
    for(uint32_t i=0; i< box.qualityEntryTable.size(); ++i)
    {
        printDBG("\t%d: %s\n", i+1, box.qualityEntryTable[i].c_str());
    }
    
    printDBG("drmData:\t%s\n", box.drmData.c_str());
    printDBG("metaData:\t%s\n", box.metaData.c_str());
    
    printDBG("segmentRunTableEntries:\n");
    for(uint32_t i=0; i< box.segmentRunTableEntries.size(); ++i)
    {
        dumpDBG(box.segmentRunTableEntries[i]);
    }
    
    printDBG("fragmentRunTableEntries:\n");
    for(uint32_t i=0; i< box.fragmentRunTableEntries.size(); ++i)
    {
        dumpDBG(box.fragmentRunTableEntries[i]);
    }
    
    printDBG("==============================================\n");
}

}