#include "mainWindow.h"

PCWSTR baseWindow<mainWindow>::className = L"Main Window Class";

int mouseOverSplitter(int x, int splitterX, int splitterSize)
{
	return x >= splitterX && x <= splitterX + splitterSize;
}

int diff = 0;

wchar_t helpStr[] = L"Calculator grafic - lucrare de atestat\n\
Citește expresia unor funcții matematice și le trasează graficele în planul xOy.\n\n\
Aplicația acceptă cel mult 100 de funcții concomitent. O funcție poate fi definită în funcție de altă funcție \
(de exemplu, f(x)=x^2 și g(x) = f(x^3)), însă o funcție nu poate fi definită recursiv. Pe lângă funcțiile obișnuite, \
care au atât nume cât lista cu variabilele acceptate (cum sunt cele din exemplul anterior), aplicația permite și \
funcții \"degenerate\": cărora le lipsește cel puțin lista explicită de variabile. În cazul acestora, ele nu pot avea \
decât cel mult o variabila identificată de aplicație implicit, și dacă au nume, funcțiile care sunt definite în funcție \
de acestea trebuie să prezinte variabila respectivă în lista lor (sau să fie implicit a lor, dacă sunt degenerate).\
Un caz special sunt funcțiile fără variabile care au litera \"x\" sau \"y\"; ele nu sunt recunoscute prin acest nume \
în definițiile altor funcții și definesc drepte verticale, respectiv orizontale. Funcțiilor cu 0 sau mai mult de o variabilă \
nu li se reprezintă graficul, în schimb ele pot fi folosite în mod obișnuit în expresiile altor funcții.\n\n\
Pentru a face exprimarea unor funcții mai simplă, înmulțirea implicită (\"ab\" este înmulțire implicită între a și b) \
are prioritate față de aproape orice altă operație, fapt ce poate duce la interpretări neașteptate ale unor expresii matematice (1/ab este 1/(ab), nu (1/a) * b). \
Utilizarea parantezelor poate rezolva orice ambiguitate.\n\n\
Acuratețea graficelor se poate modifica din meniul Setări.\n\
Vizualizarea graficelor se face in partea dreapta a aplicatiei, după separator (care poate fi mutat). \
Pentru a vedea graficele pe alte intervale ale axelor Ox, Oy, \
se poate folosi mouse-ul prin click și săgețile de pe tastatură pentru a \"trage\" imaginea, sau rotița mouse-ului și \
+/- pentru a da zoom in/out. Apăsarea tastei Space resetează imaginea la poziția inițială, centrată pe punctul (0, 0), \
dar păstrează zoom-ul. Apăsarea tastei Space ținând Shift apăsat resetează atât poziția, cât și nivelul de zoom. Click-ul cu mouse-ul pe curba \
unei funcții evidențiază coordonatele punctului de pe curbă aflat sub mouse și permite deplasarea punctului de-a lungul curbei.";

wchar_t predefFuncStr[] = L"Lista funcțiilor predefinite recunoscute de aplicație\n\n\
sin, cos, tan, cot, arcsin, arccos, arctan, arccot\n\
floor, ceil, abs (abs(x) = |x|)\n\
pow, sqrt, cbrt, nthroot\n(pow(x,2) = x^2, nthroot(x,2) = pow(x, 1/2) = sqrt(x))\n\
ln, log (log(x,e) = ln(x))";

functionListWindow* globalFunctionListWindow = NULL;

graphContainerWindow* globalGraphContainerWindow = NULL;


LRESULT CALLBACK generalDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

LRESULT mainWindow::procMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
	{
		GetClientRect(m_hwnd, &clientSize);
		globalFunctionListWindow = &funcListWin;
		globalGraphContainerWindow = &graphConWin;
		if (!funcListWin.Create(hInst, L"t1", WS_CHILD | WS_VSCROLL | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, 0UL, m_hwnd, 0, 0, 0, 10, 10))
		{
			return -1;
		}
		ShowWindow(funcListWin.m_hwnd, SW_SHOWNORMAL);
		if (!graphConWin.Create(hInst, L"t2", WS_CHILD | WS_CLIPSIBLINGS, 0UL, m_hwnd, 0, 0, 0, 10, 10))
		{
			return -1;
		}
		ShowWindow(graphConWin.m_hwnd, SW_SHOWNORMAL);
		CreateGraphicsResources();
		SetLayout();

		CheckMenuRadioItem(GetMenu(m_hwnd), ID_ACURATE32782, ID_ACURATE32784, ID_ACURATE32782, MF_BYCOMMAND);

		return 0;
	}
	case WM_COMMAND:
	{
		int type = HIWORD(wParam);
		int id = LOWORD(wParam);
		if (type != 0) return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

		MENUITEMINFO strct = { 0 };
		strct.cbSize = sizeof(strct);
		strct.fMask = MIIM_STATE;
		GetMenuItemInfo(GetMenu(m_hwnd), id, FALSE, &strct);

		if (id == ID_INFORMA32779) //ajutor
		{
			MessageBox(m_hwnd, helpStr, L"Ajutor", 0);
		}
		else if (id == ID_INFORMA32780)
		{
			MessageBox(m_hwnd, predefFuncStr, L"Lista funcțiilor predefinite", 0);
		}
		else if (id == ID_INFORMA32785) //despre
		{
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), m_hwnd, generalDlgProc);
		}
		else if (id == ID_ACURATE32782 || id == ID_ACURATE32783 || id == ID_ACURATE32784)
		{
			CheckMenuRadioItem(GetMenu(m_hwnd), ID_ACURATE32782, ID_ACURATE32784, id, MF_BYCOMMAND);
			if (id == ID_ACURATE32782) pointsPerPixel = 1;
			else if (id == ID_ACURATE32783) pointsPerPixel = 2;
			else pointsPerPixel = 3;

			globalGraphContainerWindow->GraphAccuracyChanged();
		}
		else
		{
			bool state = (strct.fState & MFS_CHECKED) ? 1 : 0;
			bool& targetBool = (id == ID_DESPRE_SET32771 ? showMajorGridlines :
				id == ID_DESPRE_INFORMA32772 ? showMinorGridlines :
				id == ID_SET32773 ? showOX :
				id == ID_SET32774 ? showOY :
				id == ID_SET32775 ? isGraphClickable :
				id == ID_SET32777 ? showGridText : showGridText);

			state = 1 - state;
			targetBool = state;

			if (state) strct.fState = (strct.fState ^ MFS_UNCHECKED) | MFS_CHECKED;
			else strct.fState = (strct.fState ^ MFS_CHECKED) | MFS_UNCHECKED;

			int rez = SetMenuItemInfo(GetMenu(m_hwnd), id, FALSE, &strct);
		}
		DrawMenuBar(m_hwnd);
		RedrawWindow(funcListWin.m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		RedrawWindow(graphConWin.m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW | RDW_INVALIDATE);
		return 0;
		
	}
	case WM_DESTROY:
	{
		PostQuitMessage(0);
		return 0;
	}
	case WM_PAINT:
	{
		HRESULT hr = CreateGraphicsResources();
		if (FAILED(hr)) return 0;

		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);
		pRenderTarget->BeginDraw();
		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::Black));
		hr = pRenderTarget->EndDraw();

		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET) DiscardGraphicsResources();
		EndPaint(m_hwnd, &ps);
		return 0;
	}
	case WM_GETMINMAXINFO:
	{
		MINMAXINFO& strct = *reinterpret_cast<MINMAXINFO*>(lParam);
		strct.ptMinTrackSize = { 800, 600 };
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
		SetLayout();
		return 0;
	}
	case WM_LBUTTONDOWN:
	{
		SetFocus(m_hwnd);
		int xPos = GET_X_LPARAM(lParam);
		if (!mouseOverSplitter(xPos, splitterX, splitterSize)) return 0;
		SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		SetCapture(m_hwnd);
		capturing = 1;
		diff = xPos - splitterX;
		return 0;
	}
	case WM_MOUSEMOVE:
	{
		int xPos = GET_X_LPARAM(lParam);
		if (!capturing||clientSize.right==0) return 0;
		xPos -= diff;
		if (xPos < 350) xPos = 350;
		if (xPos > clientSize.right - splitterSize - 350) xPos = clientSize.right - splitterSize - 350;
		splitterPerc = (double)xPos / clientSize.right;
		SetLayout();
		return 0;
	}
	case WM_SETCURSOR:
	{
		int caz = LOWORD(lParam);
		if (caz != HTCLIENT) return DefWindowProc(m_hwnd, uMsg, wParam, lParam);

		POINT cursorPos;
		GetCursorPos(&cursorPos);
		ScreenToClient(m_hwnd, &cursorPos);
		int xPos = cursorPos.x;
		if (mouseOverSplitter(xPos, splitterX, splitterSize) || capturing)
			SetCursor(LoadCursor(NULL, IDC_SIZEWE));
		else
			return 0;
		return 1;
	}
	case WM_LBUTTONUP:
	{
		if (capturing)
			ReleaseCapture();
		capturing = 0;
		return 0;
	}
	case WM_KEYDOWN:
	{
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
	default:
		return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
	}
}


void mainWindow::SetLayout()
{
	if (isnan(splitterPerc) || !isfinite(splitterPerc)) splitterPerc = 0.33f;
	splitterX = max(350, (int) (clientSize.right * splitterPerc));
	splitterX = min(clientSize.right - splitterSize - 350, splitterX);


	HDWP deferStruct = BeginDeferWindowPos(2);
	deferStruct = DeferWindowPos(deferStruct, funcListWin.m_hwnd, HWND_BOTTOM, 0, 0, splitterX, clientSize.bottom, 0);
	deferStruct = DeferWindowPos(deferStruct, graphConWin.m_hwnd, HWND_TOP, splitterX + splitterSize + 2, 0, clientSize.right - splitterX - splitterSize - 1, clientSize.bottom, 0);
	EndDeferWindowPos(deferStruct);

	RedrawWindow(funcListWin.m_hwnd, NULL, NULL, RDW_UPDATENOW);
	RedrawWindow(graphConWin.m_hwnd, NULL, NULL, RDW_UPDATENOW);
	RedrawWindow(m_hwnd, NULL, NULL, RDW_UPDATENOW);
}

HRESULT mainWindow::CreateGraphicsResources()
{
	HRESULT hr = baseWindowDrawable<mainWindow>::CreateGraphicsResources();
	if (FAILED(hr)) return hr;
	const D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 0);
	hr = pRenderTarget->CreateSolidColorBrush(color, &pBrush);
	return hr;
}

void mainWindow::DiscardGraphicsResources()
{
	SafeRelease(&pBrush);
	baseWindowDrawable<mainWindow>::DiscardGraphicsResources();
}

LRESULT CALLBACK generalDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		if (wParam == IDOK ||
			wParam == IDCANCEL)
		{
			EndDialog(hwndDlg, TRUE);
			return 1;
		}
	}
	return 0;
}