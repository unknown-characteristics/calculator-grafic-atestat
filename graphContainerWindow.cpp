#include "graphContainerWindow.h"
#include "functionListWindow.h"
#include <algorithm>
#include <sstream>
PCWSTR baseWindow<graphContainerWindow>::className = L"Graph Container Window Class";

vector2 lastPos;
double pixelsPerMainUnit = -1, mainUnit = 1, targetPPMU = 150;
int mainUnitDigit = 1, selectedGraph = -1, hitTestMode = 0; //0 for D2D testing, 1 for manual testing

bool showMajorGridlines = 1, showMinorGridlines = 1, showOX = 1, showOY = 1, showGridText = 1, isGraphClickable = 1;

std::wstring prepareNumber(double num)
{
	std::wostringstream outstream;
	outstream << num;
	return outstream.str();
}

LRESULT graphContainerWindow::procMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		GetClientRect(m_hwnd, &clientSize);
		cam.resizeCamera({ (double)clientSize.right, (double)clientSize.bottom });
		CreateGraphicsResources();

		functie* itemOX, *itemOY;
		itemOX = new functie(L"y=0");
		itemOY = new functie(L"x=0");

		axaOX = new graficFunctie(itemOX, cam);
		axaOY = new graficFunctie(itemOY, cam);

		double posZero = cam.worldToPixel({ 0, 0 }).x, posOne = cam.worldToPixel({ 1,0 }).x;
		pixelsPerMainUnit = posOne - posZero;
		handleMainUnitSize();

		HRESULT hr;
		hr = pDWFactory->CreateTextFormat(L"Lucida Sans Unicode", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 20.0f, L"en-us", &pPointFormat);
		if (FAILED(hr)) return -1;
		hr = pDWFactory->CreateTextFormat(L"Lucida Sans Unicode", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15.0f, L"en-us", &pSmallFormat);
		if (FAILED(hr)) return -1;

		D2D1_STROKE_STYLE_PROPERTIES prop = D2D1::StrokeStyleProperties();
		prop.startCap = prop.endCap = prop.dashCap = D2D1_CAP_STYLE_FLAT;
		prop.lineJoin = D2D1_LINE_JOIN_ROUND;
		prop.miterLimit = 1.0f;
		hr = pFactory->CreateStrokeStyle(prop, NULL, 0, &pGraphStroke);
		if (FAILED(hr)) return -1;

		pSmallFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		pPointFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP); pPointFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
		return 0;
	}
	case WM_PAINT:
	{
		HRESULT hr = CreateGraphicsResources();
		if (FAILED(hr)) return 0;

		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		pRenderTarget->BeginDraw();
		pRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		//draw minor gridlines
		double smallUnit, pixelsPerSmallUnit, currPosWX, currPosPX;
		vector2 ULcorner, BRcorner;
		if (mainUnitDigit == 2) 
		{
			smallUnit = mainUnit / 4; 
			pixelsPerSmallUnit = pixelsPerMainUnit / 4;
		}
		else
		{
			smallUnit = mainUnit / 5;
			pixelsPerSmallUnit = pixelsPerMainUnit / 5;
		}

		pBrush->SetOpacity(0.5);
		pBrush->SetColor(D2D1::ColorF(0.75f, 0.75f, 0.75f));

		ULcorner = cam.cameraPosWX - cam.cameraSizeWX / 2;
		BRcorner = cam.cameraPosWX + cam.cameraSizeWX / 2;
		if(showMinorGridlines)
		{
			currPosWX = floor(ULcorner.x / smallUnit) * smallUnit;
			currPosPX = cam.worldToPixel({ currPosWX, currPosWX }).x;
			while (currPosPX <= clientSize.right)
			{
				pRenderTarget->DrawLine({ (float)currPosPX, 0 }, { (float)currPosPX, (float)clientSize.bottom }, pBrush, 1.5f);
				currPosPX += pixelsPerSmallUnit;
			}
			currPosWX = ceil(ULcorner.y / smallUnit) * smallUnit;
			currPosPX = cam.worldToPixel({ currPosWX, currPosWX }).y;
			while (currPosPX <= clientSize.bottom)
			{
				pRenderTarget->DrawLine({ 0, (float)currPosPX }, { (float)clientSize.right, (float)currPosPX }, pBrush, 1.5f);
				currPosPX += pixelsPerSmallUnit;
			}
		}

		//draw major gridlines
		pBrush->SetOpacity(1);
		pBrush->SetColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
		if(showMajorGridlines)
		{
			currPosWX = floor(ULcorner.x / mainUnit) * mainUnit;
			currPosPX = cam.worldToPixel({ currPosWX, currPosWX }).x;
			while (currPosPX <= clientSize.right)
			{
				pRenderTarget->DrawLine({ (float)currPosPX, 0 }, { (float)currPosPX, (float)clientSize.bottom }, pBrush, 1.5f);
				currPosPX += pixelsPerMainUnit;
			}
			currPosWX = ceil(ULcorner.y / mainUnit) * mainUnit;
			currPosPX = cam.worldToPixel({ currPosWX, currPosWX }).y;
			while (currPosPX <= clientSize.bottom)
			{
				pRenderTarget->DrawLine({ 0, (float)currPosPX }, { (float)clientSize.right, (float)currPosPX }, pBrush, 1.5f);
				currPosPX += pixelsPerMainUnit;
			}
		}

		//draw axis
		pBrush->SetColor(D2D1::ColorF(0, 0, 0));
		if(showOX) pRenderTarget->DrawGeometry(axaOX->pPathGeom, pBrush, 2.0f);
		if(showOY) pRenderTarget->DrawGeometry(axaOY->pPathGeom, pBrush, 2.0f);

		//draw graphs
		for (int i = 0; i < listLength; i++)
			if(globalFunctionListWindow->list[i]->props.visible && i != selectedGraph)
			{
				if (graf[i]->pFunc->state.justUpdated) RecreateGraph(i);
				if (graf[i]->canDrawGraph)
				{
					pBrush->SetColor(globalFunctionListWindow->list[i]->props.color);
					pRenderTarget->DrawGeometry(graf[i]->pPathGeom, pBrush, 3.0f, pGraphStroke);
				}
			}

		//draw unit text markings
		static IDWriteTextLayout* pLayout = NULL;
		static std::wstring numstr;
		static DWRITE_TEXT_METRICS metrics;
		if(showGridText)
		{
			double posOY = axaOY->pointList[0].posPX.x, posOX = axaOX->pointList[0].posPX.y, pos;
			if(showOX)
			{
				pBrush->SetColor(D2D1::ColorF(0.5f, 0.5f, 0.5f));
				currPosWX = floor(ULcorner.x / mainUnit) * mainUnit;
				currPosPX = cam.worldToPixel({ currPosWX, currPosWX }).x;
				pSmallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
				while (currPosPX <= clientSize.right)
				{
					if (abs(round(currPosWX / mainUnit) * mainUnit) >= mainUnit) //let Oy draw 0 so there's no overlap
					{
						numstr = prepareNumber(abs(round(currPosWX / mainUnit) * mainUnit) < mainUnit ? 0 : currPosWX);
						pDWFactory->CreateTextLayout(&numstr[0], numstr.size(), pSmallFormat, 0, 0, &pLayout);

						pLayout->GetMetrics(&metrics);
						if (posOX + metrics.height > clientSize.bottom) pos = clientSize.bottom - metrics.height;
						else pos = max(posOX, 0);

						pRenderTarget->FillRectangle(D2D1::RectF(currPosPX - metrics.width / 2, pos + 2, currPosPX + metrics.width / 2, pos + metrics.height - 2), pWhiteBrush);
						pRenderTarget->DrawTextLayout(D2D1::Point2F(currPosPX, pos), pLayout, pBrush);

						SafeRelease(&pLayout);
					}
					currPosPX += pixelsPerMainUnit; currPosWX += mainUnit;
				}
			}
			if(showOY)
			{
				pSmallFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
				currPosWX = ceil(ULcorner.y / mainUnit) * mainUnit;
				currPosPX = cam.worldToPixel({ currPosWX, currPosWX }).y;
				while (currPosPX <= clientSize.bottom)
				{
					numstr = prepareNumber(abs(round(currPosWX/mainUnit)*mainUnit) < mainUnit ? 0 : currPosWX);
					pDWFactory->CreateTextLayout(&numstr[0], numstr.size(), pSmallFormat, 0, 0, &pLayout);

					pLayout->GetMetrics(&metrics);
					if (posOY - metrics.width - 1 < 0) pos = metrics.width;
					else pos = min(posOY, clientSize.right) - 1;

					pRenderTarget->FillRectangle(D2D1::RectF(pos - metrics.width, currPosPX + 2, pos - 2, currPosPX + metrics.height - 2), pWhiteBrush);
					pRenderTarget->DrawTextLayout(D2D1::Point2F(pos, currPosPX), pLayout, pBrush);

					SafeRelease(&pLayout);
					currPosPX += pixelsPerMainUnit; currPosWX -= mainUnit; // minus mainUnit, because y axis direction in WX is flipped relative to y axis in PX
				}
			}
		}

		//draw point highlight if needed
		if (selectedGraph != -1 && isGraphClickable)
		{
			if (graf[selectedGraph]->pFunc->state.justUpdated) RecreateGraph(selectedGraph);
			pBrush->SetColor(globalFunctionListWindow->list[selectedGraph]->props.color);
			pRenderTarget->DrawGeometry(graf[selectedGraph]->pPathGeom, pBrush, 4.0f, pGraphStroke);

			POINT cursorPos;
			GetCursorPos(&cursorPos);
			ScreenToClient(m_hwnd, &cursorPos);
			if(cursorPos.x >= 0 && cursorPos.x <= clientSize.right)
			{
				point mousePos, hit; mousePos.posPX = { (double)cursorPos.x,(double)cursorPos.y }; mousePos.posWX = cam.pixelToWorld(mousePos.posPX);
				double posX, posY;
				if (graf[selectedGraph]->pFunc->state.funcLetter == L'x')
				{
					hit = graf[selectedGraph]->pointList[0];
					posX = hit.posPX.x; posY = cursorPos.y;
					numstr = std::wstring(L"(") + prepareNumber(hit.posWX.x) + L", " + prepareNumber(cam.pixelToWorld({posX, posY}).y) + L")";
				}
				else if (graf[selectedGraph]->pFunc->state.funcLetter == L'y' && graf[selectedGraph]->pFunc->state.variables.size() == 0)
				{
					hit = graf[selectedGraph]->pointList[0];
					posX = cursorPos.x; posY = hit.posPX.y;
					numstr = std::wstring(L"(") + prepareNumber(cam.pixelToWorld({posX, posY}).x) + L", " + prepareNumber(hit.posWX.y) + L")";
				}
				else
				{
					double increment = cam.effectiveUnitPerPixel.x / pointsPerPixel;
					mousePos.posWX.x = floor(mousePos.posWX.x / increment) * increment;
					double leftmostPos = cam.cameraPosWX.x - cam.cameraSizeWX.x / 2;
					leftmostPos = floor(leftmostPos / increment) * increment;
					int leftdif = (mousePos.posWX.x - leftmostPos) / increment;

					if (graf[selectedGraph]->pointList[leftdif].posWX.x != mousePos.posWX.x)
					{
						while (graf[selectedGraph]->pointList[leftdif].posWX.x > mousePos.posWX.x) leftdif--;
						while (graf[selectedGraph]->pointList[leftdif].posWX.x < mousePos.posWX.x) leftdif++;
					}
					hit = graf[selectedGraph]->pointList[leftdif];
					posX = cursorPos.x; posY = hit.posPX.y;
					numstr = std::wstring(L"(") + prepareNumber(hit.posWX.x) + L", " + prepareNumber(hit.posWX.y) + L")";
				}

				if (!isnan(hit.posWX.y) && isfinite(hit.posWX.y))
				{
					pDWFactory->CreateTextLayout(&numstr[0], numstr.size(), pPointFormat, 0, 0, &pLayout);

					pLayout->GetMetrics(&metrics);
					pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(posX, posY), 5.0f, 5.0f), pBrush);

					if (posX - metrics.width / 2 < 0) posX = metrics.width / 2;
					else if (posX + metrics.width / 2 > clientSize.right) posX = clientSize.right - metrics.width / 2;

					if (posY + metrics.height > clientSize.bottom) posY = clientSize.bottom - metrics.height;
					else if (posY < 0) posY = 0;

					pRenderTarget->FillRectangle(D2D1::RectF(posX - metrics.width / 2, posY, posX + metrics.width / 2, posY + metrics.height), pWhiteBrush);

					pBrush->SetColor(D2D1::ColorF(0, 0, 0));
					pRenderTarget->DrawTextLayout(D2D1::Point2F(posX, posY), pLayout, pBrush);

					SafeRelease(&pLayout);
				}
			}
		}
		hr = pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardGraphicsResources();
		EndPaint(m_hwnd, &ps);
		return 0;
	}
	case WM_SIZE:
	{
		GetClientRect(m_hwnd, &clientSize);
		if (pRenderTarget == NULL) return 0;
		D2D1_SIZE_U size = D2D1::SizeU(clientSize.right, clientSize.bottom);
		pRenderTarget->Resize(size);
		cam.resizeCamera({ (double)clientSize.right, (double)clientSize.bottom });

		for (int i = 0; i < listLength; i++)
		{
			graf[i]->cameraResized(cam);
		}

		axaOX->cameraResized(cam);
		axaOY->cameraResized(cam);
		
		RecreateGraphsAfterMove();

		return 0;
	}
	case WM_ERASEBKGND:
	{
		return 1;
	}
	case WM_LBUTTONDOWN:
	{
		SetFocus(m_hwnd);
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		lastPos = { (double)xPos, (double)yPos };
		SetCapture(m_hwnd);
		capturing = 1;
		int rez = hitTestGraphs(xPos, yPos);
		selectedGraph = rez;
		if (rez != -1) RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		if (!capturing) return 0;
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		vector2 currPos = { (double)xPos, (double)yPos };
		if (lastPos == currPos) return 0;
		if(selectedGraph==-1)
		{
			vector2 delta = lastPos - currPos;
			delta *= cam.unitPerPixel;
			cam.moveCamera(cam.cameraPosWX + delta);
			RecreateGraphsAfterMove();
		}
		lastPos = currPos;
		RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		return 0;
	}
	case WM_LBUTTONUP:
	{
		if (capturing)
			ReleaseCapture();
		capturing = 0;
		selectedGraph = -1;
		RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		return 0;
	}
	case WM_MOUSEWHEEL:
	{
		//return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

		SetFocus(m_hwnd);
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam)/120;
		int xPos = GET_X_LPARAM(lParam);
		int yPos = GET_Y_LPARAM(lParam);
		RECT winPos;
		GetWindowRect(m_hwnd, &winPos);
		xPos -= winPos.left;
		yPos -= winPos.top;
		int r = cam.zoomCamera({ (double)xPos, (double)yPos }, zDelta);
		RecreateGraphsAfterZoom(r);
		handleMainUnitSize();
		RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		return 0;
	}
	case WM_KEYDOWN:
	{
		//return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

		vector2 direction;
		switch (wParam)
		{

		case VK_LEFT:
		{
			direction = { -1, 0 };
			break;
		}
		case VK_RIGHT:
		{
			direction = { 1, 0 };
			break;
		}
		case VK_UP:
		{
			direction = { 0, -1 };
			break;
		}
		case VK_DOWN:
		{
			direction = { 0, 1 };
			break;
		}

		case 0x4F:
		case VK_OEM_MINUS:
		case VK_SUBTRACT:
		{
			int r = cam.zoomCamera(cam.cameraPosPX, -1);
			RecreateGraphsAfterZoom(r);
			handleMainUnitSize();
			RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
			return 0;
		}

		case 0x49:
		case VK_OEM_PLUS:
		case VK_ADD:
		{
			int r = cam.zoomCamera(cam.cameraPosPX, 1);
			RecreateGraphsAfterZoom(r);
			handleMainUnitSize();
			RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
			return 0;
		}

		case VK_SPACE:
		{
			if (capturing) { ReleaseCapture(); capturing = 0; }
			short r = GetKeyState(VK_SHIFT);
			cam.resetCamera(r&(1<<16));   //if shift is held
			RecreateGraphs();
			handleMainUnitSize();
			RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
			return 0;
		}

		default:
			return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
		}
		cam.slideCamera(direction);
		RecreateGraphsAfterMove();
		RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		return 0;
	}
	default:
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
}

int graphContainerWindow::hitTestGraphs(int x, int y)
{
	if (!isGraphClickable) return -1;
	static int hitTestTolerance = 7;
	int index;
	if (hitTestMode == 0)
	{
		BOOL check;
		for (index = listLength - 1; index >= 0; index--)
		{
			if (!graf[index]->canDrawGraph) continue;
			graf[index]->pPathGeom->StrokeContainsPoint(D2D1::Point2F((double)x, (double)y), (FLOAT)hitTestTolerance+3, pGraphStroke, D2D1::Matrix3x2F::Identity(), &check);
			if (check) break;
		}
		return index;
	}
	else
	{
		point mousePos; mousePos.posPX = { (double)x,(double)y }; mousePos.posWX = cam.pixelToWorld(mousePos.posPX);
		double increment = cam.effectiveUnitPerPixel.x / pointsPerPixel;
		mousePos.posWX.x = floor(mousePos.posWX.x / increment) * increment;
		double leftmostPos = cam.cameraPosWX.x - cam.cameraSizeWX.x / 2;
		leftmostPos = floor(leftmostPos / increment) * increment;
		int leftdif = (mousePos.posWX.x - leftmostPos) / increment;
		for (index = listLength - 1; index >= 0; index--)
		{
			if (!graf[index]->canDrawGraph) continue;
			if (graf[index]->pFunc->state.funcLetter == L'x' || (graf[index]->pFunc->state.funcLetter == L'y' && graf[index]->pFunc->state.variables.size() == 0))
			{
				//special case
				if (graf[index]->pFunc->state.funcLetter == L'x')
				{
					if (abs(graf[index]->pointList[0].posPX.x - x) <= hitTestTolerance)
						break;
				}
				else if (abs(graf[index]->pointList[0].posPX.y - y) <= hitTestTolerance)
					break;
				continue;
			}
			if (graf[index]->pointList[leftdif].posWX.x != mousePos.posWX.x)
			{
				while (graf[index]->pointList[leftdif].posWX.x > mousePos.posWX.x) leftdif--;
				while (graf[index]->pointList[leftdif].posWX.x < mousePos.posWX.x) leftdif++;
			}
			if (abs(graf[index]->pointList[leftdif].posPX.y - y) <= hitTestTolerance)
				break;
		}
		return index;
	}
}

void graphContainerWindow::handleMainUnitSize()
{
	int retest, upperUnitDigit, lowerUnitDigit;
	double posZero = cam.worldToPixel({ 0,0 }).x, posUnit = cam.worldToPixel({ mainUnit,0 }).x;
	pixelsPerMainUnit = posUnit - posZero;
	double upperUnit, lowerUnit, posUpperUnit, posLowerUnit, lowerPixels, upperPixels;
	do
	{
		if (mainUnitDigit == 1)
		{
			lowerUnit = mainUnit / 2; lowerUnitDigit = 5;
			upperUnit = mainUnit * 2; upperUnitDigit = 2;
		}
		else if (mainUnitDigit == 2)
		{
			lowerUnit = mainUnit / 2; lowerUnitDigit = 1;
			upperUnit = mainUnit * 2.5f; upperUnitDigit = 5;
		}
		else
		{
			lowerUnit = mainUnit / 2.5; lowerUnitDigit = 2;
			upperUnit = mainUnit * 2; upperUnitDigit = 1;
		}
		posUpperUnit = cam.worldToPixel({ upperUnit, 0 }).x;  posLowerUnit = cam.worldToPixel({ lowerUnit, 0 }).x;
		lowerPixels = posLowerUnit - posZero; upperPixels = posUpperUnit - posZero;
		if (abs(pixelsPerMainUnit - targetPPMU) > abs(lowerPixels - targetPPMU))
		{
			pixelsPerMainUnit = lowerPixels; mainUnit = lowerUnit; mainUnitDigit = lowerUnitDigit;
			retest = 1;
		}
		else if (abs(pixelsPerMainUnit - targetPPMU) > abs(upperPixels - targetPPMU))
		{
			pixelsPerMainUnit = upperPixels; mainUnit = upperUnit; mainUnitDigit = upperUnitDigit;
			retest = 1;
		}
		else retest = 0;
	}
	while (retest);
}

HRESULT graphContainerWindow::CreateGraphicsResources()
{
	HRESULT hr = baseWindowDrawable<graphContainerWindow>::CreateGraphicsResources();
	if (FAILED(hr)) return hr;
	if(pBrush == NULL)
		hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pBrush);
	if (FAILED(hr)) return hr;
	if (pWhiteBrush == NULL)
		hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pWhiteBrush);
	if (FAILED(hr)) return hr;
	pWhiteBrush->SetOpacity(0.75f);
	return hr;
}

void graphContainerWindow::DiscardGraphicsResources()
{
	SafeRelease(&pBrush);
	SafeRelease(&pWhiteBrush);
	baseWindowDrawable<graphContainerWindow>::DiscardGraphicsResources();
}

void graphContainerWindow::RecreateGraphsAfterMove()
{
	for (int i = 0; i < listLength; i++)
		graf[i]->fixPoints(cam);
	axaOX->fixPoints(cam);
	axaOY->fixPoints(cam);
}

void graphContainerWindow::GraphAccuracyChanged()
{
	for (int i = 0; i < listLength; i++)
		graf[i]->cameraResized(cam);
	RecreateGraphs();
}

void graphContainerWindow::RecreateGraphsAfterZoom(int mode)
{
	if (mode == 0) RecreateGraphsAfterMove();
	else if (mode == -1)
	{
		for (int i = 0; i < listLength; i++)
			graf[i]->reduceResolution(cam);

		axaOX->reduceResolution(cam);
		axaOY->reduceResolution(cam);
	}
	else
	{
		for (int i = 0; i < listLength; i++)
			graf[i]->increaseResolution(cam);

		axaOX->increaseResolution(cam);
		axaOY->increaseResolution(cam);
	}
}

void graphContainerWindow::RecreateGraphs()
{
	RecreateGraph(axaOX); 
	RecreateGraph(axaOY);
	for (int i = 0; i < listLength; i++)
		RecreateGraph(i);
}

int graphContainerWindow::RecreateGraph(int i)
{
	if (globalFunctionListWindow->list[i]->props.visible == 0)
	{
		graf[i]->updatePostponed = 1;
		return 0;
	}
	return RecreateGraph(graf[i]);
}

int graphContainerWindow::RecreateGraph(graficFunctie* ptr)
{
	ptr->updatePostponed = 0;
	int r = ptr->generateGraph(cam);
	ptr->pFunc->state.justUpdated = 0;
	return r;
}


void graphContainerWindow::itemChanged(targetType target, int index1, int index2)
{
	switch (target)
	{
		using enum targetType;
	case none:
		return; //huh?
	case closeButton:
	{
		delete graf[index1];
		for (; index1 < listLength - 1; index1++)
		{
			graf[index1] = graf[index1 + 1];
			//RecreateGraph(index1);
		}
		listLength--;
		break;
	}
	case moveItem:
	{
		graficFunctie* tmp = graf[index1];
		if (index1 > index2)
		{
			for (int i = index1; i > index2; i--)
				graf[i] = graf[i - 1];
			graf[index2] = tmp;

		}
		else
		{
			for (int i = index1; i < index2; i++)
				graf[i] = graf[i + 1];
			graf[index2] = tmp;
		}
		break;
	}
	case newItem:
	{
		graf[index1] = new graficFunctie(globalFunctionListWindow->list[index1]->funcptr, cam);
		listLength++;
		break;
	}
	case modifyItem:
	{
		RecreateGraph(index1);
		break;
	}
	case visibilityToggle:
	{
		if (graf[index1]->updatePostponed) RecreateGraph(index1);
		break;
	}
	case colorPicker:
		break;
	}
	RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOINTERNALPAINT);
}