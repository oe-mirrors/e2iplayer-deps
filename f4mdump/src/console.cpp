#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>

#include "console.h"
#include "debug.h"

int bidirpipe(int pfd[], const char *cmd , const char * const argv[], const char *cwd )
{
    int pfdin[2];  /* from child to parent */
    int pfdout[2]; /* from parent to child */
    int pfderr[2]; /* stderr from child to parent */
    int pid;       /* child's pid */

    if ( pipe(pfdin) == -1 || pipe(pfdout) == -1 || pipe(pfderr) == -1)
    {
        return(-1);
    }

    if ( ( pid = vfork() ) == -1 )
    {
        return(-1);
    }
    else if (pid == 0) /* child process */
    {
        setsid();
        if ( close(0) == -1 || close(1) == -1 || close(2) == -1 )
        {
            _exit(0);
        }

        if (dup(pfdout[0]) != 0 || dup(pfdin[1]) != 1 || dup(pfderr[1]) != 2 )
        {
            _exit(0);
        }

        if (close(pfdout[0]) == -1 || close(pfdout[1]) == -1 ||
                close(pfdin[0]) == -1 || close(pfdin[1]) == -1 ||
                close(pfderr[0]) == -1 || close(pfderr[1]) == -1 )
        {
            _exit(0);
        }
        
        for (unsigned int i=3; i < 90; ++i )
        {
            close(i);
        }

        if (cwd)
        {
            if( chdir(cwd) == -1)
            {
                printDBG("bidirpipe chdir to cwd[%s] failed\n", cwd);
            }
        }

        execvp(cmd, (char * const *)argv);
        /* the vfork will actually suspend the parent thread until execvp is called. thus it's ok to use the shared arg/cmdline pointers here. */
        _exit(0);
    }
    if (close(pfdout[0]) == -1 || close(pfdin[1]) == -1 || close(pfderr[1]) == -1)
    {
        return(-1);
    }

    pfd[0] = pfdin[0];
    pfd[1] = pfdout[1];
    pfd[2] = pfderr[0];

    return(pid);
}

ConsoleAppContainer::ConsoleAppContainer()
{
}

ConsoleAppContainer::~ConsoleAppContainer()
{
}

void ConsoleAppContainer::terminate()
{
    m_bTerm = true;
}

int ConsoleAppContainer::execute(const std::string &cmd, IConsoleBuffer &inData, IConsoleBuffer &errData)
{
    printDBG("ConsoleAppContainer::execute cmd[%s]\n", cmd.c_str());
    
    int argc = 3;
    const char *argv[argc + 1];
    argv[0] = "/bin/sh";
    argv[1] = "-c";
    argv[2] = cmd.c_str();
    argv[argc] = NULL;
    
    int pid = -1;
    int fd[3] = {-1, -1, -1};
    
    inData.clear();
    errData.clear();
    
    if(m_bTerm)
    {
        return -1;
    }

    // get one read, one write and the err pipe to the prog..
    pid = bidirpipe(fd, argv[0], argv, 0);
    if( -1 == pid)
    {
        return -1;
    }
    
    ::fcntl(fd[0], F_SETFL, O_NONBLOCK);
    //::fcntl(fd[1], F_SETFL, O_NONBLOCK);
    ::fcntl(fd[2], F_SETFL, O_NONBLOCK);

    bool bFinished = false;
    while(!m_bTerm && !bFinished)
    {
        struct timeval tv;
        int maxFd = -1;
        
        fd_set read_fd;
        tv.tv_sec  = 1;
        tv.tv_usec = 0;
 
        FD_ZERO(&read_fd);
        FD_SET(fd[0], &read_fd);
        //FD_SET(fd[1], &read_fd);
        FD_SET(fd[2], &read_fd);
 
        /* Get max file descriptor number for select */
        maxFd = fd[0] > fd[2] ? fd[0] : fd[2]; 
        
        int ret = select(maxFd + 1, &read_fd, NULL, NULL, &tv);
        // a return value of 0 means that the time expired
        // without any acitivity on the file descriptor
        if(0 == ret)
        {
            continue;
        }
        else if(0 > ret)
        {
            if(EINTR == errno)
            {
                continue;
            }
            else
            {
                /* Probably some interrupt occur */
                m_bTerm = true;
                break;
            }
        }

        bool tmpRet = true;
        if(FD_ISSET(fd[0], &read_fd))
        {
            tmpRet = readData(fd[0], inData) && tmpRet; 
        }
        
        if(FD_ISSET(fd[2], &read_fd))
        {
            tmpRet = readData(fd[2], errData) && tmpRet;
        }
        
        bFinished = (!tmpRet);
    }

    if(m_bTerm)
    {
        ::kill(-pid, SIGINT);
    }

    for(unsigned i=0; i<3; ++i)
    {
        ::close(fd[i]);
    }
    
    int childstatus;
    int retval = -1;
    /*
     * We have to call 'wait' on the child process, in order to avoid zombies.
     * Also, this gives us the chance to provide better exit status info to appClosed.
     */
    if (0 < ::waitpid(pid, &childstatus, 0))
    {
        if (WIFEXITED(childstatus))
        {
            retval = WEXITSTATUS(childstatus);
        }
    }
    return retval;
}

bool ConsoleAppContainer::readData(const int fd, IConsoleBuffer &outBuffer)
{
    bool bDone = true;
    uint8_t tmpBuffer[256];
    int nBytes = 0;
    
    while(!m_bTerm)
    {
        //memset(tmpBuffer, 0, sizeof(tmpBuffer));
        nBytes = read(fd, tmpBuffer, sizeof(tmpBuffer));
        if(0 < nBytes)
        {
            outBuffer.append(tmpBuffer, nBytes);
            continue;
        }
        else if(0 == nBytes || (-1 == nBytes && errno != EAGAIN))
        {
            bDone = false;
        }
        break;
    }
    
    return bDone;
}
