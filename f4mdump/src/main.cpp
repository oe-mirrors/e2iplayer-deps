#include <signal.h>
#include <cstdio>
#include <cstdlib>
#include <string.h> 
#include <sstream> 
#include <vector>

#include "StringHelper.h"
#include "ManifestParser.h"
#include "F4mDownloader.h"
#include "UdsDownloader.h"

#include "console.h"
#include "debug.h"

using namespace f4m;

////////////////////////////////////////////////////////
// GLOBALS
///////////////////////////////////////////////////////
CF4mDownloader g_F4mDownloader;
CUDSDownloader g_UdsDownloader;
bool terminated = false;

void SigHandler (int signum)
{
   // Ignore it so that another Ctrl-C doesn't appear any soon
   signal(signum, SIG_IGN); 
   
   ConsoleAppContainer::getInstance().terminate();
   g_F4mDownloader.terminate();
   g_UdsDownloader.terminate();
   terminated = true;
}

int main(int argc, char *argv[])
{
    //
    ConsoleAppContainer &console = ConsoleAppContainer::getInstance();
    signal(SIGINT, SigHandler);
    
    if(1 == argc)
    {
        printInf("F4MDump v0.80\n");
        printInf("(c) 2014-2016 samsamsam@o2.pl\n");
        return 0;
    }
    
    // get parameters from cmdline 
    if(3 > argc || 5 < argc)
    {
        printDBG("Wrong arguments\n");
        return -1;
    }
    
    std::string wget(argv[1]);
    std::string maniUrl(argv[2]);
    if(3 == argc)
    {
        bool bRet = false;
        std::string streamInfo;
        if(g_UdsDownloader.canHandleUrl(maniUrl))
        {
            g_UdsDownloader.initialize(maniUrl, wget);
            bRet = g_UdsDownloader.reportStreamsInfo(streamInfo);
        }
        else if(g_F4mDownloader.canHandleUrl(maniUrl))
        {
            g_F4mDownloader.initialize(maniUrl, wget);
            bRet = g_F4mDownloader.reportStreamsInfo(streamInfo);
        }
        printInf("%s\n", streamInfo.c_str());
        return bRet ? 0 : -1;
    }
    
    std::string outFile(argv[3]);
    std::string tmpFile("");
    try
    {
        if(g_UdsDownloader.canHandleUrl(maniUrl))
        {
            if(5 == argc)
            {
                g_UdsDownloader.initialize(maniUrl, wget);
                g_UdsDownloader.downloadWithoutTmpFile(wget, outFile, argv[4] );
            }
            else
            {
                printDBG("Wrong number of parameters. 5 is needed for UdsDownloader\n");
            }
        }
        else
        {
            int32_t bitrate = 0;
            if(5 == argc)
            {
                bitrate = CStringHelper::aton<int32_t>(argv[4]);
            }
            // download
            g_F4mDownloader.download(wget, maniUrl, outFile, tmpFile, bitrate);
        }
    }
    catch(const char *err)
    {
        printExc("%s\n", err);
        return -1;
    }
    
    return 0;
}