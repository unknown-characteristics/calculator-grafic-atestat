// atestat.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "atestat.h"
#include "mainWindow.h"
#include "math.h"
#include <cstdlib>
#include <ctime>

ID2D1Factory* pFactory = NULL;
HINSTANCE hInst = NULL;
IDWriteFactory* pDWFactory = NULL;
IWICImagingFactory* pWICFactory = NULL;


int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);
    hInst = hInstance;

    //seed rand
    srand((unsigned)time(0));
    

    //timerProc
    BOOL val = false;
    SetUserObjectInformation(GetCurrentProcess(), UOI_TIMERPROC_EXCEPTION_SUPPRESSION, &val, sizeof(val));

    //init mate
    initConstante();
    
    //init common controls
    INITCOMMONCONTROLSEX prop = { 0 };
    prop.dwICC = ICC_STANDARD_CLASSES;
    prop.dwSize = sizeof(prop);
    if (!InitCommonControlsEx(&prop)) 
        return -1;

    //init direct2d
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
    {
        return -1;
    }
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWFactory))))
    {
        SafeRelease(&pFactory);
        return -1;
    }

    //init wic
    if (FAILED(CoInitialize(NULL)))
    {
        SafeRelease(&pDWFactory);
        SafeRelease(&pFactory);
        return -1;
    }
    if (FAILED(CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWICFactory))))
    {
        SafeRelease(&pDWFactory);
        SafeRelease(&pFactory);
        return -1;
    }

    //init main window
    mainWindow* mainWin = new mainWindow();
    if (!mainWin)
    {
        SafeRelease(&pDWFactory);
        SafeRelease(&pFactory);
        return -1;
    }

    if (!mainWin->Create(hInst, L"Calculator grafic", WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, 0, 0, LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU1)), CW_USEDEFAULT, CW_USEDEFAULT, 900, 700))
    {
        delete mainWin;
        SafeRelease(&pDWFactory);
        SafeRelease(&pFactory);
        return -1;
    }
    ShowWindow(mainWin->m_hwnd, nCmdShow);
    BringWindowToTop(mainWin->m_hwnd);

    //process messages
    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_ATESTAT));

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    delete mainWin;
    SafeRelease(&pDWFactory);
    SafeRelease(&pFactory);
    return (int) msg.wParam;
}
