#pragma once
//#include "baseWindow.h"
#include "functionListWindow.h"
#include "graphContainerWindow.h"

class mainWindow : public baseWindowDrawable<mainWindow>
{
protected:
	ID2D1SolidColorBrush* pBrush;
public:
	mainWindow() : pBrush(nullptr), splitterSize(2), splitterPerc(1./3), splitterX(0), capturing(0) {}
	LRESULT procMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	graphContainerWindow graphConWin;
	functionListWindow funcListWin;
	virtual HRESULT CreateGraphicsResources();
	virtual void DiscardGraphicsResources();
	void SetLayout();
	int splitterSize, splitterX, capturing;
	double splitterPerc;
};


