#include <vector>
#include <algorithm>

#include "StringHelper.h"
#include "ManifestParser.h"
#include "console.h"
#include "debug.h"

/*****************************************************************************
 * Using namespace(s)
 ****************************************************************************/
using namespace std;
using namespace tinyxml2;


/*************************************************************************************************
* namespace
**************************************************************************************************/
namespace f4m
{

CManifestParser::CManifestParser(const std::string &wgetCMD)
: m_sWgetCMD(wgetCMD)
{
    // do nothing
}

CManifestParser::~CManifestParser()
{
    // do nothing
}

bool CManifestParser::getMedia(MediaEntry &media, const int32_t bitrate/*=0*/) const
{
    if(0 < m_media.size())
    {
        uint32_t idx = 0;
        if(1 < m_media.size() && 0 < bitrate)
        {
            for(uint32_t i=0; i<m_media.size() &&
                               m_media[i].bitrate <= bitrate; ++i)
            {
                idx = i;
            }
        }
        media = m_media[idx];
        return true;
    }
    return false;
}

bool CManifestParser::getManifest(const string &url, XMLDocument &xmlDoc)
{
    printDBG("CManifestParser::getManifest [%s]\n", url.c_str());
    string output;
    bool bRet = getPage(url, output);
    if(bRet)
    {
        XMLError xmlError = xmlDoc.Parse(output.c_str(), output.size());
        if(xmlError != XML_NO_ERROR)
        {
            printDBG("XML parse Error\n");
            bRet = false;
        }
    }

    printDBG("CManifestParser::getManifest end ret[%s]\n", bRet ? "true":"false");
    return bRet;
}

bool CManifestParser::getPage(const string &url, string &data)
{
    printDBG("CManifestParser::getPage [%s]\n", url.c_str());

    CStrConsoleBuffer inData;
    CStrConsoleBuffer errData;
    
    // prepare cmd
    std::stringstream cmd;
    cmd << m_sWgetCMD << " --tries=0 --timeout=10 -O - " << '"' << url << '"';
    
    // download
    int retval = ConsoleAppContainer::getInstance().execute(cmd.str().c_str(), inData, errData);
    if(0 != retval)
    {
       printDBG("CManifestParser::getPage error[%d], message[%s]\n", retval, errData.m_data.c_str());
       return false;
    }
    printDBG("CManifestParser::getPage data[%s]\n", inData.m_data.c_str());
    data = inData.m_data;
    return true;
}

void  CManifestParser::refreshBootstrap(const std::string &bootstrapUrl, MediaEntry &mediaEntry)
{
    printDBG("CManifestParser::refreshBootstrap [%s]\n", bootstrapUrl.c_str());
    mediaEntry.bootstrapUrl = bootstrapUrl;
    
    // prepare cmd
    std::stringstream cmd;
    cmd << m_sWgetCMD << " --tries=0 --timeout=10 -O - " << '"' << bootstrapUrl << '"';
    CRawConsoleBuffer inData;
    CStrConsoleBuffer errData;
    int32_t retval = ConsoleAppContainer::getInstance().execute(cmd.str().c_str(), inData, errData);
    if(0 == retval)
    {
        mediaEntry.bootstrap = inData.m_data;
    }
    else
    {
        throw "CManifestParser::refreshBootstrap bootstrap download error!";
    }
}

bool CManifestParser::parseManifest(const string &mainUrl)
{
    LogDebug("CManifestParser::parseManifest - start");
    XMLDocument mainXml;
    if(!getManifest(mainUrl, mainXml))
    {
        LogError("Problem with getting main manifest!");
        return false;
    }

    string sBaseUrl;
    XMLElement *pXmlElem = 0;
    XMLElement *pRoot = mainXml.RootElement();

    if(0 == pRoot)
    {
        LogError("Root element is NULL!");
        return false;
    }
    pXmlElem = pRoot->FirstChildElement("baseURL");
    if(pXmlElem)
    {
        const char *str = pXmlElem->GetText();
        if(str)
        {
            sBaseUrl = str;
            sBaseUrl = CStringHelper::trim(sBaseUrl);
        }
    }

    // get all media items
    pXmlElem = pRoot->FirstChildElement("media");
    while(pXmlElem)
    {
        int32_t bitrate = 0;
        string url;
        XMLDocument xml;
        // get attribute bitrate
        {
            const char *value = pXmlElem->Attribute("bitrate");
            if(value && CStringHelper::is_number(value))
            {
                bitrate = CStringHelper::aton<int32_t>(value);
            }
        }
        // get attribute href
        {
            const char *value = pXmlElem->Attribute("href");
            if(value)
            {
                url = sBaseUrl + value;
            }
        }
        // get parent manifest
        if(0 < url.size() && getManifest(url, xml))
        {
            addMediaEntries(url, xml, bitrate);
        }
        pXmlElem = pXmlElem->NextSiblingElement("media");
    }

    if(0 == m_media.size())
    {
        // last chance
        addMediaEntries(mainUrl, mainXml);
    }

    if( 0 < m_media.size() )
    {
        // sort by bitrate
        std::sort(m_media.begin(), m_media.end());
        LogInfo("Report sorted qualities:\n");
        for(size_t idx=0; idx<m_media.size(); idx++)
        {
            LogInfo(StringFormat("idx[%d] bitrate[%d]\n", idx, m_media[idx].bitrate));
        }
        LogError("More than one quality is available - first will be used!");
    }

    if(0 == m_media.size())
    {
        LogError("There is no valid media link!");
        return false;
    }

    LogDebug("CManifestParser::parseManifest - end");
    return true;
}

bool CManifestParser::addMediaEntries(const std::string &url,
                                      tinyxml2::XMLDocument &xml,
                                      const int32_t bitrate /*=0*/)
{
    LogDebug("CManifestParser::addMediaEntries - start");

    //Extract baseUrl from manifest url
    XMLElement *pRoot = xml.RootElement();
    if(0 == pRoot)
    {
        LogError("Root element is NULL!");
        return false;
    }

    string baseUrl;
    const XMLElement *pBaseUrlElem = pRoot->FirstChildElement("baseURL");
    if(pBaseUrlElem)
    {
        const char *str = pBaseUrlElem->GetText();
        if(str)
        {
            baseUrl = str;
            baseUrl = CStringHelper::trim(baseUrl);
        }
    }
    else
    {
        baseUrl = url;
        size_t idx = baseUrl.find('?');
        if(string::npos != idx)
        {
            baseUrl = baseUrl.substr(0, idx);
        }
        idx = baseUrl.find_last_of('/');
        if(string::npos != idx)
        {
            baseUrl = baseUrl.substr(0, idx);
        }
    }

    if(!IsHttpUrl(baseUrl))
    {
        LogError("Provided manifest is not a valid HDS manifest");
    }

    // get all streams items
    XMLElement *pMediaElem = pRoot->FirstChildElement("media");
    while(pMediaElem)
    {
        MediaEntry mediaEntry;
        // get metadata
        const XMLElement *pMetadataElem = pMediaElem->FirstChildElement("metadata");
        if(pMetadataElem)
        {
            const char *str = pMetadataElem->GetText();
            if(str)
            {
                b64decode(str, mediaEntry.metadata);
            }
        }
        // get attribute bitrate
        {
            const char *value = pMediaElem->Attribute("bitrate");
            if(value && CStringHelper::is_number(value))
            {
                mediaEntry.bitrate = CStringHelper::aton<int32_t>(value);
            }
            else
            {
                mediaEntry.bitrate = bitrate;
            }
        }
        // get attribute streamId
        {
            const char *value = pMediaElem->Attribute("streamId");
            if(value)
            {
                mediaEntry.streamId = value;
            }
        }
        // get attribute url
        {
            const char *value = pMediaElem->Attribute("url");
            if(value)
            {
                mediaEntry.url = value;
                if( 0 < mediaEntry.url.size() && '/' == mediaEntry.url[0])
                {
                    mediaEntry.url = mediaEntry.url.substr(1);
                }
            }
        }
        mediaEntry.baseUrl = baseUrl;

        // get bootstrap
        {
            XMLElement *pBootstrapInfo = 0;
            const char *pBootstrapInfoId = pMediaElem->Attribute("bootstrapInfoId");
            if(pBootstrapInfoId)
            {
                // check all bootstrapInfo
                pBootstrapInfo = pRoot->FirstChildElement("bootstrapInfo");
                while(pBootstrapInfo)
                {
                    if(pBootstrapInfo->Attribute("id", pBootstrapInfoId))
                    {
                        break;
                    }
                    pBootstrapInfo = pRoot->NextSiblingElement("bootstrapInfo");
                }
            }

            if(0 == pBootstrapInfo)
            {
                pBootstrapInfo = pRoot->FirstChildElement("bootstrapInfo");
            }

            if(0 != pBootstrapInfo)
            {
                const char *value = pBootstrapInfo->Attribute("url");
                if(value)
                {
                    string bootstrapUrl = value;
                    if(!IsHttpUrl(bootstrapUrl))
                    {
                        bootstrapUrl = mediaEntry.baseUrl + '/' + bootstrapUrl;
                    }
                    refreshBootstrap(bootstrapUrl, mediaEntry);
                }
                else
                {
                    // get direct bootstrap
                    const char *str = pBootstrapInfo->GetText();
                    if(str)
                    {
                        b64decode(str, mediaEntry.bootstrap);
                    }
                }
            }
        }

        m_media.push_back(mediaEntry);
        pMediaElem = pMediaElem->NextSiblingElement("media");
    }
    LogDebug("CManifestParser::addMediaEntries - end");
    return true;
}

} /* namespace f4f*/
