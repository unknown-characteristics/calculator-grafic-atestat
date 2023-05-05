#pragma once
#include "framework.h"
#include "atestat.h"
template <class T>
class baseWindow
{
public:
    HWND m_hwnd;
    inline static BOOL classRegistered = 0;
    inline static WNDCLASSEX wcex = { 0 };
    static PCWSTR className;
    RECT clientSize = { 0, 0 };
    virtual LRESULT procMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

    BOOL Create(HINSTANCE hInstance, PCWSTR lpWindowName, DWORD dwStyle, DWORD dwExStyle = 0, HWND hWndParent = 0, HMENU hMenu = 0, int x = CW_USEDEFAULT, int y = CW_USEDEFAULT, int nWidth = CW_USEDEFAULT, int nHeight = CW_USEDEFAULT)
    {
        CreateWindowClass(hInstance);

        m_hwnd = CreateWindowEx(dwExStyle, className, lpWindowName, dwStyle, x, y, nWidth, nHeight, hWndParent, hMenu, hInstance, this);
        if (m_hwnd) return 1;
        else return 0;
    }
    baseWindow() : m_hwnd(NULL) {}
protected:
    
    static void CreateWindowClass(HINSTANCE hInstance)
    {
        if (classRegistered) return;

        wcex.style = CS_HREDRAW | CS_VREDRAW;
        wcex.hInstance = hInstance;
        wcex.lpszClassName = T::className;
        wcex.lpfnWndProc = T::winProc;
        wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ATESTAT));
        wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wcex.hbrBackground = NULL;
        wcex.cbSize = sizeof(WNDCLASSEX);
        if(!RegisterClassEx(&wcex)) return;

        classRegistered = TRUE;
    }
  
    static LRESULT CALLBACK winProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        T* pThis = NULL;
        if (uMsg == WM_NCCREATE)
        {
            CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
            pThis = (T*)pCreate->lpCreateParams;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            pThis->m_hwnd = hwnd;
        }
        else
        {
            pThis = (T*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
        }
        if (pThis)
            return pThis->procMsg(uMsg, wParam, lParam);
        else
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
};




template <class T>
class baseWindowDrawable : public baseWindow<T>
{
public:
    ID2D1HwndRenderTarget* pRenderTarget;
    


    HRESULT CreateRenderTarget()
    {
        HRESULT hr = S_OK;
        if (pRenderTarget == NULL)
        {

            D2D1_SIZE_U size = D2D1::SizeU(this->clientSize.right, this->clientSize.bottom);

            hr = pFactory->CreateHwndRenderTarget(
                D2D1::RenderTargetProperties(),
                D2D1::HwndRenderTargetProperties(this->m_hwnd, size),
                &pRenderTarget);
        }
        return hr;

    }
    baseWindowDrawable() : pRenderTarget(NULL) {}
    ~baseWindowDrawable()
    {
        baseWindowDrawable<T>::DiscardGraphicsResources();
    }
    virtual HRESULT CreateGraphicsResources()
    {
        return CreateRenderTarget();
    }
    virtual void DiscardGraphicsResources()
    {
        SafeRelease(&pRenderTarget);
    }
};