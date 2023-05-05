// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
#include <windowsx.h>

#include <commctrl.h>
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32")

#include <commdlg.h>
#pragma comment(lib, "Comdlg32")

#include <wincodec.h>
#include <d2d1_1.h>
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include "d2dHelper.h"

#include <dwrite.h>
#pragma comment(lib, "dwrite")
// C RunTime Header Files
//#include <stdlib.h>
//#include <malloc.h>
//#include <memory.h>
#include <tchar.h>


//globals
extern HINSTANCE hInst;
extern ID2D1Factory* pFactory;
extern IDWriteFactory* pDWFactory;
extern IWICImagingFactory* pWICFactory;

extern bool showMajorGridlines, showMinorGridlines, showOX, showOY, showGridText, isGraphClickable;