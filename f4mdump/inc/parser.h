#ifndef SULGE_F4M_PARSER
#define SULGE_F4M_PARSER

#include <string>
#include "debug.h"

namespace f4m
{

enum EStreamType
{
	LIVE,
	RECORDED,
	LIVE_OR_RECORDED
};

struct CManifest
{
    int m_iBitrate;
	EStreamType m_streamType;
};


class CF4MParser
{
public:
    CF4MParser();
    
    ~CF4MParser();

	bool getManifest(const std::string &inData);

private:
    CManifest m_manifest;
    
    std::string add_ns(const std::string &prop);

    
};

}

#endif /* SULGE_F4M_PARSER */