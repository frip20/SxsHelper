#include "StdAfx.h"

const LPTSTR PathRel_Packages = TEXT("Servicing\\Packages");

int CALLBACK CheckPath(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
    switch (uMsg)
    {
    case BFFM_INITIALIZED:
        if (NULL != lpData)
        {
            ::SendMessage(hWnd, BFFM_SETSELECTION, TRUE, lpData);
        }
        break;
    case BFFM_SELCHANGED:
        LPITEMIDLIST pidlSelected = (LPITEMIDLIST)lParam;
        TCHAR szPath[MAX_PATH] = { 0 };
        if (::SHGetPathFromIDList(pidlSelected, szPath))
        {
            ::PathCombine(szPath, szPath, PathRel_Packages);
            ::SendMessage(hWnd, BFFM_ENABLEOK, NULL, ::PathIsDirectory(szPath));
        }
        break;
    }
    return 0;
}

BOOL SelectFolder(LPTSTR szFolder)
{
    BOOL bRet = FALSE;
    CAtlString szTitle;
    szTitle.LoadString(IDS_SOURCE);

    BROWSEINFO bi = { 0 };
    bi.hwndOwner = HWND_DESKTOP;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_DONTGOBELOWDOMAIN | BIF_NONEWFOLDERBUTTON;
    bi.lParam = (LPARAM)szFolder;
    bi.lpfn = CheckPath;
    bi.lpszTitle = szTitle;

    LPITEMIDLIST pidlSelected = ::SHBrowseForFolder(&bi);
    if (NULL != pidlSelected)
    {
        bRet = ::SHGetPathFromIDList(pidlSelected, szFolder);
        CoTaskMemFree(pidlSelected);
    }
    return bRet;
}

int APIENTRY _tWinMain(HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPTSTR    lpCmdLine,
    int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    TCHAR szPath[MAX_PATH] = { 0 };
    ::GetWindowsDirectory(szPath, _countof(szPath));
#ifndef _DEBUG
    if (!SelectFolder(szPath)) return FALSE;
#endif
    HRESULT hr = ::CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    ATLASSERT(SUCCEEDED(hr));

    ::DefWindowProc(NULL, 0, 0, 0L);

    hr = _Module.Init(NULL, hInstance);
    ATLASSERT(SUCCEEDED(hr));

    ::PathCombine(szPath, szPath, PathRel_Packages);
    MSG msg;
    {
        CMainDlg dlgMain(szPath);
        RECT rcWnd = { 0, 0, 600, 500 };
        HWND hWnd = dlgMain.Create(HWND_DESKTOP, rcWnd);

        dlgMain.ShowWindow(nCmdShow);
        HACCEL hAccMain = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_MAIN));

        // ����Ϣѭ��:
        while (::GetMessage(&msg, NULL, 0, 0))
        {
            if (!dlgMain.FindMsg(&msg) && !TranslateAccelerator(hWnd, hAccMain, &msg))
            {
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
        }
    }

    _Module.Term();
    ::CoUninitialize();
    return (int)msg.wParam;
}