#include "stdafx.h"
#include "ListCtrlEx.h"

BEGIN_MESSAGE_MAP(CListCtrlEx, CListCtrl)
	//{{AFX_MSG_MAP(CListCtrlEx)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CListCtrlEx::auto_size()
{
	if (!GetSafeHwnd() || !GetHeaderCtrl())
		return;
	auto count = GetHeaderCtrl()->GetItemCount();
	for (int i = 0; i < count; i++)
	{
		// to prevent crash
		switch (i)
		{
		case 0:
			SetColumnWidth(i, 160);//LVSCW_AUTOSIZE_USEHEADER);
			break;
		case 1:
			SetColumnWidth(i, 360);//LVSCW_AUTOSIZE_USEHEADER);
			break;
		default:
			SetColumnWidth(i, 200);//LVSCW_AUTOSIZE_USEHEADER);
			break;
		}
	}
}

void CListCtrlEx::set_size(int width, int column)
{
	if (!GetSafeHwnd() || !GetHeaderCtrl())
		return;
	auto count = GetHeaderCtrl()->GetItemCount();
	if (column < 0 || column >= count)
		return;
	SetColumnWidth(column, width <= 0 ? LVSCW_AUTOSIZE_USEHEADER : width);
}

void CListCtrlEx::DeleteAllColumns()
{
	while (GetHeaderCtrl()->GetItemCount())
		DeleteColumn(0);
}

DWORD CListCtrlEx::GetItemData(int nItem) const
{
	return nItem == -1 ? -1 : CListCtrl::GetItemData(nItem);
}

int CListCtrlEx::InsertItemData(int nItem, DWORD dwData)
{
	int index = InsertItem(nItem, LPSTR_TEXTCALLBACK);
	SetItemData(index, dwData);
	return index;
}

int CListCtrlEx::InsertItemData(DWORD dwData)
{
	return InsertItemData(GetItemCount(), dwData);
}

void CListCtrlEx::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	if ((GetStyle() & LVS_TYPEMASK) != LVS_REPORT)
		return;
	NMLVCUSTOMDRAW* pCustomDraw = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);
	switch (pCustomDraw->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
		pCustomDraw->clrTextBk = pCustomDraw->nmcd.dwItemSpec & 1 ? RGB(0xf8, 0xf8, 0xf8) : RGB(0xff, 0xff, 0xff);
		break;
	}
}

void CListCtrlEx::OnSize(UINT nType, int cx, int cy)
{
	CListCtrl::OnSize(nType, cx, cy);
	auto_size();
}

void CListCtrlEx::PreSubclassWindow()
{
	CListCtrl::PreSubclassWindow();
	SetExtendedStyle(GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);
}

BOOL CListCtrlEx::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (GetKeyState(VK_CONTROL) < 0)
		{
			switch (pMsg->wParam)
			{
			case 'A':
				if (GetStyle() & LVS_SINGLESEL)
					break;
				select_all();
				return true;
			case VK_ADD:
				auto_size();
				return true;
			}
		}
	}
	return CListCtrl::PreTranslateMessage(pMsg);
}

void CListCtrlEx::select_all()
{
	for (int i = 0; i < GetItemCount(); i++)
		SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
}

std::string& CListCtrlEx::get_buffer()
{
	return m_buffer[++m_buffer_w &= 3].erase();
}

std::string CListCtrlEx::get_selected_rows_tsv()
{
	string d;
	for (int j = 0; j < GetHeaderCtrl()->GetItemCount(); j++)
	{
		const int cb_b = 256;
		char b[cb_b];
		HDITEM item;
		item.mask = HDI_TEXT;
		item.pszText = b;
		item.cchTextMax = cb_b - 1;
		GetHeaderCtrl()->GetItem(j, &item);
		d += item.pszText;
		d += "\t";
	}
	d += "\r\n";
	for (int i = -1; (i = GetNextItem(i, LVNI_SELECTED)) != -1; )
	{
		for (int j = 0; j < GetHeaderCtrl()->GetItemCount(); j++)
			d += GetItemText(i, j) + "\t";
		d += "\r\n";
	}
	return d;
}
