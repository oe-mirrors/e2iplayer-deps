#ifndef _F4M_PROCESSOR_H_
#define _F4M_PROCESSOR_H_

#include "SimpleDataTypes.h"
#include "StreamReader.h"

namespace f4m
{

const uint8_t SegmentRunTableType[] = {'a', 's', 'r', 't'};
const uint8_t FragmentRunTableType[] = {'a', 'f', 'r', 't'};

struct F4VBoxHeader
{
    uint64_t payloadSize;
    char boxType[4];
};

struct F4VSegmentRunEntry
{
    uint32_t  firstSegment;
    uint32_t  fragmentsPerSegment;
};
typedef std::vector<F4VSegmentRunEntry> F4VSegmentRunEntryArray_t;


struct F4VFragmentRunEntry
{
    uint32_t  firstFragment;
    uint64_t  firstFragmentTimestamp;
    uint32_t  fragmentDuration;
    uint8_t   discontinuityIndicator;
};
typedef std::vector<F4VFragmentRunEntry> FragmentRunEntryArray_t;


struct F4VFragmentRunTableBox
{
    /* BoxType ='afrt' (0x61667274) */
    uint8_t   version;
    uint32_t  flags; // 0 = A full table, 1 = The records in this table are updates (or new entries to be appended) 
    uint32_t  timeScale;
    StringArray_t  qualitySegmentUrlModifiers;
    FragmentRunEntryArray_t fragmentRunEntryTable;
};
typedef std::vector<F4VFragmentRunTableBox> FragmentRunTableBoxArray_t;

struct F4VSegmentRunTableBox
{
    /* BoxType = 'asrt' (0x61737274) */
    uint8_t   version;
    uint32_t  flags; // 0 = A full table, 1 = The records in this table are updates (or new entries to be appended) 
    StringArray_t  qualitySegmentUrlModifiers;
    F4VSegmentRunEntryArray_t segmentRunEntryTable;
};
typedef std::vector<F4VSegmentRunTableBox> SegmentRunTableBoxArray_t;

struct F4VBootstrapInfoBox
{
    /* BoxType = 'abst' (0x61627374) */
    uint8_t  version;
    uint32_t flags;
    uint32_t bootstrapinfoVersion;
    uint8_t  profile;
    uint8_t  live;
    uint8_t  update;
    uint32_t timeScale;
    uint64_t currentMediaTime;
    uint64_t smpteTimeCodeOffset;
    std::string movieIdentifier;
    StringArray_t serverEntryTable;
    StringArray_t qualityEntryTable;
    std::string drmData;
    std::string metaData;
    SegmentRunTableBoxArray_t segmentRunTableEntries;
    FragmentRunTableBoxArray_t fragmentRunTableEntries;
};

void ReadFragmentRunTableBox(iptv::IStreamReader &buffer, F4VFragmentRunTableBox &box);
void ReadSegmentRunTableBox(iptv::IStreamReader &buffer, F4VSegmentRunTableBox &box);
void ReadBootstrapInfobox(iptv::IStreamReader &buffer, F4VBootstrapInfoBox &box);
void ReadBoxHeader(iptv::IStreamReader &buffer, F4VBoxHeader &header);

void dumpDBG(const F4VBoxHeader &box);
void dumpDBG(const F4VBootstrapInfoBox &box);

}

#endif // _F4M_PROCESSOR_H_