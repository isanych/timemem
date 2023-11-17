/* TimeMem.c - Windows port of Unix time utility */

#include <Windows.h>
#include <Psapi.h>
#include <stdio.h>
#include <tchar.h>

/* Displays usage help for this program. */
static void usage()
{
  _tprintf(_T("Usage: TimeMem command [args...]\n"));
}

/* Converts FILETIME to ULONGLONG. */
static ULONGLONG ConvertFileTime(const FILETIME *t)
{
  ULARGE_INTEGER i;
  CopyMemory(&i, t, sizeof(ULARGE_INTEGER));
  return i.QuadPart;
}

/* Displays information about a process. */
static int info(HANDLE hProcess)
{
  DWORD dwExitCode;
  FILETIME ftCreation, ftExit, ftKernel, ftUser;
  double tElapsed, tKernel, tUser;
  PROCESS_MEMORY_COUNTERS pmc = { sizeof(PROCESS_MEMORY_COUNTERS) };

  /* Exit code */
  if (!GetExitCodeProcess(hProcess, &dwExitCode))
  {
    return 1;
  }

  /* CPU info */
  if (!GetProcessTimes(hProcess, &ftCreation, &ftExit, &ftKernel, &ftUser))
  {
    return 1;
  }
  tElapsed = 1.0e-7 * (ConvertFileTime(&ftExit) - ConvertFileTime(&ftCreation));
  tKernel = 1.0e-7 * ConvertFileTime(&ftKernel);
  tUser = 1.0e-7 * ConvertFileTime(&ftUser);

  /* Memory info */
  // Print information about the memory usage of the process.
  if (!GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
  {
    return 1;
  }

  /* Display info. */
  _tprintf(_T("Exit code      : %u\n"), dwExitCode);

  _tprintf(_T("Elapsed time   : %.2lf\n"), tElapsed);
  _tprintf(_T("Kernel time    : %.2lf (%.1lf%%)\n"), tKernel, 100.0*tKernel/tElapsed);
  _tprintf(_T("User time      : %.2lf (%.1lf%%)\n"), tUser, 100.0*tUser/tElapsed);

  _tprintf(_T("page fault #   : %u\n"), pmc.PageFaultCount);
  _tprintf(_T("Working set    : %zd KB\n"), pmc.PeakWorkingSetSize/1024);
  _tprintf(_T("Paged pool     : %zd KB\n"), pmc.QuotaPeakPagedPoolUsage/1024);
  _tprintf(_T("Non-paged pool : %zd KB\n"), pmc.QuotaPeakNonPagedPoolUsage/1024);
  _tprintf(_T("Page file size : %zd KB\n"), pmc.PeakPagefileUsage/1024);

  return 0;
}

/* Todo:
 * - mimic linux time utility interface; e.g. see http://linux.die.net/man/1/time
 * - build under 64-bit
 * - display detailed error message
 */

TCHAR buf[500];
LPTSTR dst = buf;

LPTSTR addMem(LPCTSTR src, int len)
{
  CopyMemory(dst, src, len * sizeof(TCHAR));
  dst += len;
  return dst;
}

LPTSTR addString(LPCTSTR src)
{
  return addMem(src, lstrlen(src) + 1);
}

void pre()
{
  STARTUPINFO si = { sizeof(STARTUPINFO) };
  PROCESS_INFORMATION pi;
  if (!CreateProcess(NULL, TEXT("C:/opt/apps/git/current/usr/bin/ldd.exe C:/b/all/debug/Helix-QAC-2023.3/components/dataflow-1.3.0/bin/dataflow.exe"), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
  {
    _tprintf(_T("Error: Cannot create process.\n"));
    return;
  }

  /* Wait for the process to finish. */
  if (WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0)
  {
    _tprintf(_T("Error: Cannot wait for process.\n"));
    return;
  }

  /* Close process handles. */
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
}

int _tmain(
#ifndef NDEBUG
  int argc, _TCHAR* argv[]
#endif
)
{
  pre();
  STARTUPINFO si = { sizeof(STARTUPINFO) };
  PROCESS_INFORMATION pi;

  /* Read the command line. */
  LPCTSTR szCmdLine = GetCommandLine();
  int cmdSize = lstrlen(szCmdLine);
  if (cmdSize + 4 >= sizeof(buf) / sizeof(TCHAR))
  {
    _tprintf(_T("Error: Command line too long.\n"));
    return 1;
  }

  LPCTSTR del = szCmdLine;

  /* Strip the first token from the command line. */
  if (szCmdLine[0] == '"')
  {
    /* The first token is double-quoted. Note that we don't need to
      * worry about escaped quote, because a quote is not a valid
      * path name under Windows.
      */
    ++del;
    LPCTSTR p = del;
    while (*p && *p != '"')
    {
      if (*p == '/' || *p == '\\')
      {
        del = p + 1;
      }
      ++p;
    }
  }
  else
  {
    /* The first token is deliminated by a space or tab.
      * See "Parsing C++ Command Line Arguments" below:
      * http://msdn.microsoft.com/en-us/library/windows/desktop/17w5ykft(v=vs.85).aspx
      */
    LPCTSTR p = del;
    while (*p && *p != ' ' && *p != '\t')
    {
      if (*p == '/' || *p == '\\')
      {
        del = p + 1;
      }
      ++p;
    }
  }

  if (del > szCmdLine)
  {
    addMem(szCmdLine, del - szCmdLine);
  }
  addMem(TEXT("bin\\"), 4);
  addString(del);

  /* Create the process. */
  if (!CreateProcess(NULL, buf, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
  {
    _tprintf(_T("Error: Cannot create process.\n"));
    return 1;
  }

  /* Wait for the process to finish. */
  if (WaitForSingleObject(pi.hProcess, INFINITE) != WAIT_OBJECT_0)
  {
    _tprintf(_T("Error: Cannot wait for process.\n"));
    return 1;
  }

  /* Display process statistics. */
  int ret = info(pi.hProcess);

  /* Close process handles. */
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);

  return ret;
}
