#include "App.h"
#include <sstream>

int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow) {

    //SetConsoleOutputCP(CP_UTF8);

    try {
        return App{}.Run();

    } catch (const PracticeException& e) {
        MessageBox(nullptr, std::wstring(e.what(), e.what()
            + strlen(e.what())).c_str(), std::wstring(e.GetType()
            , e.GetType() + strlen(e.GetType())).c_str(), MB_OK | MB_ICONEXCLAMATION);
    } catch (const std::exception& e) {
        MessageBox(nullptr, std::wstring(e.what(), e.what()
            + strlen(e.what())).c_str(), L"Standard Exception", MB_OK | MB_ICONEXCLAMATION);
    } catch (...) {
        MessageBox(nullptr, L"No details available", L"Unknown Exception", MB_OK | MB_ICONEXCLAMATION);
    }

    return -1;
}