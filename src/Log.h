#ifndef _LOG_FILE_H_
#define _LOG_FILE_H_

#ifndef LOG_DEFAULT
#ifdef _DEBUG
#define LOG_DEFAULT LL_DEBUG
#else 
#define LOG_DEFAULT LL_INFO
#endif
#endif

class CLog
{
public:
    enum LOG_LEVEL
    {
        LL_ERROR = 0,
        LL_WARNING,
        LL_INFO,
        LL_DEBUG,
        LL_ALL
    };
    void Log(LOG_LEVEL level, LPCTSTR szFile, int nLine, LPCTSTR szFunc, LPCTSTR szFormat, ...)
    { 
        if (level > mLevel || !OpenFile()) return;

        va_list args;
        va_start(args, szFormat);
        int nSize = _vsctprintf(szFormat, args);
     
        SYSTEMTIME tm;
        ::GetLocalTime(&tm);
            
        ::EnterCriticalSection(&m_szLock);

        if (nSize + MAX_PATH > m_nSize)
        {
            ::HeapFree(::GetProcessHeap(), 0, m_pBuffer);
            m_nSize = nSize + MAX_PATH * 2;
            m_pBuffer = (LPTSTR)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, m_nSize * sizeof(TCHAR));
        }

        const static TCHAR *arrLevel[] = { TEXT("error"), TEXT("warning"), TEXT("info"), TEXT("debug"), TEXT("none") };
        int nLen = ::wnsprintf(m_pBuffer, m_nSize, TEXT("%.2d:%.2d:%.2d:%.3d [%d] %s:%s:%d(%s) "),
            tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds, ::GetCurrentThreadId(), arrLevel[level], szFile, nLine, szFunc);
        nLen += ::wvnsprintf(m_pBuffer + nLen, m_nSize - nLen, szFormat, args);
        ::StrCat(m_pBuffer, TEXT("\r\n"));

        DWORD cbSize = (nLen + 2) * sizeof(TCHAR);
        ::WriteFile(m_hFile, m_pBuffer, cbSize, &cbSize, NULL);

        ::LeaveCriticalSection(&m_szLock);
        va_end(args);
    }
    CLog() : mLevel(LOG_DEFAULT), m_hFile(INVALID_HANDLE_VALUE), m_nSize(MAX_PATH * 2)
    {
        // 初始化锁
        ::InitializeCriticalSection(&m_szLock);
        // 申请内存
        m_pBuffer = (LPTSTR)::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, m_nSize * sizeof(TCHAR));
    }
    virtual ~CLog()
    {
        if (NULL != m_pBuffer)
        {
            ::HeapFree(::GetProcessHeap(), 0, m_pBuffer);
        }
        if (INVALID_HANDLE_VALUE != m_hFile)
        {
            ::FlushFileBuffers(m_hFile);
            ::CloseHandle(m_hFile);
        }
        ::DeleteCriticalSection(&m_szLock);
    }
protected:
    bool OpenFile() //打开文件，指针到文件尾
    {
        if (INVALID_HANDLE_VALUE != m_hFile) return true;

        TCHAR szPath[MAX_PATH];
        ::GetModuleFileName(NULL, szPath, _countof(szPath));
        ::PathRemoveExtension(szPath);
        int nSize = _tcslen(szPath);

        SYSTEMTIME tm;
        ::GetLocalTime(&tm);
        ::wnsprintf(szPath + nSize, _countof(szPath) - nSize, TEXT("_%.2d%.2d%.2d[%d].log"),
            tm.wYear, tm.wMonth, tm.wDay, ::GetCurrentProcessId());

        m_hFile = ::CreateFile(szPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (INVALID_HANDLE_VALUE == m_hFile) return false;
        ::SetFilePointer(m_hFile, 0, NULL, FILE_END);
        return true;
    }
private:
    CLog(const CLog&);
    CLog&operator=(const CLog&);

protected:
    LOG_LEVEL mLevel;
    HANDLE m_hFile;
    LPTSTR m_pBuffer;
    int m_nSize;
    CRITICAL_SECTION m_szLock;
};

__declspec(selectany) CLog _gLog;

#define __WIDEN(str)    L##str
#define _WIDEN(str)     __WIDEN(str)

#ifdef _UNICODE
#define __TFILE__ _WIDEN(__FILE__)
#define __TFUNCTION__ _WIDEN(__FUNCTION__)
#else
#define __TFILE__ __FILE__
#define __TFUNCTION__ __FUNCTION__
#endif

#define LOG_ERROR(format, ...)  _gLog.Log(CLog::LL_ERROR, __TFILE__, __LINE__, __TFUNCTION__, TEXT(format), __VA_ARGS__)
#define LOG_WARNING(format, ...)  _gLog.Log(CLog::LL_WARNING, __TFILE__, __LINE__, __TFUNCTION__, TEXT(format), __VA_ARGS__)
#define LOG_INFO(format, ...)  _gLog.Log(CLog::LL_INFO, __TFILE__, __LINE__, __TFUNCTION__, TEXT(format), __VA_ARGS__)
#define LOG_DEBUG(format, ...)  _gLog.Log(CLog::LL_DEBUG, __TFILE__, __LINE__, __TFUNCTION__, TEXT(format), __VA_ARGS__)


#define HR_CHECK(_hr_) hr = _hr_; if (FAILED(hr)) { LOG_ERROR("0x%.8x", hr); goto exit; }
#define BOOL_CHECK(_hr_) if (!(_hr_)) { hr = HRESULT_FROM_WIN32(::GetLastError()); LOG_ERROR("0x%.8x", hr); goto exit; }
#define HANDLE_CHECK(_handle_) BOOL_CHECK(INVALID_HANDLE_VALUE != (_handle_))

#endif // _LOG_FILE_H_
