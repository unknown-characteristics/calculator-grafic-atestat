#include "functionListWindow.h"
#include "graphContainerWindow.h"
PCWSTR baseWindow<functionListWindow>::className = L"Function List Window Class";
PCWSTR baseWindow<grabbedWindow>::className = L"Grabbed Window Helper Class";


int timerSet = 0;
int grabbedFrom = 0;
int lastPos = 0;

int focusedIndex = -1, initialIndex = -1, targetIndex = -1;

float scrollVelocity = 0, scrollAccel = 0;
int currScrollPos = 0;
int scrollDirection = 0;

COLORREF custColors[16];

targetType currentTarget = targetType::none;

using namespace functionListSizes;

int addWhitespace(const std::wstring& str, int pos)
{
	int realPos = -1, i = 0;
	for (i = 0; i < str.size() && realPos != pos; i++)
		if (str[i] != L' ') realPos++;
	return i-1;
}
//abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZαβγδεζηθικλμνξοπρστυφχψωăîâșț
struct alphabetEntry { std::wstring replaced; wchar_t replacer; } arr[] = 
{
	{L"pi", L'π' },
	{L"alfa", L'α' },
	{L"beta", L'β' },
	{L"gamma", L'γ' },
	{L"delta", L'δ' },
	{L"epsilon", L'ε' },
	{L"theta", L'θ' },
	{L"zeta", L'ζ' },
	{L"eta", L'η' },
	{L"lambda", L'λ' },
	{L"miu", L'μ' },
	{L"tau", L'τ' },
	{L"niu", L'ν' },
	{L"phi", L'φ' },
	{L"sigma", L'σ' },
	//{L"ro", L'ρ' },   conflicts with nthroot
	{L"omega", L'ω' },
	{L"chi", L'χ' },
	//{L"psi", L'ψ' },  conflicts with epsilon
	{L"xi", L'ξ' },
	{L"iota", L'ι' },
	{L"kappa", L'κ' }
};

std::pair<int, int> formatString(std::wstring& str)
{
	int ok = 0, pos = 0;
	for (auto& entry : arr)
	{
		while (str.find(entry.replaced) != str.npos)
		{
			ok = 1;
			pos = str.find(entry.replaced);
			str.replace(str.find(entry.replaced), entry.replaced.size(), 1, entry.replacer);
		}
	}
	return { ok, pos };
}

// https://devblogs.microsoft.com/oldnewthing/20191108-00/?p=103080
void ForceTimerMessagesToBeCreatedIfNecessary()
{
	MSG msg;
	PeekMessage(&msg, nullptr, WM_TIMER, WM_TIMER, PM_NOREMOVE);
}

HRESULT LoadResourceBitmap(ID2D1RenderTarget* pRenderTarget, int pngID, ID2D1Bitmap** ppBitmap)
{
	HRSRC imageResHandle = FindResourceW(NULL, MAKEINTRESOURCE(pngID), L"PNG");
	if (!imageResHandle) return E_FAIL;

	HGLOBAL imageResDataHandle = LoadResource(NULL, imageResHandle);
	if (!imageResDataHandle) return E_FAIL;

	void* pImageFile = LockResource(imageResDataHandle);
	if (!pImageFile) return E_FAIL;

	DWORD imageFileSize = SizeofResource(NULL, imageResHandle);
	if (!imageFileSize) return E_FAIL;

	HRESULT hr;
	IWICStream* pStream;
	hr = pWICFactory->CreateStream(&pStream);
	if (FAILED(hr)) return hr;

	hr = pStream->InitializeFromMemory(reinterpret_cast<BYTE*>(pImageFile), imageFileSize);
	if (FAILED(hr)) { SafeRelease(&pStream); return hr; }

	IWICBitmapDecoder* pDecoder;
	hr = pWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnDemand, &pDecoder);
	if (FAILED(hr)) { SafeRelease(&pStream); return hr; }

	IWICBitmapFrameDecode* pSource;
	hr = pDecoder->GetFrame(0, &pSource);
	if (FAILED(hr)) { SafeRelease(&pDecoder); SafeRelease(&pStream); return hr; }

	IWICFormatConverter* pConverter;
	hr = pWICFactory->CreateFormatConverter(&pConverter);
	if (FAILED(hr)) { SafeRelease(&pSource); SafeRelease(&pDecoder); SafeRelease(&pStream); return hr; }

	hr = pConverter->Initialize(pSource, GUID_WICPixelFormat32bppPBGRA, WICBitmapDitherTypeNone, NULL, 1.f, WICBitmapPaletteTypeMedianCut);
	if (FAILED(hr)) { SafeRelease(&pConverter); SafeRelease(&pSource); SafeRelease(&pDecoder); SafeRelease(&pStream); return hr; }

	hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, NULL, ppBitmap);
	SafeRelease(&pConverter); SafeRelease(&pSource); SafeRelease(&pDecoder); SafeRelease(&pStream);
	return hr;
}


LRESULT functionListWindow::procMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ForceTimerMessagesToBeCreatedIfNecessary();
	switch (uMsg)
	{
	case WM_CREATE:
	{
		GetClientRect(m_hwnd, &clientSize);
		resizeScrollbar();
		SetTimer(m_hwnd, 1111, 1, NULL);
		if (!grabbedWin.Create(hInst, L"grabbed temp", WS_CHILD | WS_CLIPSIBLINGS, 0, m_hwnd, 0, 0, 0, 0, 0))
		{
			return -1;
		}
		grabbedWin.parent = this;
		ShowWindow(grabbedWin.m_hwnd, SW_SHOWNORMAL);
		ShowWindow(grabbedWin.m_hwnd, SW_HIDE);

		D2D1_STROKE_STYLE_PROPERTIES prop = { D2D1_CAP_STYLE_ROUND , D2D1_CAP_STYLE_ROUND , D2D1_CAP_STYLE_ROUND, D2D1_LINE_JOIN_MITER, 1.f, D2D1_DASH_STYLE_SOLID, 0 };
		if (FAILED(pFactory->CreateStrokeStyle(prop, NULL, 0, &pCloseStrokeStyle)))
		{
			return -1;
		}

		CreateGraphicsResources();

		hFont = CreateFont(emptyItemHeight / 3, 0, 0, 0, FW_NORMAL, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Lucida Sans Unicode");
		
		HRESULT hr;
		hr = pDWFactory->CreateTextFormat(L"Lucida Sans Unicode", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 15.0f, L"en-us", &pNormalFormat);
		if (FAILED(hr)) return -1;
		hr = pDWFactory->CreateTextFormat(L"Lucida Sans Unicode", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 12.0f, L"ro-ro", &pErrorFormat);
		if (FAILED(hr)) return -1;
		hr = pDWFactory->CreateTextFormat(L"Lucida Sans Unicode", NULL, DWRITE_FONT_WEIGHT_REGULAR, DWRITE_FONT_STYLE_ITALIC, DWRITE_FONT_STRETCH_NORMAL, 20.0f, L"ro-ro", &pNewFormat);
		if (FAILED(hr)) return -1;

		return 0;
	}

	case WM_COMMAND:
	{
		int msg = HIWORD(wParam);
		HWND editHwnd = (HWND)lParam;
		switch (msg)
		{
		case EN_SETFOCUS:
		{
			for (focusedIndex = 0; focusedIndex < listLength; focusedIndex++)
				if (list[focusedIndex]->editHwnd == editHwnd) break;
			if (focusedIndex == listLength) focusedIndex = -1; //how?
			return 0;
		}
		case EN_KILLFOCUS:
		{
			focusedIndex = -1; return 0;
		}
		case EN_CHANGE:
		{
			if (focusedIndex == -1)
				for (focusedIndex = 0; focusedIndex < listLength; focusedIndex++)
					if (list[focusedIndex]->editHwnd == editHwnd) break;
			TCHAR buf[256] = { 0 }; buf[0] = 255;
			int len = SendMessage(editHwnd, EM_GETLINE, 0, (LPARAM)buf);
			buf[len] = 0;
			std::wstring s = std::wstring(buf);
			auto [ok, pos] = formatString(s);
			if (ok)
			{
				for (int i = 0; i < s.length(); i++)
					buf[i] = s[i];
				buf[s.length()] = 0;
				SetWindowText(editHwnd, buf);
				SendMessage(editHwnd, EM_SETSEL, pos+1, pos+1);
			}
			list[focusedIndex]->realstr = s;
			while (s.find(L' ') != s.npos) s.erase(s.find(L' '), 1);
			if (s != list[focusedIndex]->funcstr)
			{
				list[focusedIndex]->funcstr = s;
				list[focusedIndex]->funcptr->updateString(list[focusedIndex]->funcstr);
				list[focusedIndex]->errorstr = list[focusedIndex]->funcptr->getErrorString();
				globalGraphContainerWindow->itemChanged(targetType::modifyItem, focusedIndex, 0);
			}
			RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOCHILDREN);

			return 0;
		}
		default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
		}
	}

	case WM_PAINT:
	{
		HRESULT hr = CreateGraphicsResources();
		if (FAILED(hr)) return 0;


		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		pRenderTarget->BeginDraw();
		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));


		static std::wstring numStr, errorStr;
		int top = 0, bottom = 0;
		for (int i = 0; i < listLength; i++)
		{
			top = list[i]->currHeight - currScrollPos;
			bottom = top + list[i]->height;

			if (list[i]->grabbed) continue;

			if (bottom + delimHeight < 0) continue;
			if (top >= clientSize.bottom) break;
			
			pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

			D2D1_COLOR_F textColor = D2D1::ColorF(D2D1::ColorF::Black);
			//draw indent background if function visible and not errored
			if (list[i]->funcptr->state.eroareFunctie.errorType == ErrorType::NoError && (list[i]->funcptr->state.variables.size() == 1 || list[i]->funcptr->isSpecialFunction()) && list[i]->props.visible)
			{
				const auto& itemColor = list[i]->props.color;
				pNormalBrush->SetColor(itemColor);
				D2D1_RECT_F indentBackground = D2D1::RectF(0, (float)top - 1, (float)indentWidth + 1, (float)bottom + 1);
				pRenderTarget->DrawRectangle(indentBackground, pNormalBrush);
				pRenderTarget->FillRectangle(indentBackground, pNormalBrush);

				//https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color
				textColor = (0.299f * itemColor.r + 0.587f * itemColor.g + 0.114f * itemColor.b) >= 0.5f ? D2D1::ColorF(D2D1::ColorF::Black) : D2D1::ColorF(D2D1::ColorF::White);
			}
			
			//draw index number
			numStr = std::to_wstring(i + 1);
			D2D1_RECT_F numPos = D2D1::RectF(closeMargin, top, indentWidth, bottom);
			pNormalBrush->SetColor(textColor);
			pRenderTarget->DrawText(&numStr[0], numStr.size(), pNormalFormat, numPos, pNormalBrush);
			
			//draw icons if needed
			if (!((list[i]->funcptr->state.variables.size() != 1 && !list[i]->funcptr->isSpecialFunction()) || list[i]->funcstr.size() == 0 || list[i]->funcptr->state.eroareFunctie.errorValue != 0))
			{
				D2D1_RECT_F pipettePos = D2D1::RectF((float)closeMargin, (float)bottom - closeMargin - closeSize, (float)closeMargin + closeSize, (float)bottom - closeMargin);
				D2D1_RECT_F visibilityPos = D2D1::RectF((float)3 * closeMargin + closeSize, (float)bottom - closeMargin - closeSize, (float)3 * closeMargin + 2 * closeSize, (float)bottom - closeMargin);
				//if color is black
				if(textColor.r == 0)
				{
					pRenderTarget->DrawBitmap(colorPickerBitmap, pipettePos);
					pRenderTarget->DrawBitmap(showBitmap, visibilityPos);
				}
				else
				{
					pRenderTarget->DrawBitmap(invertedPicker, pipettePos);
					pRenderTarget->DrawBitmap(invertedShow, visibilityPos);
				}
				if (!list[i]->props.visible)
					pRenderTarget->DrawLine({ visibilityPos.left, visibilityPos.top }, { visibilityPos.right, visibilityPos.bottom }, pNormalBrush, 2.5f, pCloseStrokeStyle);
			}

			//draw error text
			if (list[i]->funcptr->state.eroareFunctie.errorType != ErrorType::NoError && list[i]->funcstr.size()>0)
			{
				D2D1_RECT_F errorPos = D2D1::RectF(indentWidth + delimWidth, bottom - 15, clientSize.right, bottom);
				pNormalBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
				

				//draw error indicator, if necessary
				auto& err = list[i]->funcptr->state.eroareFunctie;
				if (errorIsPositional(err))
				{
					auto errorPos = addWhitespace(list[i]->realstr, err.errorInfo);
					LRESULT rez = SendMessage(list[i]->editHwnd, EM_POSFROMCHAR, errorPos, 0);
					int xPos = LOWORD(rez)+indentWidth+delimWidth+5, yPos = HIWORD(rez)+top+emptyItemHeight/3;
					
					pRenderTarget->DrawLine({ (float)xPos, (float)yPos - 15 }, { (float)xPos, (float)yPos - 3 }, pNormalBrush, 2.0f, pCloseStrokeStyle);
					pRenderTarget->DrawLine({ (float)xPos - 3, (float)yPos - 6 }, { (float)xPos, (float)yPos - 3 }, pNormalBrush, 2.0f, pCloseStrokeStyle);
					pRenderTarget->DrawLine({ (float)xPos + 3, (float)yPos - 6 }, { (float)xPos, (float)yPos - 3 }, pNormalBrush, 2.0f, pCloseStrokeStyle);
				}
				list[i]->errorstr = list[i]->funcptr->getErrorString();
				pRenderTarget->DrawText(&((list[i]->errorstr)[0]), list[i]->errorstr.size(), pErrorFormat, errorPos, pNormalBrush);
			}
			//draw value if variable count is 0 (so the value is constant)
			else if (list[i]->funcptr->state.variables.size() == 0 && list[i]->funcstr.size() > 0)
			{
				errorStr = std::to_wstring(list[i]->funcptr->evalFunc(NULL));
				int pointPos = errorStr.find(L'.');
				if (pointPos != errorStr.npos)
				{
					int ok = 1, i;
					for (i = pointPos + 1; i < errorStr.size()&&ok; i++)
						if (errorStr[i] != L'0') ok = 0;
					if (ok) errorStr.erase(pointPos, errorStr.size() - i - 1);
				}
				D2D1_RECT_F valuePos = D2D1::RectF(indentWidth + delimWidth, bottom - 15, clientSize.right, bottom);
				pNormalBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
				pRenderTarget->DrawText(&errorStr[0], errorStr.size(), pErrorFormat, valuePos, pNormalBrush);
			}


			//draw right margin mask
			D2D1_RECT_F marginMask = D2D1::RectF(clientSize.right - 2 * closeMargin - closeSize, top, clientSize.right, bottom);
			pNormalBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
			pRenderTarget->FillRectangle(marginMask, pNormalBrush);

			//draw indent delimiter
			pRenderTarget->DrawLine({ (float)indentWidth+delimWidth/2.f,(float)top-2 }, { (float)indentWidth + delimWidth / 2.f,(float)bottom+2 }, pDelimBrush, delimWidth/2.f);

			//draw top delimiter
			pRenderTarget->DrawLine({ 0, (float)top - delimHeight / 2.f }, { (float)clientSize.right, (float)top - delimHeight / 2.f }, pDelimBrush, delimHeight/2.f);

			//draw bottom delimiter
			pRenderTarget->DrawLine({0, (float)bottom+delimHeight/2.f}, {(float)clientSize.right, (float)bottom+delimHeight/2.f}, pDelimBrush, delimHeight/2.f);

			//draw close button
			pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			D2D1_RECT_F closeRect = D2D1::RectF((float)(clientSize.right - closeMargin - closeSize), (float)(top+closeMargin), (float)(clientSize.right - closeMargin), (float)(top + 2 * closeMargin + closeSize));

			pRenderTarget->DrawLine({ closeRect.left, closeRect.top }, { closeRect.right, closeRect.bottom }, pCloseLineBrush, 2.5f, pCloseStrokeStyle);
			pRenderTarget->DrawLine({ closeRect.left, closeRect.bottom }, { closeRect.right, closeRect.top }, pCloseLineBrush, 2.5f, pCloseStrokeStyle);
		}

		//draw new function text
		if (bottom < clientSize.bottom && targetIndex == -1)
		{
			static const std::wstring newFunctionStr = L"Funcție nouă...";
			static const std::wstring tooManyFunctionsStr = L"Numărul maxim de funcții (100) atins";

			auto& chosenStr = listLength == 100 ? tooManyFunctionsStr : newFunctionStr;
			pNormalBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
			D2D1_RECT_F textPos = D2D1::RectF(closeMargin, bottom + emptyItemHeight/2-20, clientSize.right, clientSize.bottom);
			pRenderTarget->DrawText(&chosenStr[0], chosenStr.size(), pNewFormat, textPos, pNormalBrush);
		}

		hr = pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardGraphicsResources();

		EndPaint(m_hwnd, &ps);

		for (int i = 0; i < listLength; i++)
			if(i!=targetIndex||currentTarget!=targetType::moveItem)
				RedrawWindow(list[i]->editHwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE | RDW_NOINTERNALPAINT);
		if(currentTarget==targetType::moveItem) 
			RedrawWindow(grabbedWin.m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE | RDW_INTERNALPAINT);

		return 0;
	}
	case WM_SIZE:
	{
		GetClientRect(m_hwnd, &clientSize);
		if (pRenderTarget == NULL) return 0;

		D2D1_SIZE_U size = D2D1::SizeU(clientSize.right, clientSize.bottom);
		pRenderTarget->Resize(size);

		resizeScrollbar();
		repositionEditBoxes();
		RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		return 0;
	}
	case WM_VSCROLL:
	{
		int msg = LOWORD(wParam), pos;
		switch (msg)
		{
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			pos = HIWORD(wParam);
			break;
		case SB_PAGEDOWN:
			pos = currScrollPos + emptyItemHeight;
			break;
		case SB_PAGEUP:
			pos = currScrollPos - emptyItemHeight;
			break;
		case SB_LINEDOWN:
			pos = currScrollPos + emptyItemHeight / 4;
			break;
		case SB_LINEUP:
			pos = currScrollPos - emptyItemHeight / 4;
			break;
		default: return 0;
		}

		if (timerSet)
		{
			timerSet = scrollDirection = 0;
			scrollAccel = scrollVelocity = 0;
			KillTimer(m_hwnd, 2002);
		}

		SCROLLINFO scrollInfo = { 0 };
		scrollInfo.cbSize = sizeof(SCROLLINFO);
		scrollInfo.fMask = SIF_ALL;
		GetScrollInfo(m_hwnd, SB_VERT, &scrollInfo);
		if (pos < 0) pos = 0;
		if (pos > (int)(scrollInfo.nMax - scrollInfo.nPage)) pos = max(0, (int)(scrollInfo.nMax - scrollInfo.nPage + 1));

		currScrollPos = pos;

		scrollInfo.fMask = SIF_POS;
		scrollInfo.nPos = pos;
		SetScrollInfo(m_hwnd, SB_VERT, &scrollInfo, true);

		if (targetIndex != -1 && currentTarget == targetType::moveItem) handleGrabbedItem();
		else repositionEditBoxes();
		
		RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE | RDW_NOCHILDREN);
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		if (currentTarget != targetType::none) return 0; //how?
		SetFocus(m_hwnd);
		int mousePos = GET_Y_LPARAM(lParam) + currScrollPos;
		//lastMousePos = mousePos - currScrollPos;
		int itemIndex = 0, currHeight = 0;
		int newHeight = 0;
		if (listLength)
			newHeight = list[listLength - 1]->currHeight + list[listLength - 1]->height + delimHeight;
		if (mousePos > newHeight)
		{
			if (mousePos > newHeight + emptyItemHeight)
				return 0; //random click
			else
			{
				//need to make new item
				currentTarget = targetType::newItem;
				SetCapture(m_hwnd);
				return 0;
			}
		}

		for (itemIndex = listLength - 1; itemIndex >= 0; itemIndex--)
			if (list[itemIndex]->currHeight <= mousePos) break;
		if (itemIndex < 0) return 0; //how??
		
		currHeight = list[itemIndex]->currHeight;
		targetIndex = itemIndex;
		int xPos = GET_X_LPARAM(lParam);
		mousePos -= currHeight;
		hitTestResult hit = performHitTest(xPos, mousePos, list[itemIndex]->height);
		if (hit == hitTestResult::Color || hit == hitTestResult::Visibility)
		{
			if ((list[itemIndex]->funcptr->state.variables.size() != 1 && !list[itemIndex]->funcptr->isSpecialFunction()) || list[itemIndex]->funcstr == L"" || list[itemIndex]->funcptr->state.eroareFunctie.errorValue != 0)
				hit = hitTestResult::Move;
		}
		switch (hit)
		{
			using enum hitTestResult;
		case Move:
		{
			currentTarget = targetType::moveItem;
			initialIndex = itemIndex;
			list[itemIndex]->grabbed = 1;
			grabbedFrom = mousePos;
			SetCapture(m_hwnd);
			handleGrabbedItem(xPos, GET_Y_LPARAM(lParam));
			RedrawWindow(m_hwnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_NOCHILDREN);
			break;
		}
		case Close:
		{
			currentTarget = targetType::closeButton;
			SetCapture(m_hwnd);
			break;
		}
		case Visibility:
		{
			currentTarget = targetType::visibilityToggle;
			SetCapture(m_hwnd);
			break;
		}
		case Color:
		{
			currentTarget = targetType::colorPicker;
			SetCapture(m_hwnd);
			break;
		}
		default:
			targetIndex = -1;
		}
		return 0;
	}
	case WM_SETCURSOR:
	{
		if (currentTarget != targetType::none) return 1;
		
		POINT cursorPos;
		GetCursorPos(&cursorPos);
		ScreenToClient(m_hwnd, &cursorPos);
		int xPos = cursorPos.x;
		int mousePos = cursorPos.y + currScrollPos;
		int itemIndex = 0, currHeight = 0;
		int newHeight = 0;
		if (listLength)
			newHeight = list[listLength - 1]->currHeight + list[listLength - 1]->height + delimHeight;
		if (mousePos > newHeight)
		{
			if (mousePos > newHeight + emptyItemHeight)
				SetCursor(LoadCursor(NULL, IDC_ARROW)); //mouse over nothing
			else
				SetCursor(LoadCursor(NULL, IDC_HAND));  //mouse over new function button
			return 0;
		}

		for (itemIndex = listLength - 1; itemIndex >= 0; itemIndex--)
			if (list[itemIndex]->currHeight <= mousePos) break;
		if (itemIndex < 0) return 0; //how??

		currHeight = list[itemIndex]->currHeight;
		mousePos -= currHeight;
		hitTestResult hit = performHitTest(xPos, mousePos, list[itemIndex]->height);
		if (hit == hitTestResult::Color || hit == hitTestResult::Visibility)
		{
			if ((list[itemIndex]->funcptr->state.variables.size()!=1 && !list[itemIndex]->funcptr->isSpecialFunction())  || list[itemIndex]->funcstr == L"" || list[itemIndex]->funcptr->state.eroareFunctie.errorValue != 0)
				hit = hitTestResult::Move;
		}
		switch (hit)
		{
			using enum hitTestResult;
		case Move:
			SetCursor(LoadCursor(NULL, IDC_SIZEALL));
			return 1;
		case Visibility:
		case Color:
		case Close:
			SetCursor(LoadCursor(NULL, IDC_HAND));
			return 1;
		case None:
			SetCursor(LoadCursor(NULL, IDC_ARROW));
			return 1;
		default:
			break;
		}
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
	case WM_LBUTTONUP:
	{
		if (currentTarget == targetType::none) return 0;
		ReleaseCapture();
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		targetType target = currentTarget;
		currentTarget = targetType::none;
		int xPos, yPos;
		xPos = GET_X_LPARAM(lParam);
		yPos = GET_Y_LPARAM(lParam);
		int mousePos = yPos + currScrollPos;
		int itemIndex = 0, currHeight = 0;
		int newHeight = 0;
		if (listLength)
			newHeight = list[listLength - 1]->currHeight + list[listLength - 1]->height + delimHeight;
		
		for (itemIndex = listLength - 1; itemIndex >= 0; itemIndex--)
			if (list[itemIndex]->currHeight <= mousePos) break;
		if (itemIndex < 0 && listLength) itemIndex = 0; //how??

		if (target != targetType::moveItem && target != targetType::newItem)
			if (itemIndex != targetIndex || mousePos > newHeight) target = targetType::none;

		currHeight = list[itemIndex]->currHeight;
		hitTestResult hit = performHitTest(xPos, mousePos-currHeight, list[itemIndex]->height);


		switch (target)
		{
			using enum targetType;
		case closeButton:
		{
			if (hit != hitTestResult::Close) break;
			int h = list[targetIndex]->height + delimHeight;
			delete list[targetIndex];
			for (int i = targetIndex; i < listLength - 1; i++)
			{
				list[i] = list[i + 1];
				list[i]->changeHeight(list[i]->currHeight - h);
			}
			listLength--;
			resizeScrollbar();
			repositionEditBoxes();
			globalGraphContainerWindow->itemChanged(target, targetIndex, 0);
			break;
		}
		case moveItem:
		{
			list[targetIndex]->grabbed = 0;
			SetWindowPos(list[targetIndex]->editHwnd, 0, indentWidth + 5, list[targetIndex]->currHeight-currScrollPos + emptyItemHeight / 3, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
			grabbedFrom = 0;
			ShowWindow(grabbedWin.m_hwnd, SW_HIDE);

			globalGraphContainerWindow->itemChanged(target, initialIndex, targetIndex);
			initialIndex = -1;
			break;
		}
		case newItem:
		{
			if (listLength == 100 || mousePos > newHeight + emptyItemHeight) return 0;

			int height = 0;
			if(listLength)
				height = list[listLength-1]->currHeight + delimHeight + list[listLength-1]->height;

			list[listLength] = new functionListItem();
			list[listLength]->parentHwnd = m_hwnd;
			globalGraphContainerWindow->itemChanged(target, listLength , 0);
			list[listLength]->activateItem(height, clientSize.right - clientSize.left, listLength, hFont, m_hwnd);

			height += list[listLength]->height + delimHeight;
			height += emptyItemHeight + delimHeight;
			currScrollPos = height;
			listLength++;
			resizeScrollbar();
			repositionEditBoxes();
			SetFocus(list[listLength - 1]->editHwnd);
			break;
		}
		case visibilityToggle:
		{
			if (hit != hitTestResult::Visibility) break;
			if ((list[targetIndex]->funcptr->state.variables.size() != 1 && !list[targetIndex]->funcptr->isSpecialFunction()) || list[targetIndex]->funcstr == L"" || list[targetIndex]->funcptr->state.eroareFunctie.errorValue != 0) break;
			list[targetIndex]->props.visible = 1 - list[targetIndex]->props.visible;
			globalGraphContainerWindow->itemChanged(target, targetIndex, 0);
			break;
		}
		case colorPicker:
		{
			if (hit != hitTestResult::Color) break;
			if ((list[targetIndex]->funcptr->state.variables.size() != 1 && !list[targetIndex]->funcptr->isSpecialFunction()) || list[targetIndex]->funcstr == L"" || list[targetIndex]->funcptr->state.eroareFunctie.errorValue != 0) break;
			auto& color = list[targetIndex]->props.color;
			CHOOSECOLOR strct = { 0 };
			strct.Flags = CC_FULLOPEN | CC_RGBINIT;
			strct.hwndOwner = m_hwnd;
			strct.hInstance = NULL;
			strct.rgbResult = RGB(color.r*255, color.g*255, color.b*255);
			strct.lpCustColors = custColors;
			strct.lStructSize = sizeof(strct);

			bool res = ChooseColor(&strct);

			if (res)
				color = { GetRValue(strct.rgbResult) / 255.f, GetGValue(strct.rgbResult) / 255.f, GetBValue(strct.rgbResult) / 255.f, 1 };

			globalGraphContainerWindow->itemChanged(target, targetIndex, 0);
			break;
		}
		default:
			break;
		}
		focusedIndex = targetIndex = -1;
		
		RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		return 0;
	}
	case WM_MOUSEWHEEL:
	{
		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam) / 120, prevDir = scrollDirection;
		scrollDirection = abs(zDelta) / zDelta;
		if (prevDir * scrollDirection < 0)
		{
			scrollAccel = scrollVelocity = 0;
		}
		else scrollAccel = min(800, scrollAccel + abs(zDelta)*20);
		if (!timerSet)
		{
			SetTimer(m_hwnd, 2002, 10, NULL);
			timerSet = 1;
		}
			timerScroll();
		//}
		return 0;
	}
	case WM_TIMER:
	{
		if (wParam == 1111) { resizeScrollbar(); KillTimer(m_hwnd, 1111); return 0; }
		timerScroll();
		return 0;
	}
	case WM_ERASEBKGND:
	{
		return 1;
	}
	case WM_KEYDOWN:
	{
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
	case WM_MOUSEMOVE:
	{
		//fout << "entered\n";
		if (currentTarget != targetType::moveItem) return 0;
		if (GET_Y_LPARAM(lParam) == lastPos) return 0;
		lastPos = GET_Y_LPARAM(lParam);
		handleGrabbedItem(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
		RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE | RDW_NOCHILDREN);
		//fout << "exited\n";
		return 0;
	}
	default:
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
}



void functionListWindow::resizeScrollbar()
{
	int tmp = currScrollPos;

	SCROLLINFO scrollInfo = { 0 };
	scrollInfo.cbSize = sizeof(SCROLLINFO);
	scrollInfo.fMask = SIF_ALL;
	scrollInfo.nMin = 0;
	scrollInfo.nMax = emptyItemHeight + delimHeight;
	for (int i = 0; i < listLength; i++)
		scrollInfo.nMax += list[i]->height + delimHeight;
	scrollInfo.nPage = (clientSize.bottom - clientSize.top);
	if (currScrollPos > (int)(scrollInfo.nMax-scrollInfo.nPage)) currScrollPos = max(0, (int)(scrollInfo.nMax - scrollInfo.nPage));
	scrollInfo.nPos = currScrollPos;
	scrollInfo.nTrackPos = 0;
	SetScrollInfo(m_hwnd, SB_VERT, &scrollInfo, true);
	
}


void functionListWindow::repositionEditBoxes()
{

	HDWP deferStruct = BeginDeferWindowPos((targetIndex!=-1 && currentTarget == targetType::moveItem) ? listLength-1:listLength);
	int height = -currScrollPos;
	for (int i = 0; i < listLength; i++)
	{
		if (!(targetIndex == i && currentTarget == targetType::moveItem))
			deferStruct = DeferWindowPos(deferStruct, list[i]->editHwnd, HWND_BOTTOM, indentWidth + 5, height + emptyItemHeight / 3, clientSize.right - indentWidth - delimWidth - 2 * closeMargin - closeSize - 5, emptyItemHeight / 3, SWP_SHOWWINDOW);
		height += delimHeight + list[i]->height;
	}
	EndDeferWindowPos(deferStruct);
		
}


HRESULT functionListWindow::CreateGraphicsResources()
{
	HRESULT hr = baseWindowDrawable<functionListWindow>::CreateGraphicsResources();
	if (FAILED(hr)) return hr;
	if (pNormalBrush == NULL)
	{
		hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkSlateGray), &pNormalBrush);
		if (FAILED(hr)) return hr;
	}
	if (pDelimBrush == NULL)
	{
		hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &pDelimBrush);
		if (FAILED(hr)) return hr;
	}
	if (pCloseLineBrush == NULL)
	{
		hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0.1f, 0.1f, 0.1f), &pCloseLineBrush);
		if (FAILED(hr)) return hr;
	}
	if (showBitmap == NULL)
	{
		hr = LoadResourceBitmap(pRenderTarget, IDB_PNG2, &showBitmap);
		if (FAILED(hr)) return hr;
	}
	if (colorPickerBitmap == NULL)
	{
		hr = LoadResourceBitmap(pRenderTarget, IDB_PNG1, &colorPickerBitmap);
		if (FAILED(hr)) return hr;
	}
	if (invertedShow == NULL)
	{
		hr = LoadResourceBitmap(pRenderTarget, IDB_PNG4, &invertedShow);
		if (FAILED(hr)) return hr;
	}
	if (invertedPicker == NULL)
	{
		hr = LoadResourceBitmap(pRenderTarget, IDB_PNG3, &invertedPicker);
		if (FAILED(hr)) return hr;
	}
	return hr;
}

void functionListWindow::DiscardGraphicsResources()
{
	SafeRelease(&pCloseLineBrush);
	SafeRelease(&pNormalBrush);
	SafeRelease(&pDelimBrush);
	SafeRelease(&invertedPicker);
	SafeRelease(&invertedShow);
	SafeRelease(&colorPickerBitmap);
	SafeRelease(&showBitmap);
	baseWindowDrawable<functionListWindow>::DiscardGraphicsResources();
}

void functionListWindow::timerScroll()
{
	if (!timerSet) { scrollVelocity = scrollAccel = 0; scrollDirection = 0; KillTimer(m_hwnd, 2002); return; }
	int pos;

	currScrollPos -= (int)round(scrollVelocity * scrollDirection);

	if (scrollVelocity + scrollAccel <= 0)
	{
		scrollVelocity = scrollAccel = 0; scrollDirection = 0;
		KillTimer(m_hwnd, 2002);
		timerSet = 0;
	}
	else
	{
		scrollVelocity = min(60, scrollVelocity + scrollAccel);
		//scrollVelocity += scrollAccel;
		if (scrollAccel > 0) scrollAccel = 0;
		else if (-scrollAccel >= scrollVelocity) scrollAccel = min(-1, -scrollVelocity / 1.5f);
		else scrollAccel -= 1.8f;
	}

	pos = currScrollPos;
	SCROLLINFO scrollInfo = { 0 };
	scrollInfo.cbSize = sizeof(SCROLLINFO);
	scrollInfo.fMask = SIF_ALL;
	GetScrollInfo(m_hwnd, SB_VERT, &scrollInfo);
	if (pos < 0)
		pos = 0;
	if (pos > (int)(scrollInfo.nMax - scrollInfo.nPage))
		pos = max(0, (int)(scrollInfo.nMax - scrollInfo.nPage));
	if (pos != currScrollPos)
	{
		scrollVelocity = scrollAccel = 0;  scrollDirection = 0;
		KillTimer(m_hwnd, 2002);
		timerSet = 0;
	}
	currScrollPos = pos;

	scrollInfo.fMask = SIF_POS;
	scrollInfo.nPos = pos;
	SetScrollInfo(m_hwnd, SB_VERT, &scrollInfo, true);
	if (targetIndex != -1 && currentTarget == targetType::moveItem) handleGrabbedItem();
	else repositionEditBoxes();

	RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
}



void functionListWindow::handleGrabbedItem()
{
	POINT cursorPos;
	GetCursorPos(&cursorPos);
	ScreenToClient(m_hwnd, &cursorPos);
	handleGrabbedItem(cursorPos.x, cursorPos.y);
}

void functionListWindow::handleGrabbedItem(int x, int y)
{
	if (targetIndex < 0 || targetIndex > listLength || listLength == 0) return; //how?
	int newIndex = 0, mousePos = y + currScrollPos;

	for (newIndex = listLength-1; newIndex >= 0; newIndex--)
		if (list[newIndex]->currHeight <= mousePos) break;

	if (newIndex < 0) newIndex = 0;

	functionListItem* tmp = list[targetIndex];
	int destHeight = list[newIndex]->currHeight;
	if (newIndex > targetIndex)
	{
		int prevHeight = list[targetIndex]->currHeight;
		for (int i = targetIndex; i < newIndex; i++)
		{
			list[i] = list[i + 1];
			list[i]->changeHeight(prevHeight);
			prevHeight += list[i]->height + delimHeight;
		}
	}
	else if (newIndex < targetIndex)
	{
		for (int i = targetIndex; i > newIndex; i--)
		{
			list[i] = list[i - 1];
			list[i]->changeHeight(list[i]->currHeight + list[i]->height + delimHeight);
		}
	}

	list[newIndex] = tmp;
	list[newIndex]->changeHeight(destHeight);
	list[newIndex]->grabbedHeight = mousePos-grabbedFrom;
	list[newIndex]->changeHeight(list[newIndex]->currHeight, currScrollPos);
	targetIndex = newIndex;

	SetWindowPos(grabbedWin.m_hwnd, HWND_TOP, 0, y-grabbedFrom-delimHeight/2-1, clientSize.right, emptyItemHeight + 1.5f * delimHeight, SWP_SHOWWINDOW);
	BringWindowToTop(grabbedWin.m_hwnd);

	SetWindowPos(list[targetIndex]->editHwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	BringWindowToTop(list[targetIndex]->editHwnd);

	repositionEditBoxes();
}

hitTestResult functionListWindow::performHitTest(int x, int y, int height)
{
	if (x <= indentWidth+delimWidth)
	{
		if (!(y >= height - closeMargin - closeSize && y <= height - closeMargin)) return hitTestResult::Move;
		if (x >= closeMargin && x <= closeMargin + closeSize) return hitTestResult::Color;
		if (x >= 3 * closeMargin + closeSize && x <= 3 * closeMargin + 2 * closeSize) return hitTestResult::Visibility;
		return hitTestResult::Move;
	}
	if (x >= clientSize.right - closeSize - closeMargin && x <= clientSize.right - closeMargin && y <= closeSize + closeMargin && y >= closeMargin) return hitTestResult::Close;
	if (x >= indentWidth + delimWidth + 5 && x <= clientSize.right - 2 * closeMargin - closeSize && y>=emptyItemHeight / 3 && y<=emptyItemHeight/3+emptyItemHeight/3) return hitTestResult::Edit;
	return hitTestResult::None;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

HRESULT grabbedWindow::CreateGraphicsResources()
{
	HRESULT hr = baseWindowDrawable<grabbedWindow>::CreateGraphicsResources();
	if (FAILED(hr)) return hr;
	pRenderTarget->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
	if (pNormalBrush == NULL)
	{
		hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::DarkSlateGray), &pNormalBrush);
		if (FAILED(hr)) return hr;
	}
	if (pDelimBrush == NULL)
	{
		hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(0, 0, 0), &pDelimBrush);
		if (FAILED(hr)) return hr;
	}
	return hr;
}

void grabbedWindow::DiscardGraphicsResources()
{
	SafeRelease(&pNormalBrush);
	SafeRelease(&pDelimBrush);
	baseWindowDrawable<grabbedWindow>::DiscardGraphicsResources();
}


LRESULT grabbedWindow::procMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{

	case WM_PAINT:
	{
		HRESULT hr = CreateGraphicsResources();
		if (FAILED(hr) || !parent) return 0;

		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);

		const auto item = parent->list[targetIndex];

		int top = delimHeight-1;
		int bottom = top + item->height;

		static std::wstring numStr, errorStr;
		if (!(bottom + delimHeight < 0 || top >= clientSize.bottom))
		{
			pRenderTarget->BeginDraw();
			pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

			D2D1_COLOR_F textColor = D2D1::ColorF(D2D1::ColorF::Black);
			//draw indent background if function visible and not errored
			if (item->funcptr->state.eroareFunctie.errorType == ErrorType::NoError && item->funcptr->state.variables.size() == 1 && item->props.visible)
			{
				const auto& itemColor = item->props.color;
				pNormalBrush->SetColor(itemColor);
				D2D1_RECT_F indentBackground = D2D1::RectF(0, (float)top - 1, (float)indentWidth + 1, (float)bottom + 1);
				pRenderTarget->DrawRectangle(indentBackground, pNormalBrush);
				pRenderTarget->FillRectangle(indentBackground, pNormalBrush);

				//https://stackoverflow.com/questions/596216/formula-to-determine-perceived-brightness-of-rgb-color
				textColor = (0.299f * itemColor.r + 0.587f * itemColor.g + 0.114f * itemColor.b) >= 0.5f ? D2D1::ColorF(D2D1::ColorF::Black) : D2D1::ColorF(D2D1::ColorF::White);
			}

			//draw index number
			numStr = std::to_wstring(targetIndex + 1);
			D2D1_RECT_F numPos = D2D1::RectF(closeMargin, top, indentWidth, bottom);
			pNormalBrush->SetColor(textColor);
			pRenderTarget->DrawText(&numStr[0], numStr.size(), parent->pNormalFormat, numPos, pNormalBrush);


			//draw error text
			if (item->funcptr->state.eroareFunctie.errorType != ErrorType::NoError && item->funcstr.size() > 0)
			{
				//errorStr = item->funcptr->getErrorString();
				D2D1_RECT_F errorPos = D2D1::RectF(indentWidth + delimWidth, bottom - 15, clientSize.right, bottom);
				pNormalBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));


				//draw error indicator, if necessary
				eroare& err = item->funcptr->state.eroareFunctie;
				if (errorIsPositional(err))
				{
					auto errorPos = addWhitespace(item->realstr, err.errorInfo);
					LRESULT rez = SendMessage(item->editHwnd, EM_POSFROMCHAR, errorPos, 0);
					int xPos = LOWORD(rez) + indentWidth + delimWidth + 5, yPos = HIWORD(rez) + top + emptyItemHeight / 3;

					pRenderTarget->DrawLine({ (float)xPos, (float)yPos - 15 }, { (float)xPos, (float)yPos - 3 }, pNormalBrush, 2.0f, parent->pCloseStrokeStyle);
					pRenderTarget->DrawLine({ (float)xPos - 3, (float)yPos - 6 }, { (float)xPos, (float)yPos - 3 }, pNormalBrush, 2.0f, parent->pCloseStrokeStyle);
					pRenderTarget->DrawLine({ (float)xPos + 3, (float)yPos - 6 }, { (float)xPos, (float)yPos - 3 }, pNormalBrush, 2.0f, parent->pCloseStrokeStyle);
				}

				pRenderTarget->DrawText(&((item->errorstr)[0]), item->errorstr.size(), parent->pErrorFormat, errorPos, pNormalBrush);

			}
			
			//draw value is variable count is 0 (so the value is constant)
			else if (item->funcptr->state.variables.size() == 0 && item->funcstr.size() > 0)
			{
				errorStr = std::to_wstring(item->funcptr->evalFunc(NULL));
				int pointPos = errorStr.find(L'.');
				if (pointPos != errorStr.npos)
				{
					int ok = 1, i;
					for (i = pointPos + 1; i < errorStr.size() && ok; i++)
						if (errorStr[i] != L'0') ok = 0;
					if (ok) errorStr.erase(pointPos, errorStr.size() - i - 1);
				}
				D2D1_RECT_F errorPos = D2D1::RectF(indentWidth + delimWidth, bottom - 15, clientSize.right, bottom);
				pNormalBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
				pRenderTarget->DrawText(&errorStr[0], errorStr.size(), parent->pErrorFormat, errorPos, pNormalBrush);
			}

			//draw right margin mask
			D2D1_RECT_F marginMask = D2D1::RectF(clientSize.right - 2 * closeMargin - closeSize, top, clientSize.right, bottom);
			pNormalBrush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
			pRenderTarget->FillRectangle(marginMask, pNormalBrush);

			//draw indent delimiter
			pRenderTarget->DrawLine({ (float)indentWidth + delimWidth / 2.f,(float)top - 2 }, { (float)indentWidth + delimWidth / 2.f,(float)bottom + 2 }, pDelimBrush, delimWidth/2.f);

			//draw top delimiter
			pRenderTarget->DrawLine({ 0, (float)top - delimHeight / 2.f }, { (float)clientSize.right, (float)top - delimHeight / 2.f }, pDelimBrush, delimHeight/2.f);

			//draw bottom delimiter
			pRenderTarget->DrawLine({ 0, (float)bottom + delimHeight / 2.f }, { (float)clientSize.right, (float)bottom + delimHeight / 2.f }, pDelimBrush, delimHeight/2.f);


			hr = pRenderTarget->EndDraw();
			if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardGraphicsResources();
		}

		RedrawWindow(item->editHwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE | RDW_NOINTERNALPAINT);
		EndPaint(m_hwnd, &ps);

		return 0;
	}
	case WM_ERASEBKGND:
	{
		return 1;
	}
	case WM_SIZE:
	{
		GetClientRect(m_hwnd, &clientSize);
		if (pRenderTarget == NULL) return 0;
		D2D1_SIZE_U size = D2D1::SizeU(clientSize.right, clientSize.bottom);
		pRenderTarget->Resize(size);
		RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		return 0;
	}
	default: return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
}
