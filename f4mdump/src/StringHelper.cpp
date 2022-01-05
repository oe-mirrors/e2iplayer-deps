#include "StringHelper.h"
#include "SimpleFunctions.h"

/*****************************************************************************
 * Using namespace(s)
 ****************************************************************************/
using namespace std;

bool CStringHelper::is_number(const string &inStr)
{
    string::const_iterator it = inStr.begin();
    while (it != inStr.end() && std::isdigit(*it))
    {
        ++it;
    }

    return !inStr.empty() && it == inStr.end();
}

void CStringHelper::splitString(const string &inStr, vector<string> &outVect,
		                            const char delim, const bool addEmpty)
{
	size_t pos = inStr.find(delim);
	size_t pos2 = 0;

	if(string::npos == pos)
	{
		if( 0 < inStr.size() || addEmpty)
		{
			outVect.push_back(inStr);
		}
	}
	else
	{
		while( string::npos != pos )
		{
			if( (pos - pos2) > 0 || addEmpty)
			{
				outVect.push_back( inStr.substr(pos2, pos - pos2) );
			}

			if( (pos + 1) < inStr.size() )
			{
				pos2 = pos + 1;
				pos = inStr.find(delim, pos2);
			}
			else
			{
				break;
			}
		}
	}
}

string CStringHelper::trim(const string& str,
							 const string& whitespace /*= " \t"*/)
{
	const size_t strBegin = str.find_first_not_of(whitespace);
    if (strBegin == string::npos)
        return ""; // no content

    const size_t strEnd = str.find_last_not_of(whitespace);
    const size_t strRange = strEnd - strBegin + 1;

    return str.substr(strBegin, strRange);
}

void CStringHelper::replaceAll(std::string& str, const std::string& from, const std::string& to)
{
    if(from.empty())
    {
        return;
    }
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length();
    }
}

void CStringHelper::replace(std::string& str, const std::string& from, const std::string& to)
{
    size_t start_pos = str.find(from);
    if(start_pos != std::string::npos)
    {  
        str.replace(start_pos, from.length(), to);
    }
}

bool CStringHelper::startsWith(const std::string& haystack, const std::string& needle)
{
    return (0 == haystack.find(needle)) ? true : false;
}


