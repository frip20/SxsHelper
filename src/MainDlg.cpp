#include "StdAfx.h"

UINT CFindDlg::WM_FINDMESSAGE = 0;

CMainDlg::CMainDlg(LPTSTR szPath) :
m_hThread(NULL), 
m_hMenu(NULL),
mTree(WC_TREEVIEW, this, 1),
mFilter(WC_EDIT, this, 2)
{
    ZeroMemory(&m_szExport, sizeof(m_szExport));
    mRoot = new CAssemblyNode();
    mRoot->name = szPath;
}

CMainDlg::~CMainDlg()
{
    
}

LPCTSTR CMainDlg::GetWndCaption()
{
    static CAtlString szTitle;
    if (szTitle.IsEmpty()) szTitle.LoadString(IDR_MAIN);
    return szTitle;
}

LRESULT CMainDlg::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    mTree.Create(m_hWnd, NULL);

    // ���� ɸѡ�� �ؼ�
    CAtlString szFilterHint;
    szFilterHint.LoadString(IDS_FILTERHINT);
    mFilter.Create(mTree, NULL);
    mFilter.SendMessage(EM_SETCUEBANNER, TRUE, (LPARAM)(LPCTSTR)szFilterHint);
    mFilter.SendMessage(EM_LIMITTEXT, MAX_PATH);

    // ���ô���ͼ��
    HICON hIcon = ::LoadIcon(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAIN));
    SetIcon(hIcon);
    CenterWindow();

    // ���ز˵���Դ
    m_hMenu = ::LoadMenu(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAIN));

    // ����ϵͳ�˵�
    HMENU hSysMenu = GetSystemMenu(FALSE);
    CAtlString szAbout;
    szAbout.LoadString(IDS_ABOUT);
    InsertMenu(hSysMenu, 0, MF_SEPARATOR, IDM_ABOUT, NULL);
    InsertMenu(hSysMenu, 0, MF_STRING, IDM_ABOUT, szAbout);

    // ע����ҶԻ����¼�
    CFindDlg::WM_FINDMESSAGE = ::RegisterWindowMessage(FINDMSGSTRING);

    // �ӻ�ȡĬ�ϱ���·��
    if (::SHGetSpecialFolderPath(m_hWnd, m_szExport, CSIDL_MYDOCUMENTS, TRUE))
    {
        ::PathCombine(m_szExport, m_szExport, TEXT("Remove.txt"));
    }

    // ����ɨ���߳�
    m_hThread = ::CreateThread(NULL, 0, CMainDlg::ThreadScan, this, 0, NULL);
    return 0;
}

LRESULT CMainDlg::OnDestroy(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    // �ͷŲ˵���Դ
    if (NULL != m_hMenu) ::DestroyMenu(m_hMenu);

    ::PostQuitMessage(LOWORD(wParam));
    return TRUE;
}

LRESULT CMainDlg::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    mTree.ResizeClient(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
    return mFilter.SetWindowPos(HWND_TOP, GET_X_LPARAM(lParam) - 150, 0, 150, 20, 0);
}

LRESULT CMainDlg::OnSysCommand(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    switch (LOWORD(wParam))
    {
    case IDM_ABOUT:
    {
        ATL::CSimpleDialog<IDD_ABOUT> dlgAbout;
        return dlgAbout.DoModal(m_hWnd);
    }
    case SC_CLOSE:
        bHandled = IsWorking();
        break;
    }
    bHandled = FALSE;
    return S_OK;
}

/**
 * ��������߼�
 */
LRESULT CMainDlg::OnFind(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/)
{
    LPFINDREPLACE pfr = (LPFINDREPLACE)lParam;
    if (pfr->Flags & FR_DIALOGTERM)
    {
        mFind.m_hWnd = NULL;
        pfr->Flags &= ~FR_DIALOGTERM;
        return TRUE;
    }

    PTSTR(WINAPI *fnStr)(PCTSTR, PCTSTR) = (pfr->Flags & FR_MATCHCASE) ? ::StrStr : ::StrStrI;
    ULONG(WINAPI *fnInc)(LPLONG) = (pfr->Flags & FR_DOWN) ? CComSingleThreadModel::Increment : CComSingleThreadModel::Decrement;

    LONG index = (LONG)pfr->lCustData;
    if (index >= 0 && index < mMap.GetSize())
    {
        TreeView_SetItemState(mTree, mMap.GetValueAt(index)->Parent.GetValueAt(0), 0, TVIS_BOLD);
    }
    else
    {
        index = 0;
    }

    while (fnInc(&index) < (ULONG)mMap.GetSize() && index >= 0)
    {
        HTREEITEM hItem = mMap.GetValueAt(index)->Parent.GetValueAt(0);

        if (fnStr(mMap.GetKeyAt(index), pfr->lpstrFindWhat) != NULL)
        { 
            pfr->lCustData = index;
            TreeView_SetItemState(mTree, hItem, TVIS_BOLD, TVIS_BOLD);
            return TreeView_SelectItem(mTree, hItem);
        }

        TreeView_SetItemState(mTree, hItem, 0, TVIS_BOLD);
        TreeView_Expand(mTree, hItem, TVE_COLLAPSE);
    }

    // ��ʾû���ҵ�
    CAtlString szEnd;
    szEnd.Format(IDS_FINDEND, pfr->lpstrFindWhat);
    return MessageBox(szEnd, CMainDlg::GetWndCaption(), MB_ICONWARNING);
}

LRESULT CMainDlg::OnFindNext(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    LPFINDREPLACE pfr = mFind.GetNotifier();
    if (pfr->lpstrFindWhat[0] == _T('\0')) return FALSE;

    pfr->Flags |= FR_DOWN;
    return OnFind(WM_COMMAND, 0, (LPARAM)pfr, bHandled);
}

LRESULT CMainDlg::OnFindPrev(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& bHandled)
{
    LPFINDREPLACE pfr = mFind.GetNotifier();
    if (pfr->lpstrFindWhat[0] == _T('\0')) return FALSE;

    pfr->Flags &= ~FR_DOWN;
    return OnFind(WM_COMMAND, 0, (LPARAM)pfr, bHandled);
}

/**
* ���ڵ㼶��ѡ��
*/
LRESULT CMainDlg::OnItemChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
{
    NMTVITEMCHANGE *pChange = (NMTVITEMCHANGE *)pnmh;
    CAssemblyNode *pNode = (CAssemblyNode *)pChange->lParam;
    BOOL bChecked = (pChange->uStateNew >> 12) == 2 ? TRUE : FALSE;
    // ѡ��״̬�ı�
    if (NULL != pNode && bChecked != pNode->bCheck)
    {
        pNode->bCheck = bChecked;
        for (int i = 0; i < pNode->Parent.GetSize(); i++)
        {
            CAssemblyNode *pParent = pNode->Parent.GetKeyAt(i);
            HTREEITEM hItem = pNode->Parent.GetValueAt(i);
            if (NULL != pParent && ((bChecked ^ pParent->bCheck)))
            {
                BOOL bAll = TRUE;
                for (int j = 0; j < pParent->Package.GetSize(); j++)
                {
                    CAssemblyNode *pChild = pParent->Package.GetValueAt(j);
                    if (NULL != pChild && !pChild->bCheck)
                    {
                        bAll = FALSE;
                        break;
                    }
                }

                if (!bChecked) pParent->bCheck = bAll;

                for (int j = 0; j < pParent->Parent.GetSize(); j++)
                {
                    TreeView_SetCheckState(pnmh->hwndFrom, pParent->Parent.GetValueAt(j), bAll);
                }
            }
            if (pChange->hItem != hItem)
            {
                TreeView_SetCheckState(pnmh->hwndFrom, hItem, bChecked);
            }
        }
        for (int i = 0; i < pNode->Package.GetSize(); i++)
        {
            CAssemblyNode *pChild = pNode->Package.GetValueAt(i);
            if (NULL != pChild)
            {
                for (int j = 0; j < pChild->Parent.GetSize(); j++)
                {
                    TreeView_SetCheckState(pnmh->hwndFrom, pChild->Parent.GetValueAt(j), bChecked);
                }
            }
        }
    }
    return 0;
}

LRESULT CMainDlg::OnContext(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
{
    POINT pt = { 0 };
    // �����Ҽ��˵�
    ::GetCursorPos(&pt);
    return ::TrackPopupMenu(::GetSubMenu(m_hMenu, 0), TPM_LEFTALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, m_hWnd, NULL);
}

/**
* �ݹ鵼�������б�
*/
void RecurveExport(CAssemblyNode *pParent, HANDLE hFile)
{
    CHAR szName[MAX_PATH * 2];
    for (int i = 0; i < pParent->Package.GetSize(); i++)
    {
        CAssemblyNode *pNode = pParent->Package.GetValueAt(i);
        if (NULL == pNode) continue;

        if (pNode->bCheck)
        {
            DWORD cbLen = (DWORD)sprintf_s(szName, _countof(szName), "%ws\r\n", (LPCTSTR)pNode->name);
            ::WriteFile(hFile, szName, cbLen, &cbLen, NULL);
        }
        else if (pNode->Package.GetSize() > 0)
        {
            RecurveExport(pNode, hFile);
        }
    }
}

LRESULT CMainDlg::OnExport(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CAtlString szTitle, szFilter;
    szTitle.LoadString(IDS_SAVEPATH);
    szFilter.LoadString(IDS_SAVEFILTER);
    szFilter.Replace(TEXT('|'), TEXT('\0'));
   
    OPENFILENAME ofn = { sizeof(OPENFILENAME) };
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrTitle = szTitle;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrDefExt = TEXT("txt");
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFile = m_szExport;

    if (!::GetSaveFileName(&ofn)) return FALSE;

    HANDLE hFile = ::CreateFile(m_szExport, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (INVALID_HANDLE_VALUE == hFile) return FALSE;

    RecurveExport(mRoot, hFile);
    return ::CloseHandle(hFile);
}

LRESULT CMainDlg::OnSearch(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    // ��Ϊ�ղ��ܲ���
    if (mRoot->Package.GetSize() == 0) return FALSE;
    if (IsWorking()) return FALSE;
    if (NULL != mFind.m_hWnd && mFind.IsWindow()) return TRUE;
    return mFind.Create(FR_DOWN, m_hWnd);
}

LRESULT CMainDlg::OnFresh(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (IsWorking()) return FALSE;

    TreeView_DeleteAllItems(mTree);
    mRoot->Package.RemoveAll();
    mMap.RemoveAll();
 
    // ����ɨ���߳�
    m_hThread = ::CreateThread(NULL, 0, CMainDlg::ThreadScan, this, 0, NULL);
    return TRUE;
}

LRESULT CMainDlg::OnFilterChar(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    if (VK_RETURN == wParam)
    {
        CAtlString szFind;
        mFilter.GetWindowText(szFind);
        for (int i = 0; i < mMap.GetSize(); i++)
        {
            CAssemblyNode *pNode = mMap.GetValueAt(i);
            if (::StrStrI(pNode->name, szFind) != NULL)
            {
                for (int j = 0; j < pNode->Parent.GetSize(); j++)
                {
                    TreeView_SetItemState(mTree, pNode->Parent.GetValueAt(j), TVIS_BOLD, TVIS_BOLD);
                    TreeView_EnsureVisible(mTree, pNode->Parent.GetValueAt(j));
                }
            }
            else
            {
                for (int j = 0; j < pNode->Parent.GetSize(); j++)
                {
                    TreeView_SetItemState(mTree, pNode->Parent.GetValueAt(j), 0, TVIS_BOLD);
                    TreeView_Expand(mTree, pNode->Parent.GetValueAt(j), TVE_COLLAPSE);
                }
            }
        }
        return TRUE;
    }
    bHandled = FALSE;
    return TRUE;
}

BOOL CMainDlg::IsWorking()
{
    if (NULL != m_hThread && ::WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT)
    {
        CAtlString szText;
        szText.LoadString(IDS_BUSY);
        return MessageBox(szText, CMainDlg::GetWndCaption(), MB_ICONWARNING);
    }
    return FALSE;
}