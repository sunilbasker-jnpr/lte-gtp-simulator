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

#ifndef _GTP_CFG_HPP_
#define _GTP_CFG_HPP_

#include <iostream>
#include <string>

#include <cxxopts.hpp>

using namespace std;

#define DFLT_LOCAL_IP_ADDR "127.0.0.1"
#define DFLT_GTPC_PORT 2123
#define DFLT_T3_TIMER 3000 // milli seconds
#define DFLT_N3_REQUESTS 3
#define DFLT_DISP_REFRESH_TIMER 2000  // milli seconds
#define DFLT_SESSION_RATE_PERIOD 1000 // 1000 milli seconds
#define DFLT_SESSION_RATE 1           // 1 session per rate period
#define DFLT_MIN_SESSION_RATE 1       // 1 session per rate period
#define DFLT_MAX_SESSION_RATE 1000000 // 1 session per rate period
#define DFLT_TRACE_MSG_FILE_NAME_LEN 64
#define DFLT_DEAD_CALL_WAIT 20000 // milli seconds
#define DFLT_TIMEOUT 0

typedef enum {
    DISP_TARGET_NONE,
    DISP_TARGET_SCREEN,
    DISP_TARGET_FILE,
    DISP_TARGET_MAX
} DisplayTargetEn;

// Config will be a singleton object, accessed using getInstance
class Config
{
public:
    static Config *getInstance();
    ~Config();

    // service routines
    // these routines are made public, for setting configuration
    // property from a GUI
    VOID setNoOfCalls(U32 n);
    VOID setLocalIpAddr(string ip);
    VOID setRemoteIpAddr(string ip);
    VOID setLocalGtpcPort(U16 port);
    VOID setLocalGtpcSendPort(U16 port);
    VOID setRemoteGtpcPort(U16 port);
    VOID setT3TimerSeconds(U32 val);
    VOID setN3Requests(U32 n);
    VOID setDisplayRefreshTimer(U32 n);
    VOID setDisplayTarget(DisplayTargetEn target);
    VOID setDisplaySummary(BOOL val);
    VOID setErrorFile(string filename) throw(ErrCodeEn);
    VOID setScenarioFile(std::string filename) throw(ErrCodeEn);
    VOID setLogFile(string filename) throw(ErrCodeEn);
    VOID setDisplayTargetFile(string filename);
    VOID setCallRate(U32 n);
    VOID setRatePeriod(U32 n);
    VOID setLogLevel(std::uint32_t logLvl);
    VOID setTraceMsg(BOOL);
    VOID setTraceMsgFile(string);
    VOID setPidFile(string);
    VOID setTimeout(std::uint32_t timeout);

    IpAddr        getRemoteIpAddr();
    string        getRemIpAddrStr();
    const IpAddr *getLocalIpAddr();
    U32           getN3Requests();
    U16           getRemoteGtpcPort();
    string        getLocalIpAddrStr();
    U16           getLocalGtpcPort();
    U16           getLocalGtpcSendPort();
    U32           getDisplayRefreshTimer();
    DisplayTargetEn getDisplayTarget();
    string        getDisplayTargetFile();
    BOOL          getDisplaySummary();
    U32           getT3Timer();
    U32           getScnRunInterval();
    const S8 *    getScnFile();
    U32           getCallRate();
    U32           getLogLevel();
    U32           getTimeout();
    VOID          setConfig(cxxopts::ParseResult options);
    Time_t        getSessionRatePeriod();
    EpcNodeType_t getNodeType();
    U8            getIfType();
    Counter       getNumSessions();
    string        getLogFile();
    BOOL          getTraceMsg();
    string        getTraceMsgFile();
    string        getImsi();
    VOID          setImsi(S8 *pVal, U32 len);
    VOID          incrRate(U32 value);
    VOID          decrRate(U32 value);
    Time_t        getDeadCallWait();
    void          setNodeType(std::string node);
    std::string   getNodeTypeStr();
    std::string   getIfTypeStr();
    std::string   getPidFile();

    VOID          writePidFile();
private:
    Config();
    RETVAL saveIp(string &ipStr, IpAddr *pIp);
    void   setIfType(std::string ifType);

    U32             m_ssnRate; // no.of calls per sec
    IpAddr          locIpAddr;
    IpAddr          remIpAddr;
    U16             locGtpcPort;
    U16             remGtpcPort;
    U32             t3Timer;
    U32             n3Req;
    U32             dispTimer;      // display refresh rate
    DisplayTargetEn dispTarget;     // displa on screen or file
    string          errFile;        // error log file
    string          scnFile;        // xml scenario file path
    string          m_logFile;      // log file path
    string          dispTargetFile; // display redirected to this file
    BOOL            m_dispSummary;
    U32             m_scnRunIntvl;
    Time_t          m_ssnRatePeriod;
    std::uint32_t   m_logLevel;
    std::uint32_t   m_timeout;
    EpcNodeType_t   m_nodeType;
    U8              m_ifType;
    std::string     m_ifTypeStr;
    Counter         m_maxSessions;
    string          m_remIpAddrStr;
    string          m_localIpAddrStr;
    BOOL            m_traceMsg;
    string          m_traceMsgFile;
    string          m_imsiStr;
    Time_t          m_deadCallWait;
    string          m_nodeTypStr;
    string          m_pidFile;
};

#endif
