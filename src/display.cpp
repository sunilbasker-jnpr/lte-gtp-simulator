/*  Copyright (C) 2013  Nithin Nellikunnu, nithin.nn@gmail.com
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <curses.h>
#include <signal.h>
#include <iostream>
#include <exception>
#include <list>
#include <vector>
#include <fstream>

#include "types.hpp"
#include "error.hpp"
#include "macros.hpp"
#include "logger.hpp"
#include "timer.hpp"
#include "task.hpp"
#include "gtp_types.hpp"
#include "sim_cfg.hpp"
#include "gtp_util.hpp"
#include "gtp_if.hpp"
#include "gtp_ie.hpp"
#include "gtp_msg.hpp"
#include "keyboard.hpp"
#include "procedure.hpp"
#include "scenario.hpp"
#include "gtp_stats.hpp"
#include "display.hpp"

#define COUT std::cout
#define CIN std::cin
#define ENDL std::endl
#define CLEAR_SCREEN() printf("\033[2J")
#define ENDLINE "\r\n"
#define PRINT_SEPERATOR()                                                      \
    {                                                                          \
        fprintf(stdout,                                                        \
            "+------------------------+----------------------------+---------" \
            "--------------+\r\n");                                            \
    }

#define PRINT_END_SEPERATOR_PAUSE()                          \
    {                                                        \
        fprintf(stdout,                                      \
            "+--Adjust-Rate [+|-|*|/]--+-----Pause-Traffic " \
            "[p]-----+-------Quit [q]--------+\r\n");        \
    }

#define PRINT_END_SEPERATOR_RESUME()                        \
    {                                                       \
        fprintf(stdout,                                     \
            "+---Adjust-Rate [+/-]----+----Resume-Traffic " \
            "[c]------+-------Quit [q]--------+\r\n");      \
    }

#define PRINT_BLANK_LINE()       \
    {                            \
        fprintf(stdout, "\r\n"); \
    }

class Display *Display::m_pDisp = NULL;
ofstream fout;
PRIVATE VOID exit_handler();
PRIVATE VOID dump_stats();

Display *Display::getInstance()
{
    try
    {
        if (NULL == m_pDisp)
        {
            m_pDisp = new Display;
        }
    }
    catch (std::exception &e)
    {
        LOG_FATAL("Memory allocation failure, Display");
        throw ERR_DISPLAY_INIT;
    }

    return m_pDisp;
}

Display::~Display()
{
    exit_handler();
}

PRIVATE VOID exit_handler()
{
    /* Some signals may be delivered twice during exit() execution,
     * and we must prevent all this from beeing done twice */
    static BOOL alreadyExited = FALSE;
    if (alreadyExited)
    {
        return;
    }

    alreadyExited = TRUE;
    Display::getInstance()->shutdown();
}

VOID Display::shutdown()
{
    static BOOL shutdownDone = FALSE;
    if (shutdownDone)
    {
        return;
    }
    shutdownDone = TRUE;
    switch (m_dispTgt)
    {
    case DISP_TARGET_SCREEN:
    {
        endwin();
        break;
    }
    case DISP_TARGET_FILE:
    {
        dispFile();
        fout.close();
        break;
    }
    case DISP_TARGET_NONE:
    case DISP_TARGET_MAX:
    {
        break;
    }
    }
    exit(0);
}

PRIVATE VOID dump_stats()
{
    Display::getInstance()->dumpStats();
}

void Display::dumpStats()
{
    if (m_dispTgt == DISP_TARGET_FILE)
    {
        dispFile();
    }
}

VOID Display::init()
{
    struct sigaction action_quit;
    struct sigaction action_dump_stats;

    m_dispTgt   = Config::getInstance()->getDisplayTarget();
    m_dispTgtFile = Config::getInstance()->getDisplayTargetFile();
    m_dispIntvl = Config::getInstance()->getDisplayRefreshTimer();
    m_summaryOnly = Config::getInstance()->getDisplaySummary();
    m_pStats    = Stats::getInstance();
    getTimeStr(m_timeStr);
    m_startTime  = getMilliSeconds() / 1000;
    m_localPort  = Config::getInstance()->getLocalGtpcPort();
    m_remPort    = Config::getInstance()->getRemoteGtpcPort();
    m_ifTypeStr = Config::getInstance()->getIfTypeStr();
    STRCPY(m_remIpAddrStr, (Config::getInstance()->getRemIpAddrStr()).c_str());
    STRCPY(
        m_localIpAddrStr, (Config::getInstance()->getLocalIpAddrStr()).c_str());

    m_procSeq = &(Scenario::getInstance()->m_procSeq);

    switch (m_dispTgt)
    {
    case DISP_TARGET_SCREEN:
    {
        initscr();
        noecho();
        break;
    }
    case DISP_TARGET_FILE:
    {
        fout.open(m_dispTgtFile, ios::out | ios::trunc);
        fout << "Test\n";
        break;
    }
    default:
    {
       throw GsimError("Unimplemented display target type");
    }
    }

    /* Map exit handlers to curses reset procedure */
    memset(&action_dump_stats, 0, sizeof(action_dump_stats));
    (*(void **)(&(action_dump_stats.sa_handler))) = (VOID *)dump_stats;
    sigaction(SIGHUP, &action_dump_stats, NULL);
    memset(&action_quit, 0, sizeof(action_quit));
    (*(void **)(&(action_quit.sa_handler))) = (VOID *)exit_handler;
    sigaction(SIGTERM, &action_quit, NULL);
    sigaction(SIGINT, &action_quit, NULL);
    sigaction(SIGKILL, &action_quit, NULL);

    CLEAR_SCREEN();
}

RETVAL Display::run(VOID *arg)
{
    LOG_ENTERFN();

    m_lastRunTime = getMilliSeconds();
    displayToTarget();
    pause();

    LOG_EXITFN(ROK);
}

VOID Display::printJob(Job *job)
{
    switch (job->type())
    {
    case JOB_TYPE_SEND:
    {
        fprintf(stdout, "%s  ", job->m_msgName);
        fprintf(stdout, "\t--->");
        fprintf(stdout, " \t%9d", job->m_numSnd);
        fprintf(stdout, "%9d", job->m_numSndRetrans);
        fprintf(stdout, " %9d", job->m_numTimeOut);
        fprintf(stdout, ENDLINE);
        break;
    }
    case JOB_TYPE_RECV:
    {
        fprintf(stdout, "%s  ", job->m_msgName);
        fprintf(stdout, " \t<---");
        fprintf(stdout, "\t%9d", job->m_numRcv);
        fprintf(stdout, "%9d", job->m_numRcvRetrans);
        fprintf(stdout, "                  %9d", job->m_numUnexp);
        fprintf(stdout, ENDLINE);
        break;
    }
    case JOB_TYPE_WAIT:
    {
        fprintf(stdout, "[Wait %5d]\r\n", (S32)job->wait());
        fprintf(stdout, ENDLINE);
        break;
    }
    default:
    {
        break;
    }
    }
}

VOID Display::disp()
{
    static BOOL firTime = TRUE;

    CLEAR_SCREEN();

    if (firTime)
    {
        firTime = FALSE;
        fprintf(stdout,
            "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n"
            "\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    }

    PRINT_SEPERATOR();
    fprintf(stdout, "Start: %s  ", m_timeStr);
    Time_t runTime = (getMilliSeconds() / 1000) - m_startTime;
    fprintf(stdout, "Run-Time: %us   ", (U32)runTime);
    fprintf(stdout, "  Interface  : %s\r\n", m_ifTypeStr.c_str());
    fprintf(stdout, "Local-Host: %s:%d ", m_localIpAddrStr, m_localPort);
    if (STRLEN(m_remIpAddrStr) > 0)
    {
        fprintf(stdout, "\t\t\t  Remote-Host: %s:%d \r\n", m_remIpAddrStr,
            m_remPort);
    }
    else
    {
        fprintf(stdout, "\r\n");
    }

    PRINT_SEPERATOR();

    Counter ssnCreated = getStats(GSIM_STAT_NUM_SESSIONS_CREATED);
    Counter ssnSucc    = getStats(GSIM_STAT_NUM_SESSIONS_SUCC);
    Counter ssnFail    = getStats(GSIM_STAT_NUM_SESSIONS_FAIL);
    Counter deadCalls  = getStats(GSIM_STAT_NUM_DEADCALLS);
    fprintf(stdout, "Total-Sessions:    %u\r\n", ssnCreated);
    fprintf(stdout, "Session-Completed: %u\r\n", ssnSucc);
    fprintf(stdout, "Session-Aborted:   %u\r\n", ssnFail);
    fprintf(stdout, "Dead-Calls:        %u\r\n", deadCalls);

    PRINT_SEPERATOR();
    if (!m_summaryOnly)
    {
        fprintf(stdout,
            "                                 "
            "Messages  Retrans   Timeout   Unexpected-Msg\r\n");

        for (U32 i = 0; i < m_procSeq->size(); i++)
        {
            Procedure *proc = m_procSeq->at(i);

            switch (proc->type())
            {
            case PROC_TYPE_WAIT:
            {
                printJob(proc->m_wait);
                break;
            }
            case PROC_TYPE_REQ_RSP:
            {
                printJob(proc->m_initial);
                printJob(proc->m_trigMsg);
                break;
            }
            case PROC_TYPE_REQ_TRIG_REP:
            {
                printJob(proc->m_initial);
                printJob(proc->m_trigMsg);
                printJob(proc->m_trigReply);
                break;
            }
            default:
            {
                break;
            }
            }
        }
    }

    PRINT_BLANK_LINE();
    if (KB_KEY_PAUSE_TRAFFIC == Keyboard::key)
    {
        PRINT_END_SEPERATOR_RESUME();
    }
    else
    {
        PRINT_END_SEPERATOR_PAUSE();
    }

    fflush(stdout);
}

VOID Display::printJobFile(Job *job)
{
    switch (job->type())
    {
    case JOB_TYPE_SEND:
    {
        fout << job->m_msgName << ":"
             << " Sent:" << job->m_numSnd
             << " Retrans:" << job->m_numSndRetrans
             << " Timeout:" << job->m_numTimeOut
             << std::endl;
        break;
    }
    case JOB_TYPE_RECV:
    {
        fout << job->m_msgName << ":"
             << " Recv:" << job->m_numRcv
             << " Retrans:" << job->m_numRcvRetrans
             << " Unexpected:" << job->m_numUnexp
	     << std::endl;
        break;
    }
    case JOB_TYPE_WAIT:
    default:
    {
        break;
    }
    }
}

VOID Display::dispFile()
{
    Time_t runTime = (getMilliSeconds() / 1000) - m_startTime;
    fout << "Run-Time:" << (U32)runTime
         << " Interface:" << m_ifTypeStr.c_str()
         << " Local:" << m_localIpAddrStr << "/" << m_localPort
         << " Remote:" << m_remIpAddrStr << "/" << m_remPort
         << std::endl;

    Counter ssnCreated = getStats(GSIM_STAT_NUM_SESSIONS_CREATED);
    Counter ssnSucc    = getStats(GSIM_STAT_NUM_SESSIONS_SUCC);
    Counter ssnFail    = getStats(GSIM_STAT_NUM_SESSIONS_FAIL);
    Counter deadCalls  = getStats(GSIM_STAT_NUM_DEADCALLS);
    fout << "Sessions:" << ssnCreated << " Completed:" << ssnSucc
         << " Aborted:" << ssnFail << " Dead-Calls:" << deadCalls
	 << std::endl;

    if (!m_summaryOnly)
    {
        for (U32 i = 0; i < m_procSeq->size(); i++)
        {
            Procedure *proc = m_procSeq->at(i);

            switch (proc->type())
            {
            case PROC_TYPE_WAIT:
            {
                printJobFile(proc->m_wait);
                break;
            }
            case PROC_TYPE_REQ_RSP:
            {
                printJobFile(proc->m_initial);
                printJobFile(proc->m_trigMsg);
                break;
            }
            case PROC_TYPE_REQ_TRIG_REP:
            {
                printJobFile(proc->m_initial);
                printJobFile(proc->m_trigMsg);
                printJobFile(proc->m_trigReply);
                break;
            }
            default:
            {
                break;
            }
            }
        }
    }

    fout.flush();
}

Counter Display::getStats(GtpStat_t type)
{
    return m_pStats->getStats(type);
}

VOID Display::displayToTarget()
{
    switch(m_dispTgt)
    {
    case DISP_TARGET_SCREEN:
    {
         disp();
         break;
    }
    case DISP_TARGET_FILE:
    {
         dispFile();
         break;
    }
    default:
    {
       throw GsimError("Unimplemented display target type");
    }
    }
}

VOID Display::displayStats()
{
    Display::getInstance()->displayToTarget();
}
