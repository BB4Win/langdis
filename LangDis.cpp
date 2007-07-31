#include <BBApi.h>

#ifndef BB_DREDRAW
#define BB_REDRAW 10610
#endif

LPCSTR szClassName = "LangDisClass";
LPCSTR szWindowName = "LangDisWindow";
const int nMessages[] = { BB_RECONFIGURE, BB_BROADCAST, BB_ADDTASK, BB_REMOVETASK, BB_ACTIVATESHELLWINDOW, BB_ACTIVETASK, BB_MINMAXTASK, BB_REDRAW };

class LangDis {
private:
	HINSTANCE m_hInstance;
	HWND m_hParent;
	HWND m_hWnd;
	char *m_szLanguage;

	HFONT m_hFont;
	COLORREF colorFrom;
	COLORREF colorTo;
	COLORREF borderColor;

	int nBorderWidth;
	int nBevelWidth;

	StyleItem *siStyle;

public:
	LangDis(HINSTANCE hi, HWND hw)
	{
		m_hInstance = hi;
		m_hParent = hw;
		m_szLanguage = NULL;
		siStyle = NULL;
		m_hFont = NULL;
	}

	~LangDis()
	{
		if (m_hParent)
			SendMessage(m_hParent, SLIT_REMOVE, NULL, (LPARAM)m_hWnd);

		SendMessage(GetBBWnd(), BB_UNREGISTERMESSAGE, (WPARAM)m_hWnd, (LPARAM)nMessages);
		DestroyWindow(m_hWnd);
		UnregisterClass(szClassName, m_hInstance);
	}

	BOOL Initialize()
	{
		if (!_RegisterClass())
			return FALSE;

		if (!_CreateWindow())
			return FALSE;

		SendMessage(GetBBWnd(), BB_REGISTERMESSAGE, (WPARAM)m_hWnd, (LPARAM)nMessages);
		
		GetStyleSettings();
		GetInfo();

		InvalidateRect(m_hWnd, NULL, TRUE);

		if (m_hParent)
			SendMessage(m_hParent, SLIT_ADD, NULL, (LPARAM)m_hWnd);

		return TRUE;
	}

	LPSTR PluginInfo(int field)
	{
		switch (field)
		{
			case PLUGIN_NAME: return "Language\\Locale Display";
			case PLUGIN_VERSION: return "1.0";
			case PLUGIN_AUTHOR: return "Brian \"Tres`ni\" Hartvigsen";
			case PLUGIN_RELEASEDATE: return "29 July 2007";
			case PLUGIN_EMAIL: return "tresni@crackmonkey.us";
			case PLUGIN_BROAMS: return "\003@langdis next@langdis prev\0@langdis next\0@langdis prev\0\0";
			case PLUGIN_LINK:
			case PLUGIN_UPDATE_URL:
			default:
				return "Language\\Locale Display v1.0 (29 July 2007)";
		}
	}

	//GetLocaleInfo(,LOCALE_SABBREVLANGNAME)
	//VerLanguageName <-- Get Language Names
	//GetKeyboardLayoutList <-- Get all layouts
	//GetTextExtentPoint32 <-- To measure the string

private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		static LangDis *pThis = NULL;
		if(uMessage == WM_CREATE)
			pThis = (LangDis *)((CREATESTRUCT *)(lParam))->lpCreateParams;

		return pThis->WindowProc(hWnd, uMessage, wParam, lParam);
	}

	LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMessage, WPARAM wParam, LPARAM lParam)
	{
		switch(uMessage)
		{

			case BB_RECONFIGURE:
				GetStyleSettings();

			case BB_ADDTASK:
			case BB_REMOVETASK:
			case BB_ACTIVATESHELLWINDOW:
			case BB_ACTIVETASK:
			case BB_MINMAXTASK:
			case BB_REDRAW:
				GetInfo();
				InvalidateRect(m_hWnd, NULL, TRUE);
				break;

			case WM_PAINT:
				OnPaint();
				return 0;

			case WM_CLOSE:
				break;
			case BB_BROADCAST:
				if (_strnicmp((char*)lParam, "@langdis", 8) == 0)
				{
					LPARAM hklMsg = -1;
					if (_strnicmp((char*)lParam + 9, "next", 4) == 0)
						hklMsg = HKL_NEXT;
					else if (_strnicmp((char*)lParam + 9, "prev", 4) == 0)
						hklMsg = HKL_PREV;

					if (hklMsg != -1)
					{
						HWND forWnd = GetForegroundWindow();
						if (forWnd)
							PostMessage(forWnd, WM_INPUTLANGCHANGEREQUEST, 0, (LPARAM)hklMsg);

						PostMessage(hWnd, BB_REDRAW, 0, 0);
					}
				}
				break;

			default:
				return DefWindowProc(hWnd, uMessage, wParam, lParam);
		}

		return TRUE;
	}

	void GetInfo()
	{
		HWND CurApp=GetForegroundWindow();
		DWORD idthd=GetWindowThreadProcessId(CurApp,NULL);
		HKL layid = GetKeyboardLayout(idthd);

		if (m_szLanguage != NULL)
			delete[] m_szLanguage;

		int l = GetLocaleInfo(MAKELCID(layid,0), LOCALE_SABBREVLANGNAME, NULL, NULL);

		m_szLanguage = new char[l];
		GetLocaleInfo(MAKELCID(layid, 0), LOCALE_SABBREVLANGNAME, m_szLanguage, l);

		HDC test = CreateCompatibleDC(NULL);		
		HFONT old = (HFONT)SelectObject(test, m_hFont);

		SIZE size;
		GetTextExtentPoint32(test, m_szLanguage, l, &size);

		SelectObject(test, old);
		DeleteDC(test);

		SetWindowPos(m_hWnd, 0, 0, 0, size.cx + siStyle->borderWidth + siStyle->marginWidth, size.cy + siStyle->borderWidth + siStyle->marginWidth, SWP_NOMOVE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
		if (m_hParent)
			SendMessage(m_hParent, SLIT_UPDATE, NULL, NULL);
	}

	void GetStyleSettings()
	{
		char styleFile[MAX_PATH];
#ifdef _MSC_VER
		strcpy_s(styleFile, MAX_PATH, stylePath());
#else
		strncpy(styleFile, stylePath(), MAX_PATH);
#endif

		siStyle = (StyleItem*)GetSettingPtr(m_hParent ? SN_TOOLBARWINDOWLABEL : SN_TOOLBAR);
		if (siStyle->parentRelative)
			siStyle = (StyleItem*)GetSettingPtr(SN_TOOLBAR);
		m_hFont = CreateStyleFont(siStyle);
	}

	void OnPaint()
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(m_hWnd, &ps);
		HDC buf = CreateCompatibleDC(NULL);
		RECT r;
	
		GetClientRect(m_hWnd, &r);

		HBITMAP bufbmp = CreateCompatibleBitmap(hdc, r.right, r.bottom);
		HBITMAP oldbuf = (HBITMAP)SelectObject(buf, bufbmp);
		MakeStyleGradient(buf, &r, siStyle, m_hParent ? false : true);

		HFONT oldfont = (HFONT)SelectObject(buf, m_hFont);
		SetTextColor(buf, siStyle->TextColor);

		SetBkMode(buf, TRANSPARENT);
		DrawText(buf, m_szLanguage, lstrlen(m_szLanguage), &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		SelectObject(buf, oldfont);

		BitBlt(hdc, 0, 0, r.right, r.bottom, buf, 0, 0, SRCCOPY);

		// Remember to delete all objects!
		SelectObject(buf, oldbuf);
		DeleteDC(buf);
		DeleteObject(bufbmp);
		DeleteObject(oldbuf);
		EndPaint(m_hWnd, &ps);
	}

	BOOL _RegisterClass()
	{
		WNDCLASSEX wcl;
		ZeroMemory(&wcl, sizeof(wcl));
		wcl.cbSize = sizeof(WNDCLASSEX);
		wcl.hInstance = m_hInstance;
		wcl.lpfnWndProc = (WNDPROC)WndProc;
		wcl.style = CS_HREDRAW | CS_VREDRAW;
		wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
		wcl.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
		wcl.lpszClassName = szClassName;
		if(!RegisterClassEx(&wcl))
			return FALSE;

		return TRUE;
	}

	BOOL _CreateWindow()
	{
		m_hWnd = CreateWindowEx(
			WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
			szClassName,
			szWindowName, 
			WS_POPUP | WS_VISIBLE, 
			25, 25, 25, 25, 
			NULL,
			NULL, 
			m_hInstance,
			this);

		if (m_hWnd) return TRUE;
		return FALSE;
	}

};

LangDis *LangDisPlugin;

extern "C" {
	DLL_EXPORT int beginPluginEx(HINSTANCE hInstance, HWND hSlit)
	{
		LangDisPlugin = new LangDis(hInstance, hSlit);
		return LangDisPlugin->Initialize();
	}

	DLL_EXPORT void endPlugin(HINSTANCE hInstance)
	{
		delete LangDisPlugin;
	}
	DLL_EXPORT const char* pluginInfo(int field)
	{
		return LangDisPlugin->PluginInfo(field);
	}

};
