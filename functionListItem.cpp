#include "functionListWindow.h"
#include <cstdlib>
#include <string>
using namespace functionListSizes;




LRESULT EditSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
	case WM_KEYDOWN:
	{		
		auto tmp = DefSubclassProc(hWnd, uMsg, wParam, lParam);
		RedrawWindow((HWND)dwRefData, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOCHILDREN);
		return tmp;
	}
	default:
		return DefSubclassProc(hWnd, uMsg, wParam, lParam);
	}
}


int id = 3;

void functionListItem::hideWindow()
{
	ShowWindow(editHwnd, SW_HIDE);
	props.hideItem();
}

void functionListItem::showWindow()
{
	ShowWindow(editHwnd, SW_SHOW);
	props.showItem();
}

void functionListItem::deactivateItem()
{
	hideWindow();
}

void functionListItem::activateItem(int h, int w, int index, HFONT hFont, HWND parent)
{
	currHeight = h;

	std::wstring v = std::to_wstring(index);
	editHwnd = CreateWindowEx(0, L"EDIT", L"", WS_CLIPSIBLINGS | WS_CHILD | ES_AUTOHSCROLL | ES_LEFT, indentWidth + 5, h + emptyItemHeight / 3, w - indentWidth - delimWidth - 2 * closeMargin - closeSize - 5, emptyItemHeight / 3, parentHwnd, NULL, hInst, NULL);
	SetWindowSubclass(editHwnd, EditSubclass, id, (DWORD_PTR)parent);
	SendMessage(editHwnd, WM_SETFONT, (WPARAM)hFont, (LPARAM)true);
	SendMessage(editHwnd, EM_SETLIMITTEXT, 100, 0);

	showWindow();

}

functionListItem::functionListItem()
{
	funcptr = new functie();
}

functionListItem::~functionListItem()
{
	if (funcptr != NULL) delete funcptr;
	DestroyWindow(editHwnd);
	//other stuff, eventually...
}

void functionListItem::changeHeight(int h, int scrollPos)
{
	currHeight = h;
	if (grabbed) h = grabbedHeight;
	h -= scrollPos;
	
	SetWindowPos(editHwnd, 0, indentWidth + 5, h + emptyItemHeight / 3, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
}

void functionListItem::changeHeight(int h)
{
	currHeight = h;
}



functionListItemProperties::functionListItemProperties()
{
	visible = 0;
	float rnd[3] = { 0 };
	rnd[0] = rand() % 256 / 256.f;
	rnd[1] = rand() % 256 / 256.f;
	rnd[2] = rand() % 256 / 256.f;
	while (rnd[0] * rnd[1] * rnd[1] <= 0.0001f || rnd[0] * rnd[1] * rnd[2] >= 0.8f)
	{
		int rndIndex = rand() % 3;
		rnd[rndIndex] = rand() % 255 / 255.f;
	}
	color = D2D1::ColorF( rnd[0], rnd[1], rnd[2] );
}

void functionListItemProperties::changeColor(D2D1_COLOR_F col)
{
	color = col;
}

void functionListItemProperties::showItem()
{
	visible = 1;
}

void functionListItemProperties::hideItem()
{
	visible = 0;
}