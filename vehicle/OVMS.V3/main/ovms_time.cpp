/*
;    Project:       Open Vehicle Monitor System
;    Date:          14th March 2017
;
;    Changes:
;    1.0  Initial release
;
;    (C) 2011       Michael Stegen / Stegen Electronics
;    (C) 2011-2017  Mark Webb-Johnson
;    (C) 2011        Sonny Chen @ EPRO/DX
;
; Permission is hereby granted, free of charge, to any person obtaining a copy
; of this software and associated documentation files (the "Software"), to deal
; in the Software without restriction, including without limitation the rights
; to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
; copies of the Software, and to permit persons to whom the Software is
; furnished to do so, subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in
; all copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
; FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
; AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
; LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
; OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
; THE SOFTWARE.
*/

#include "ovms_log.h"
static const char *TAG = "time";

#include "ovms.h"
#include "ovms_time.h"
#include "ovms_command.h"

OvmsTime MyTime __attribute__ ((init_priority (1100)));

void time_status(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  time_t rawtime;
  time ( &rawtime );
  struct tm* tmu = gmtime(&rawtime);
  struct tm* tml = localtime(&rawtime);

  writer->printf("UTC Time:   %s", asctime(tmu));
  writer->printf("Local Time: %s", asctime(tml));
  writer->printf("Provider:   %s\n\n",(MyTime.m_current)?MyTime.m_current->m_provider:"None");

  writer->printf("PROVIDER             STRATUM  UPDATE TIME\n");
  for (OvmsTimeProviderMap_t::iterator itc=MyTime.m_providers.begin(); itc!=MyTime.m_providers.end(); itc++)
    {
    OvmsTimeProvider* tp = itc->second;
    time_t tim = tp->m_time + (monotonictime-tp->m_lastreport);
    struct tm* tml = gmtime(&tim);
    writer->printf("%s%-20.20s%7d%8d %s",
      (tp==MyTime.m_current)?"*":" ",
      tp->m_provider, tp->m_stratum, monotonictime-tp->m_lastreport, asctime(tml));
    }
  }

void time_set(int verbosity, OvmsWriter* writer, OvmsCommand* cmd, int argc, const char* const* argv)
  {
  MyTime.Set(TAG, 15, true, atol(argv[0]));
  writer->puts("Time set (at stratum 15)");
  }

OvmsTimeProvider::OvmsTimeProvider(const char* provider, int stratum, time_t tim)
  {
  m_provider = provider;
  m_time = tim;
  m_lastreport = monotonictime;
  m_stratum = stratum;
  }

OvmsTimeProvider::~OvmsTimeProvider()
  {
  }

void OvmsTimeProvider::Set(int stratum, time_t tim)
  {
  m_stratum = stratum;
  m_time = tim;
  m_lastreport = monotonictime;
  }

OvmsTime::OvmsTime()
  {
  m_current = NULL;
  m_tz.tz_minuteswest = 0;
  m_tz.tz_dsttime = 0;

  // Register our commands
  OvmsCommand* cmd_time = MyCommandApp.RegisterCommand("time","TIME framework",time_status, "", 0, 1);
  cmd_time->RegisterCommand("status","Show time status",time_status,"", 0, 0, false);
  cmd_time->RegisterCommand("set","Set current UTC time",time_set,"<time>", 1, 1, false);
  }

OvmsTime::~OvmsTime()
  {
  }

void OvmsTime::Elect()
  {
  int best=255;
  OvmsTimeProvider* c = NULL;

  for (OvmsTimeProviderMap_t::iterator itc=MyTime.m_providers.begin(); itc!=MyTime.m_providers.end(); itc++)
    {
    if (itc->second->m_stratum < best)
      {
      best = itc->second->m_stratum;
      c = itc->second;
      }
    }

  MyTime.m_current = c;
  }

void OvmsTime::Set(const char* provider, int stratum, bool trusted, time_t tim, suseconds_t timu)
  {
  if (!trusted) stratum=16;

  ESP_LOGD(TAG, "%s (stratum %d trusted %d) provides time %lu", provider, stratum, trusted, tim);

  auto k = m_providers.find(provider);
  if (k == m_providers.end())
    {
    m_providers[provider] = new OvmsTimeProvider(provider,stratum,tim);
    Elect();
    k = m_providers.find(provider);
    }

  if (k != m_providers.end())
    {
    k->second->Set(stratum,tim);
    if (k->second == m_current)
      {
      time_t rawtime;
      time ( &rawtime );
      if (tim != rawtime)
        {
        struct timeval tv;
        tv.tv_sec = tim;
        tv.tv_usec = timu;
        settimeofday(&tv, &m_tz);
        }
      }
    }
  }

