#pragma once
#include "baseWindow.h"
#include "mate/mate.h"

namespace functionListSizes
{
	const static int delimHeight = 4, delimWidth = 4, emptyItemHeight = 75, indentWidth = 50, closeSize = 15, closeMargin = 5, pickerSize = 20, showSize = 20;
}

enum class hitTestResult
{
	None = 0,
	Close,
	Move,
	Edit,
	Color,
	Visibility
};


class functionListItemProperties
{
public:
	D2D1_COLOR_F color;
	int visible = 0;
	functionListItemProperties();
	void changeColor(D2D1_COLOR_F col);
	void showItem();
	void hideItem();
};


class functionListItem
{
public:
	HWND editHwnd = NULL, parentHwnd = NULL;
	int height = functionListSizes::emptyItemHeight, currHeight = 0, grabbed = 0, grabbedHeight = 0;
	functionListItemProperties props;

	functie* funcptr = NULL;
	std::wstring funcstr, realstr, errorstr;

	functionListItem();
	~functionListItem();
	void hideWindow();
	void showWindow();
	void deactivateItem();
	void activateItem(int h, int w, int index, HFONT hFont, HWND parent);
	void changeHeight(int h, int scrollPos);
	void changeHeight(int h);
};

class functionListWindow;


class grabbedWindow : public baseWindowDrawable<grabbedWindow>
{
public:
	LRESULT procMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	HRESULT CreateGraphicsResources();
	void DiscardGraphicsResources();
	
	functionListWindow* parent = NULL;
	ID2D1SolidColorBrush* pNormalBrush = NULL, * pDelimBrush = NULL;
};



class functionListWindow : public baseWindowDrawable<functionListWindow>
{
public:
	LRESULT procMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

	HRESULT CreateGraphicsResources();
	void DiscardGraphicsResources();
	ID2D1SolidColorBrush *pNormalBrush = NULL, *pDelimBrush = NULL, *pCloseLineBrush = NULL;
	ID2D1StrokeStyle* pCloseStrokeStyle = NULL;
	IDWriteTextFormat* pNormalFormat = NULL, *pNewFormat = NULL, *pErrorFormat = NULL;
	ID2D1Bitmap* showBitmap, *colorPickerBitmap, *invertedShow, * invertedPicker;

	void resizeScrollbar();
	void repositionEditBoxes();
	void timerScroll();
	void handleGrabbedItem();
	void handleGrabbedItem(int x, int y);
	hitTestResult performHitTest(int x, int y, int height);

	functionListItem* list[101] = { NULL };
	int listLength = 0;
	grabbedWindow grabbedWin;
	HFONT hFont = NULL;
};

enum class targetType
{
	none = 0,
	closeButton,
	newItem,
	moveItem,
	modifyItem,
	visibilityToggle,
	colorPicker
};

extern functionListWindow* globalFunctionListWindow;