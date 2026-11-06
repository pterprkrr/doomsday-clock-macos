#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shellapi.h>
#include <objidl.h>
#include <gdiplus.h>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <memory>
#include <string>
#include "Countdown.h"

using namespace Gdiplus;
namespace {
constexpr wchar_t appClass[] = L"DoomsdayClock.Controller.1";
constexpr wchar_t surfaceClass[] = L"DoomsdayClock.Surface.1";
constexpr wchar_t dateClass[] = L"DoomsdayClock.Date.1";
constexpr wchar_t settingsKey[] = L"Software\\DoomsdayClock";
constexpr UINT trayMessage = WM_APP+1, refreshMessage = WM_APP+2;
enum Commands { Desktop=101,Widget,Preview,ChangeDate,ResetDate,ResetPosition,Help,Quit };
enum SurfaceKind { DesktopSurface=1,WidgetSurface,PreviewSurface };
struct Settings {
    int64_t target = doom::defaultTarget();
    bool desktop = true, widget = false;
    int x = INT_MIN, y = INT_MIN;
} settings;
struct DesktopHost { HWND parent=nullptr, after=HWND_TOP; } desktopHost;
HINSTANCE instance;
HWND controller=nullptr, desktop=nullptr, widget=nullptr, preview=nullptr, dateDialog=nullptr;
HICON icon=nullptr;
UINT taskbarCreated=0;
bool desktopFailed=false, fullscreen=false;
WINDOWPLACEMENT previewPlacement{sizeof(WINDOWPLACEMENT)};
std::unique_ptr<FontFamily> clockFont;
std::wstring clockFontName;
float titleEm=100, numeralEm=70;

int64_t now() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}
std::wstring dateLabel() {
    auto d=doom::dateAt(settings.target);
    const wchar_t* months[]={L"Januari",L"Februari",L"Maret",L"April",L"Mei",L"Juni",L"Juli",L"Agustus",L"September",L"Oktober",L"November",L"Desember"};
    wchar_t text[128];
    swprintf(text,128,L"%d %ls %d · %02d.%02d WITA",d.day,months[d.month-1],d.year,d.hour,d.minute);
    return text;
}
void loadSettings() {
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,settingsKey,0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS) return;
    auto read=[&](const wchar_t* name,void* data,DWORD size,DWORD expected) {
        DWORD type=0, bytes=size;
        return RegQueryValueExW(key,name,nullptr,&type,static_cast<BYTE*>(data),&bytes)==ERROR_SUCCESS && type==expected && bytes==size;
    };
    int64_t target=0;
    if(read(L"TargetUTC",&target,sizeof(target),REG_QWORD) &&
       target>=doom::timestamp({1970,1,1}) && target<=doom::timestamp({9998,12,31,23,59,59})) settings.target=target;
    DWORD value;
    if(read(L"Desktop",&value,4,REG_DWORD)) settings.desktop=value!=0;
    if(read(L"Widget",&value,4,REG_DWORD)) settings.widget=value!=0;
    if(read(L"WidgetX",&value,4,REG_DWORD)) settings.x=static_cast<int>(value);
    if(read(L"WidgetY",&value,4,REG_DWORD)) settings.y=static_cast<int>(value);
    RegCloseKey(key);
}
void saveSettings() {
    HKEY key;
    if(RegCreateKeyExW(HKEY_CURRENT_USER,settingsKey,0,nullptr,0,KEY_SET_VALUE,nullptr,&key,nullptr)!=ERROR_SUCCESS) return;
    auto write=[&](const wchar_t* name,DWORD value) { RegSetValueExW(key,name,0,REG_DWORD,reinterpret_cast<BYTE*>(&value),4); };
    RegSetValueExW(key,L"TargetUTC",0,REG_QWORD,reinterpret_cast<const BYTE*>(&settings.target),sizeof(settings.target));
    write(L"Desktop",settings.desktop); write(L"Widget",settings.widget);
    write(L"WidgetX",static_cast<DWORD>(settings.x)); write(L"WidgetY",static_cast<DWORD>(settings.y));
    RegCloseKey(key);
}
MONITORINFO monitorInfo(HMONITOR monitor=nullptr) {
    MONITORINFO info{sizeof(info)};
    GetMonitorInfoW(monitor ? monitor : MonitorFromPoint({0,0},MONITOR_DEFAULTTOPRIMARY),&info);
    return info;
}
float dpiScale(HWND window) { return GetDpiForWindow(window)/96.0f; }
void addText(GraphicsPath& path,const wchar_t* text,const FontFamily* family,int style,float em) {
    path.AddString(text,-1,family,style,em,PointF(0,0),StringFormat::GenericTypographic());
}
void chooseFont() {
    // Avenir is used only when already installed by the user; no proprietary font is distributed.
    for (const wchar_t* name : {L"Avenir Next",L"Avenir Next LT Pro",L"Segoe UI",L"Arial"}) {
        auto family=std::make_unique<FontFamily>(name);
        if(family->GetLastStatus()==Ok && family->IsStyleAvailable(FontStyleBold)) {
            clockFontName=name; clockFont=std::move(family); break;
        }
    }
    if(!clockFont) { clockFont.reset(FontFamily::GenericSansSerif()->Clone()); clockFontName=L"Sans Serif"; }
    GraphicsPath title, digit;
    addText(title,L"DOOMSDAY IS COMING",clockFont.get(),FontStyleBold,100);
    addText(digit,L"0",clockFont.get(),FontStyleBold,100);
    RectF a,b; title.GetBounds(&a); digit.GetBounds(&b);
    titleEm=100*float(doom::titleHeight)/a.Height;
    numeralEm=100*float(doom::numeralHeight)/b.Height;
}
void drawPath(Graphics& g,GraphicsPath& path,float x,float y,float maxWidth=0) {
    RectF bounds; path.GetBounds(&bounds);
    float squeeze=maxWidth>0 && bounds.Width>maxWidth ? maxWidth/bounds.Width : 1;
    Matrix transform(squeeze,0,0,1,x-bounds.Width*squeeze/2-bounds.X*squeeze,y-bounds.Y);
    path.Transform(&transform);
    // Soft neutral shadow; the face of every glyph remains solid white.
    for (float width : {8.f,5.f,2.f}) {
        Pen shadow(Color(34,0,0,0),width); shadow.SetLineJoin(LineJoinRound); g.DrawPath(&shadow,&path);
    }
    SolidBrush white(Color(255,255,255,255)); g.FillPath(&white,&path);
}
void text(Graphics& g,const wchar_t* textValue,float x,float y,float em,float maxWidth=0) {
    GraphicsPath path; addText(path,textValue,clockFont.get(),FontStyleBold,em); drawPath(g,path,x,y,maxWidth);
}
void label(Graphics& g,const wchar_t* value,float x,float y) {
    FontFamily family(L"Segoe UI");
    GraphicsPath line;
    float advance=0;
    for(const wchar_t* p=value;*p;++p) {
        wchar_t letter[]={*p,0}; GraphicsPath glyph;
        addText(glyph,letter,&family,FontStyleRegular,19);
        RectF b; glyph.GetBounds(&b);
        Matrix move(1,0,0,1,advance-b.X,0); glyph.Transform(&move); line.AddPath(&glyph,FALSE);
        advance+=b.Width+3.2f;
    }
    drawPath(g,line,x,y);
}
void drawClock(Graphics& g,float scale,float x,float y,int64_t instant) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    auto state=g.Save();
    g.TranslateTransform(x,y); g.ScaleTransform(scale,scale);
    text(g,L"DOOMSDAY IS COMING",690,20,titleEm,1320);
    auto values=doom::parts(instant,settings.target);
    const wchar_t* labels[]={L"MONTHS",L"DAYS",L"HOURS",L"MINUTES",L"SECONDS"};
    for(int i=0;i<5;++i) {
        float center=310+190.f*i;
        wchar_t number[32]; swprintf(number,32,L"%02d",values[i]);
        text(g,number,center,144,numeralEm,160);
        label(g,labels[i],center,224);
        if(i<4) text(g,L":",center+95,157,numeralEm);
    }
    if(instant>=settings.target) label(g,L"DOOMSDAY IS HERE",690,315);
    g.Restore(state);
}
bool renderSelfTest() {
    // Runs on Windows CI without creating desktop windows or writing user settings.
    for(int width : {560,1380,2760}) {
        int height=int(std::ceil(width*350.0/1380));
        Bitmap image(width,height,PixelFormat32bppPARGB);
        if(image.GetLastStatus()!=Ok) return false;
        {
            Graphics g(&image);
            g.Clear(Color(0,0,0,0));
            drawClock(g,width/1380.f,0,0,doom::timestamp({2026,11,5}));
            g.Flush(FlushIntentionSync);
        }
        Color corner;
        image.GetPixel(0,0,&corner); if(corner.GetA()!=0) return false;
        image.GetPixel(width-1,height-1,&corner); if(corner.GetA()!=0) return false;
        unsigned whitePixels=0;
        for(int y=0;y<height;++y) for(int x=0;x<width;++x) {
            Color pixel; image.GetPixel(x,y,&pixel);
            if(pixel.GetA()==255 && pixel.GetR()==255 && pixel.GetG()==255 && pixel.GetB()==255) ++whitePixels;
        }
        if(whitePixels<500) return false;
        // Repaint from an empty alpha buffer: expired digits must not retain old glyphs.
        {
            Graphics g(&image); g.Clear(Color(0,0,0,0));
            drawClock(g,width/1380.f,0,0,doom::defaultTarget());
            g.Flush(FlushIntentionSync);
        }
        image.GetPixel(0,0,&corner); if(corner.GetA()!=0) return false;
    }
    GraphicsPath title,zero;
    addText(title,L"DOOMSDAY IS COMING",clockFont.get(),FontStyleBold,titleEm);
    addText(zero,L"0",clockFont.get(),FontStyleBold,numeralEm);
    RectF a,b; title.GetBounds(&a); zero.GetBounds(&b);
    return std::abs(b.Height/a.Height-0.7f)<0.001f;
}
bool renderLayer(HWND window) {
    if(!IsWindow(window) || !IsWindowVisible(window)) return true;
    RECT rect; GetClientRect(window,&rect);
    const int width=rect.right,height=rect.bottom;
    if(width<=0 || height<=0) return false;
    BITMAPINFO info{}; info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=width; info.bmiHeader.biHeight=-height; info.bmiHeader.biPlanes=1;
    info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    void* pixels=nullptr;
    HDC dc=CreateCompatibleDC(nullptr);
    HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&pixels,nullptr,0);
    if(!dc || !bitmap) { if(bitmap) DeleteObject(bitmap); if(dc) DeleteDC(dc); return false; }
    auto old=SelectObject(dc,bitmap);
    {
        Bitmap image(width,height,width*4,PixelFormat32bppPARGB,static_cast<BYTE*>(pixels));
        Graphics graphics(&image); graphics.Clear(Color(0,0,0,0));
        drawClock(graphics,width/1380.f,0,0,now());
        graphics.Flush(FlushIntentionSync);
    }
    POINT source{0,0}; SIZE size{width,height}; BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};
    bool ok=UpdateLayeredWindow(window,nullptr,nullptr,&size,dc,&source,0,&blend,ULW_ALPHA)!=0;
    SelectObject(dc,old); DeleteObject(bitmap); DeleteDC(dc);
    return ok;
}
BOOL CALLBACK findDesktop(HWND top,LPARAM data) {
    HWND icons=FindWindowExW(top,nullptr,L"SHELLDLL_DefView",nullptr);
    if(!icons) return TRUE;
    auto* result=reinterpret_cast<DesktopHost*>(data);
    if(FindWindowExW(top,nullptr,L"WorkerW",nullptr)) {
        // Windows 11 raised desktop: insert our alpha surface between icons and wallpaper.
        *result={top,icons}; return FALSE;
    }
    HWND worker=FindWindowExW(nullptr,top,L"WorkerW",nullptr);
    if(worker) { *result={worker,HWND_TOP}; return FALSE; }
    return TRUE;
}
DesktopHost findHost() {
    HWND progman=FindWindowW(L"Progman",nullptr);
    if(!progman) return {};
    DWORD_PTR result=0;
    SendMessageTimeoutW(progman,0x052C,0,0,SMTO_ABORTIFHUNG,500,&result);
    DesktopHost host;
    EnumWindows(findDesktop,reinterpret_cast<LPARAM>(&host));
    return host;
}
void hideDestroy(HWND& window) {
    HWND old=window; window=nullptr;
    if(IsWindow(old)) DestroyWindow(old);
}
void balloon(const wchar_t* message) {
    NOTIFYICONDATAW data{sizeof(data)}; data.hWnd=controller; data.uID=1; data.uFlags=NIF_INFO;
    wcscpy(data.szInfoTitle,L"Doomsday Clock"); wcsncpy(data.szInfo,message,255); data.dwInfoFlags=NIIF_INFO;
    Shell_NotifyIconW(NIM_MODIFY,&data);
}
void syncDesktop(bool rediscover=false) {
    if(!settings.desktop) { hideDestroy(desktop); return; }
    if(rediscover || !IsWindow(desktopHost.parent)) {
        hideDestroy(desktop); desktopHost=findHost();
    }
    if(!desktopHost.parent) { desktopFailed=true; return; }
    auto screen=monitorInfo();
    int sw=screen.rcMonitor.right-screen.rcMonitor.left,sh=screen.rcMonitor.bottom-screen.rcMonitor.top;
    float scale=float(doom::desktopScale(sw,sh));
    // Match the shell's DPI context for cross-process child windows, then restore ours.
    auto previous=SetThreadDpiAwarenessContext(GetWindowDpiAwarenessContext(desktopHost.parent));
    if(!IsWindow(desktop)) desktop=CreateWindowExW(WS_EX_LAYERED|WS_EX_TRANSPARENT|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW,
        surfaceClass,L"Doomsday Desktop",WS_CHILD,0,0,1,1,desktopHost.parent,nullptr,instance,reinterpret_cast<void*>(DesktopSurface));
    float dpi=desktop ? dpiScale(desktop) : 1;
    int clearance=std::max(int(150*dpi),int(screen.rcMonitor.bottom-screen.rcWork.bottom)+int(48*dpi));
    int width=int(std::ceil(1380*scale)),height=int(std::ceil(350*scale));
    POINT position{screen.rcMonitor.left+(sw-width)/2,screen.rcMonitor.bottom-clearance-int(296*scale)};
    position.y=std::max(position.y,screen.rcWork.top);
    ScreenToClient(desktopHost.parent,&position);
    if(desktop) SetWindowPos(desktop,desktopHost.after,position.x,position.y,width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW);
    desktopFailed=!desktop || !renderLayer(desktop);
    SetThreadDpiAwarenessContext(previous);
}
void syncWidget(bool reposition=false) {
    if(!settings.widget) { hideDestroy(widget); return; }
    bool created=!IsWindow(widget);
    if(created) widget=CreateWindowExW(WS_EX_LAYERED|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,
        surfaceClass,L"Doomsday Widget",WS_POPUP,0,0,560,142,controller,nullptr,instance,reinterpret_cast<void*>(WidgetSurface));
    if(!widget) return;
    if(created || reposition) {
        POINT point{settings.x==INT_MIN?80:settings.x,settings.y==INT_MIN?100:settings.y};
        auto screen=monitorInfo(MonitorFromPoint(point,MONITOR_DEFAULTTONEAREST));
        SetWindowPos(widget,nullptr,point.x,point.y,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
        float scale=dpiScale(widget);
        int width=std::min(int(560*scale),int(screen.rcWork.right-screen.rcWork.left));
        int height=int(std::ceil(width*350.0/1380));
        point.x=std::clamp(int(point.x),int(screen.rcWork.left),int(screen.rcWork.right)-width);
        point.y=std::clamp(int(point.y),int(screen.rcWork.top),std::max(int(screen.rcWork.top),int(screen.rcWork.bottom)-height));
        SetWindowPos(widget,HWND_TOPMOST,point.x,point.y,width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW);
    } else ShowWindow(widget,SW_SHOWNOACTIVATE);
    renderLayer(widget);
}
void updateTray(bool add=false) {
    NOTIFYICONDATAW data{sizeof(data)}; data.hWnd=controller; data.uID=1;
    data.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP; data.uCallbackMessage=trayMessage; data.hIcon=icon;
    std::wstring tip=L"DOOM · "+dateLabel(); wcsncpy(data.szTip,tip.c_str(),127);
    Shell_NotifyIconW(add?NIM_ADD:NIM_MODIFY,&data);
}
void refresh() {
    renderLayer(desktop); renderLayer(widget);
    if(IsWindow(preview)) InvalidateRect(preview,nullptr,FALSE);
    updateTray(); saveSettings();
}
void showMenu() {
    HMENU menu=CreatePopupMenu();
    AppendMenuW(menu,MF_STRING|MF_GRAYED,0,L"DOOMSDAY CLOCK");
    AppendMenuW(menu,MF_STRING|MF_GRAYED,0,dateLabel().c_str());
    AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
    AppendMenuW(menu,MF_STRING|(settings.desktop?MF_CHECKED:0),Desktop,L"Countdown di desktop");
    AppendMenuW(menu,MF_STRING|(settings.widget?MF_CHECKED:0),Widget,L"Widget mengambang");
    AppendMenuW(menu,MF_STRING,Preview,L"Pratinjau besar…");
    AppendMenuW(menu,MF_STRING,ResetPosition,L"Kembalikan posisi widget");
    AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
    AppendMenuW(menu,MF_STRING,ChangeDate,L"Ubah tanggal countdown…");
    AppendMenuW(menu,MF_STRING,ResetDate,L"Reset ke 16 Desember 2026");
    AppendMenuW(menu,MF_SEPARATOR,0,nullptr);
    AppendMenuW(menu,MF_STRING,Help,L"Petunjuk penggunaan…");
    AppendMenuW(menu,MF_STRING,Quit,L"Keluar");
    POINT point; GetCursorPos(&point); SetForegroundWindow(controller);
    int command=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,0,controller,nullptr);
    DestroyMenu(menu); PostMessageW(controller,WM_NULL,0,0);
    if(command) PostMessageW(controller,WM_COMMAND,command,0);
}
void showPreview() {
    if(!IsWindow(preview)) preview=CreateWindowExW(0,surfaceClass,L"Doomsday Clock — F11 untuk layar penuh",WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,1120,680,nullptr,nullptr,instance,reinterpret_cast<void*>(PreviewSurface));
    ShowWindow(preview,SW_SHOW); SetForegroundWindow(preview);
}
void toggleFullscreen(HWND window) {
    if(!fullscreen) {
        GetWindowPlacement(window,&previewPlacement);
        auto screen=monitorInfo(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST));
        SetWindowLongPtrW(window,GWL_STYLE,WS_POPUP|WS_VISIBLE);
        SetWindowPos(window,HWND_TOP,screen.rcMonitor.left,screen.rcMonitor.top,
            screen.rcMonitor.right-screen.rcMonitor.left,screen.rcMonitor.bottom-screen.rcMonitor.top,SWP_FRAMECHANGED);
    } else {
        SetWindowLongPtrW(window,GWL_STYLE,WS_OVERLAPPEDWINDOW|WS_VISIBLE);
        SetWindowPlacement(window,&previewPlacement);
        SetWindowPos(window,nullptr,0,0,0,0,SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
    }
    fullscreen=!fullscreen;
}
LRESULT CALLBACK surfaceProc(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    int kind=int(GetWindowLongPtrW(window,GWLP_USERDATA));
    if(message==WM_NCCREATE) {
        kind=int(reinterpret_cast<INT_PTR>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams));
        SetWindowLongPtrW(window,GWLP_USERDATA,kind);
    }
    switch(message) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint; HDC dc=BeginPaint(window,&paint);
        if(kind==PreviewSurface) {
            RECT r; GetClientRect(window,&r);
            Graphics g(dc); g.Clear(Color(255,9,9,9));
            float scale=std::min(r.right/1500.f,r.bottom/440.f);
            drawClock(g,scale,(r.right-1380*scale)/2,(r.bottom-350*scale)/2,now());
        }
        EndPaint(window,&paint); return 0;
    }
    case WM_NCHITTEST:
        if(kind==DesktopSurface) return HTTRANSPARENT;
        if(kind==WidgetSurface) return HTCAPTION;
        break;
    case WM_MOUSEACTIVATE: if(kind!=PreviewSurface) return MA_NOACTIVATE; break;
    case WM_NCRBUTTONUP: if(kind==WidgetSurface) { showMenu(); return 0; } break;
    case WM_EXITSIZEMOVE:
        if(kind==WidgetSurface) { RECT r; GetWindowRect(window,&r); settings.x=r.left; settings.y=r.top; saveSettings(); }
        break;
    case WM_DPICHANGED:
        if(kind==WidgetSurface) {
            auto* r=reinterpret_cast<RECT*>(lp);
            settings.x=r->left; settings.y=r->top;
            PostMessageW(controller,refreshMessage,0,0);
        } else if(kind==PreviewSurface) {
            auto* r=reinterpret_cast<RECT*>(lp);
            SetWindowPos(window,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER|SWP_NOACTIVATE);
        }
        return 0;
    case WM_SIZE: if(kind==PreviewSurface) InvalidateRect(window,nullptr,FALSE); return 0;
    case WM_GETMINMAXINFO:
        if(kind==PreviewSurface) { auto* p=reinterpret_cast<MINMAXINFO*>(lp); p->ptMinTrackSize={480,300}; return 0; }
        break;
    case WM_KEYDOWN:
        if(kind==PreviewSurface && (wp==VK_F11 || (wp==VK_ESCAPE && fullscreen))) { toggleFullscreen(window); return 0; }
        break;
    case WM_CLOSE:
        if(kind==WidgetSurface) { settings.widget=false; syncWidget(); saveSettings(); return 0; }
        if(kind==DesktopSurface) return 0;
        break;
    case WM_DESTROY:
        if(window==desktop) desktop=nullptr;
        if(window==widget) widget=nullptr;
        if(window==preview) { preview=nullptr; fullscreen=false; }
        return 0;
    }
    return DefWindowProcW(window,message,wp,lp);
}
LRESULT CALLBACK dateProc(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    switch(message) {
    case WM_CREATE: {
        float dpi=dpiScale(window);
        auto control=[&](const wchar_t* cls,const wchar_t* value,DWORD style,int id,int x,int y,int w,int h) {
            HWND child=CreateWindowExW(0,cls,value,WS_CHILD|WS_VISIBLE|style,int(x*dpi),int(y*dpi),int(w*dpi),int(h*dpi),
                window,reinterpret_cast<HMENU>(INT_PTR(id)),instance,nullptr);
            SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
            return child;
        };
        control(L"STATIC",L"Pilih tanggal dan jam dalam WITA (UTC+8).",0,0,20,18,380,24);
        HWND date=control(DATETIMEPICK_CLASSW,L"",DTS_SHORTDATEFORMAT|WS_TABSTOP,201,20,55,240,30);
        HWND time=control(DATETIMEPICK_CLASSW,L"",DTS_TIMEFORMAT|WS_TABSTOP,202,275,55,125,30);
        SendMessageW(date,DTM_SETFORMATW,0,reinterpret_cast<LPARAM>(L"dd MMMM yyyy"));
        SendMessageW(time,DTM_SETFORMATW,0,reinterpret_cast<LPARAM>(L"HH:mm"));
        SYSTEMTIME limits[2]{};
        limits[0].wYear=1970; limits[0].wMonth=1; limits[0].wDay=1;
        limits[1].wYear=9998; limits[1].wMonth=12; limits[1].wDay=31;
        SendMessageW(date,DTM_SETRANGE,GDTR_MIN|GDTR_MAX,reinterpret_cast<LPARAM>(limits));
        auto d=doom::dateAt(settings.target);
        SYSTEMTIME value{}; value.wYear=WORD(d.year); value.wMonth=WORD(d.month); value.wDay=WORD(d.day);
        value.wHour=WORD(d.hour); value.wMinute=WORD(d.minute);
        SendMessageW(date,DTM_SETSYSTEMTIME,GDT_VALID,reinterpret_cast<LPARAM>(&value));
        SendMessageW(time,DTM_SETSYSTEMTIME,GDT_VALID,reinterpret_cast<LPARAM>(&value));
        control(L"BUTTON",L"Simpan",BS_DEFPUSHBUTTON|WS_TABSTOP,IDOK,200,110,95,30);
        control(L"BUTTON",L"Batal",BS_PUSHBUTTON|WS_TABSTOP,IDCANCEL,305,110,95,30);
        SetFocus(date); return 0;
    }
    case WM_COMMAND:
        if(LOWORD(wp)==IDOK) {
            SYSTEMTIME date{},time{};
            if(SendDlgItemMessageW(window,201,DTM_GETSYSTEMTIME,0,reinterpret_cast<LPARAM>(&date))!=GDT_VALID ||
               SendDlgItemMessageW(window,202,DTM_GETSYSTEMTIME,0,reinterpret_cast<LPARAM>(&time))!=GDT_VALID) return 0;
            doom::Date d{date.wYear,date.wMonth,date.wDay,time.wHour,time.wMinute,0};
            if(!doom::valid(d)) return 0;
            settings.target=doom::timestamp(d); refresh(); DestroyWindow(window); return 0;
        }
        if(LOWORD(wp)==IDCANCEL) { DestroyWindow(window); return 0; }
        break;
    case WM_DESTROY: dateDialog=nullptr; return 0;
    }
    return DefWindowProcW(window,message,wp,lp);
}
void showDate() {
    if(IsWindow(dateDialog)) { SetForegroundWindow(dateDialog); return; }
    float dpi=dpiScale(controller);
    RECT r{0,0,int(420*dpi),int(165*dpi)};
    AdjustWindowRectExForDpi(&r,WS_CAPTION|WS_SYSMENU,FALSE,WS_EX_DLGMODALFRAME,GetDpiForWindow(controller));
    auto screen=monitorInfo();
    dateDialog=CreateWindowExW(WS_EX_DLGMODALFRAME,dateClass,L"Tanggal countdown — WITA",WS_CAPTION|WS_SYSMENU,
        (screen.rcWork.left+screen.rcWork.right-r.right+r.left)/2,(screen.rcWork.top+screen.rcWork.bottom-r.bottom+r.top)/2,
        r.right-r.left,r.bottom-r.top,controller,nullptr,instance,nullptr);
    ShowWindow(dateDialog,SW_SHOW); SetForegroundWindow(dateDialog);
}
LRESULT CALLBACK controllerProc(HWND window,UINT message,WPARAM wp,LPARAM lp) {
    if(taskbarCreated && message==taskbarCreated) {
        updateTray(true); PostMessageW(window,refreshMessage,1,0); return 0;
    }
    switch(message) {
    case trayMessage:
        if(lp==WM_RBUTTONUP || lp==WM_LBUTTONUP) showMenu();
        return 0;
    case WM_TIMER:
        if(wp==1) { renderLayer(desktop); renderLayer(widget); if(IsWindow(preview)) InvalidateRect(preview,nullptr,FALSE); }
        else if(wp==2 && settings.desktop) syncDesktop(!IsWindow(desktopHost.parent) || !IsWindow(desktop));
        return 0;
    case WM_DISPLAYCHANGE: case WM_SETTINGCHANGE:
        PostMessageW(window,refreshMessage,1,0); return 0;
    case WM_POWERBROADCAST:
        if(wp==PBT_APMRESUMEAUTOMATIC) PostMessageW(window,refreshMessage,1,0);
        return TRUE;
    case refreshMessage: syncDesktop(wp!=0); syncWidget(true); refresh(); return 0;
    case WM_COMMAND:
        switch(LOWORD(wp)) {
        case Desktop:
            settings.desktop=!settings.desktop; syncDesktop(true);
            if(settings.desktop && desktopFailed) balloon(L"Countdown desktop belum bisa ditampilkan. Coba Widget mengambang dari menu DOOM.");
            break;
        case Widget: settings.widget=!settings.widget; syncWidget(); break;
        case Preview: showPreview(); break;
        case ChangeDate: showDate(); break;
        case ResetDate: settings.target=doom::defaultTarget(); break;
        case ResetPosition: settings.x=INT_MIN; settings.y=INT_MIN; settings.widget=true; syncWidget(true); break;
        case Help: {
            std::wstring help=L"Countdown tampil transparan di atas wallpaper desktop utama.\n\nKlik ikon DOOM di dekat jam taskbar (atau di menu ^) untuk mengubah tanggal dan menyalakan widget. Seret tulisan widget untuk memindahkannya.\n\nTanggal menggunakan WITA (UTC+8), apa pun zona waktu komputer. F11 membuka layar penuh di Pratinjau besar.\n\nFont aktif: "+clockFontName+L" Bold.\n\nDoomsday Clock adalah proyek penggemar.";
            MessageBoxW(window,help.c_str(),L"Doomsday Clock",MB_OK|MB_ICONINFORMATION); break;
        }
        case Quit: DestroyWindow(window); return 0;
        }
        refresh(); return 0;
    case WM_ENDSESSION: if(wp) saveSettings(); return 0;
    case WM_DESTROY: {
        KillTimer(window,1); KillTimer(window,2); saveSettings();
        hideDestroy(desktop); hideDestroy(widget); hideDestroy(preview); hideDestroy(dateDialog);
        NOTIFYICONDATAW data{sizeof(data)}; data.hWnd=window; data.uID=1; Shell_NotifyIconW(NIM_DELETE,&data);
        PostQuitMessage(0); return 0;
    }
    }
    return DefWindowProcW(window,message,wp,lp);
}
} // namespace

int WINAPI wWinMain(HINSTANCE app,HINSTANCE,PWSTR arguments,int) {
    instance=app;
    if(wcscmp(arguments,L"--self-test")==0) {
        GdiplusStartupInput input; ULONG_PTR token=0;
        if(GdiplusStartup(&token,&input,nullptr)!=Ok) return 1;
        chooseFont(); bool ok=renderSelfTest(); clockFont.reset(); GdiplusShutdown(token);
        return ok?0:1;
    }
    HANDLE mutex=CreateMutexW(nullptr,FALSE,L"Local\\DoomsdayClock.Windows.1");
    if(!mutex) return 1;
    if(GetLastError()==ERROR_ALREADY_EXISTS) {
        HWND existing=FindWindowW(appClass,L"Doomsday Clock");
        if(existing) PostMessageW(existing,WM_COMMAND,Preview,0);
        CloseHandle(mutex); return 0;
    }
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    GdiplusStartupInput input; ULONG_PTR token=0;
    if(GdiplusStartup(&token,&input,nullptr)!=Ok) { CloseHandle(mutex); return 1; }
    INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_DATE_CLASSES}; InitCommonControlsEx(&controls);
    icon=LoadIconW(instance,MAKEINTRESOURCEW(101));
    if(!icon) icon=LoadIconW(nullptr,IDI_APPLICATION);
    WNDCLASSEXW cls{sizeof(cls)}; cls.hInstance=instance; cls.hCursor=LoadCursorW(nullptr,IDC_ARROW); cls.hIcon=icon; cls.hIconSm=icon;
    cls.lpszClassName=appClass; cls.lpfnWndProc=controllerProc; RegisterClassExW(&cls);
    cls.lpszClassName=surfaceClass; cls.lpfnWndProc=surfaceProc; RegisterClassExW(&cls);
    cls.lpszClassName=dateClass; cls.lpfnWndProc=dateProc; cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1); RegisterClassExW(&cls);
    loadSettings(); chooseFont();
    controller=CreateWindowExW(WS_EX_TOOLWINDOW,appClass,L"Doomsday Clock",WS_OVERLAPPED,0,0,0,0,nullptr,nullptr,instance,nullptr);
    if(controller) {
        taskbarCreated=RegisterWindowMessageW(L"TaskbarCreated"); updateTray(true);
        syncDesktop(true); syncWidget();
        if(desktopFailed && settings.desktop) {
            settings.widget=true; syncWidget();
            balloon(L"Desktop belum bisa menampilkan countdown. Widget mengambang diaktifkan; buka menu DOOM untuk pengaturan.");
        }
        SetTimer(controller,1,1000,nullptr); SetTimer(controller,2,5000,nullptr);
        MSG msg;
        while(GetMessageW(&msg,nullptr,0,0)>0) {
            if(!dateDialog || !IsDialogMessageW(dateDialog,&msg)) { TranslateMessage(&msg); DispatchMessageW(&msg); }
        }
    }
    clockFont.reset(); GdiplusShutdown(token); CloseHandle(mutex);
    return controller?0:1;
}
