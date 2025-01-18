#include "wbk.h"
#include <windows.h>
#include <uxtheme.h>
#include <commctrl.h>
#include <shellapi.h>
#include <string>

namespace fs = std::filesystem;

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "uxtheme.lib")

/*
void test_run()
{
#if _DEBUG
    WBK wbk2;
    wbk2.read(R"(TREYARCH_LOGO_EN.WBK)");
    WAV::writeWAV(R"(TREYARCH_LOGO_EN.WBK.wav)", wbk2.tracks[0], wbk2.entries[0].samples_per_second / WBK::GetNumChannels(wbk2.entries[0]), WBK::GetNumChannels(wbk2.entries[0]));

    WAV replacement_wav;
    replacement_wav.readWAV(R"(TREYARCH_LOGO_EN.WBK.wav)");
    wbk2.replace(0, replacement_wav);
    wbk2.write(R"(TREYARCH_LOGO_EN.WBK_test.WBK)");

    WBK wbk3;
    wbk3.read(R"(TREYARCH_LOGO_EN.WBK_test.WBK)");
    WAV::writeWAV(R"(TREYARCH_LOGO_EN.WBK_test.wav)", wbk3.tracks[0], wbk3.entries[0].samples_per_second, WBK::GetNumChannels(wbk3.entries[0]));
#endif
}

int main(int argc, char** argv)
{
    test_run();

    if (argc < 4 || argc > 6) {
        printf("Usage: %s -e|-r <.wbk> <.wav>\n", argv[0]);
        return -1;
    }
    
    bool extract = false;
    int replace_idx = -1;
    if (strstr(argv[1], "-e")) {
        extract = true;
    } else if (strstr(argv[1], "-r")) {
        auto idx = atoi(argv[3]);
        if (idx > INT_MIN && idx < INT_MAX)
            replace_idx = idx;
        else 
            printf("Invalid replacement index specified!\n");
    } 
    else
        return -1;

    WBK wbk;
    wbk.read(argv[2]);
    if (extract)
    {
        size_t index = 0;
        for (auto& track : wbk.tracks) {
            WBK::nslWave& entry = wbk.entries[index];
            fs::path output_path = std::string(argv[3]).append(std::to_string(index)).append(".wav");
            WAV::writeWAV(output_path.string(), track, entry.samples_per_second / WBK::GetNumChannels(entry), WBK::GetNumChannels(entry));
            ++index;
        }
        return 1;
    }
    else {
        if (replace_idx > wbk.header.num_entries)
            printf("Invalid replacement index specified!\n");
        else
        {
            WAV replacement_wav;
            if (replacement_wav.readWAV(argv[4])) {
                if (wbk.replace(replace_idx, replacement_wav))
                {
                    fs::path path = fs::path(std::string(argv[2])).replace_extension(".new.wbk").string();
                    wbk.write(path);
                    printf("Replaced index %d and written to %s\n", replace_idx, path.string().c_str());
                }
            }
        }
    }
    return 1;
}//*/

HWND hwndDragDrop;
HWND hwndRadioExtract, hwndRadioReplace;
HWND hwndIndexInput, hwndRWAVPathInput, hwndClear, hwndConvert;

std::string droppedFilePath;
bool isReplaceMode = false;

void ResetInputs() {
    droppedFilePath.clear();
    SetWindowText(hwndDragDrop, "Drop a file here");
    SetWindowText(hwndIndexInput, "-1");
    SetWindowText(hwndRWAVPathInput, "");
}

void ProcessFile() {
    if (droppedFilePath.empty()) {
        MessageBox(NULL, "Please drop a file before converting.", "Error", MB_ICONERROR);
        return;
    }

    char _file[256];
    GetWindowText(hwndDragDrop, _file, sizeof(_file));
    WBK wbk;
    wbk.read(_file);

    if (isReplaceMode) {
        int replace_idx = -1;

        char buffer[16];
        GetWindowText(hwndIndexInput, buffer, sizeof(buffer));
        int idx = atoi(buffer);

        if (idx > INT_MIN && idx < INT_MAX) {
            replace_idx = idx;
            //MessageBox(NULL, "In range", "Info", MB_ICONINFORMATION);
        }
        else
            MessageBox(NULL, "Invalid replacement index specified!", "Error", MB_ICONERROR);
        
        if (replace_idx > wbk.header.num_entries)
            MessageBox(NULL, "Invalid replacement index specified!", "Error", MB_ICONERROR);
        else
        {
            char replacementWAVPath[256];
            GetWindowText(hwndRWAVPathInput, replacementWAVPath, sizeof(replacementWAVPath));
            if(strlen(replacementWAVPath) == 0) {
                MessageBox(NULL, "Replacement WAV file path not specified!", "Error", MB_ICONERROR);
                return;
            }
            WAV replacement_wav;
            if (replacement_wav.readWAV(replacementWAVPath)) {
                if (wbk.replace(replace_idx, replacement_wav))
                {
                    fs::path _path = fs::path(std::string(_file)).replace_extension(".new.wbk").string();
                    wbk.write(_path);
                    char infotext[256];
                    snprintf(infotext, sizeof(infotext), "Replaced index %d and written to %s\n", replace_idx, _path.string().c_str());
                    MessageBox(NULL, infotext, "Info", MB_ICONINFORMATION);
                }
            }
        }
    }
    else {
        size_t index = 0;

        for (auto& track : wbk.tracks) {
            WBK::nslWave& entry = wbk.entries[index];
            fs::path output_path = std::string(_file).append(std::to_string(index)).append(".wav");
            WAV::writeWAV(output_path.string(), track, entry.samples_per_second / WBK::GetNumChannels(entry), WBK::GetNumChannels(entry));
            ++index;
        }
    }
}//*/

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE: {
        hwndDragDrop = CreateWindow("STATIC", "Drop a file here",
            WS_CHILD | WS_VISIBLE | SS_CENTER | WS_BORDER,
            20, 20, 400, 40,
            hwnd, (HMENU)101, NULL, NULL);
        
        hwndRadioExtract = CreateWindow("BUTTON", "Extract (-e)",
            WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
            20, 80, 150, 20,
            hwnd, (HMENU)102, NULL, NULL);
        
        hwndRadioReplace = CreateWindow("BUTTON", "Replace (-r)",
            WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON,
            200, 80, 150, 20,
            hwnd, (HMENU)103, NULL, NULL);
        
        SendMessage(hwndRadioExtract, BM_SETCHECK, BST_CHECKED, 0);

        hwndIndexInput = CreateWindow("EDIT", "-1",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER,
            20, 110, 150, 20,
            hwnd, (HMENU)104, NULL, NULL);
        
        hwndRWAVPathInput = CreateWindow("EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            20, 140, 150, 20,
            hwnd, (HMENU)104, NULL, NULL);
        
        EnableWindow(hwndIndexInput, FALSE);
        EnableWindow(hwndRWAVPathInput, FALSE);

        hwndClear = CreateWindow("BUTTON", "Clear",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            200, 110, 100, 20,
            hwnd, (HMENU)105, NULL, NULL);
        
        hwndConvert = CreateWindow("BUTTON", "Convert",
            WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            20, 180, 400, 30,
            hwnd, (HMENU)106, NULL, NULL);

        DragAcceptFiles(hwnd, TRUE);
        break;
    }
    case WM_COMMAND: {
        switch (LOWORD(wParam)) {
        case 102: // Extract radio button
            isReplaceMode = false;
            EnableWindow(hwndIndexInput, FALSE);
            EnableWindow(hwndRWAVPathInput, FALSE);
            SendMessage(hwndRadioExtract, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(hwndRadioReplace, BM_SETCHECK, BST_UNCHECKED, 0);
            break;
        case 103: // Replace radio button
            isReplaceMode = true;
            EnableWindow(hwndIndexInput, TRUE);
            EnableWindow(hwndRWAVPathInput, TRUE);
            SendMessage(hwndRadioReplace, BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(hwndRadioExtract, BM_SETCHECK, BST_UNCHECKED, 0);
            break;
        case 105: // Clear button
            ResetInputs();
            break;
        case 106: // Convert button
            ProcessFile();
            break;
        }
        break;
    }
    case WM_DROPFILES: {
        HDROP hDrop = (HDROP)wParam;
        char filePath[MAX_PATH];
        DragQueryFile(hDrop, 0, filePath, MAX_PATH);
        DragFinish(hDrop);

        droppedFilePath = filePath;
        SetWindowText(hwndDragDrop, filePath);
        break;
    }
    case WM_ERASEBKGND: {
        HDC hdc = (HDC)wParam;
        RECT rect;
        GetClientRect(hwnd, &rect);
        HBRUSH hBrush = GetSysColorBrush(COLOR_3DFACE);
        FillRect(hdc, &rect, hBrush);
        return 1;
    }
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC: {
        HDC hdcStatic = (HDC)wParam;
        SetBkMode(hdcStatic, TRANSPARENT);
        return (LRESULT)GetSysColorBrush(COLOR_3DFACE);
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    SetThemeAppProperties(STAP_ALLOW_NONCLIENT | STAP_ALLOW_CONTROLS | STAP_ALLOW_WEBCONTENT);
    const char CLASS_NAME[] = "WBKTool";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "WBK Tool",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 450, 250,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}//*/
