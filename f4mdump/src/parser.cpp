#include <string>

#include "parser.h"
#include "tinyxml2.h"

using namespace tinyxml2;
using namespace std;
using namespace f4m;


CF4MParser::CF4MParser()
{
}

CF4MParser::~CF4MParser()
{
}

string CF4MParser::add_ns(const string &prop)
{
    const static string ns("{http://ns.adobe.com/f4m/1.0}");
    return ns + prop;
}

bool CF4MParser::getManifest(const string &inData)
{
    bool bRet = false;
    
    XMLDocument doc;
    XMLError xmlError = doc.Parse(inData.c_str(), inData.size());
    if(XML_NO_ERROR == xmlError)
    {
        printDBG("Manifest parser error [%d]\n", xmlError);
    
        XMLElement *levelElement = doc.FirstChildElement(add_ns("media").c_str());
        for(XMLElement* child = levelElement->FirstChildElement(); child != NULL; child = child->NextSiblingElement())
        {
            // do something with each child element
        }
    }
        
    return bRet;
}