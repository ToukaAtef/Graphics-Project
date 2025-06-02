#include <windows.h>
#include <vector>
#include <fstream>
#include <string>
#include <iostream>
#include <cmath>
#include <algorithm>

#define ID_FILE_EXIT 1
#define ID_COLOR_RED 2
#define ID_COLOR_GREEN 3
#define ID_COLOR_BLUE 4
#define ID_SCREEN_CLEAR 5
#define ID_BACKGROUND_WHITE 6
#define ID_SAVE 7
#define ID_LOAD 8

#define ID_LINE_BRESENHAM 100
#define ID_CIRCLE_DIRECT 101
#define ID_CIRCLE_POLAR 102
#define ID_CIRCLE_ITERATIVE_POLAR 103
#define ID_CIRCLE_MIDPOINT 104
#define ID_CIRCLE_MODIFIED_MIDPOINT 105
#define ID_CIRCLE_FILL_LINES 106
#define ID_CIRCLE_FILL_CIRCLES 107
#define ID_QUARTER_1 201
#define ID_QUARTER_2 202
#define ID_QUARTER_3 203
#define ID_QUARTER_4 204

// Structures for shapes
struct Line {
    POINT start, end;
    COLORREF color;
};

struct Circle {
    int xc, yc, R;
    COLORREF color;
    int quarter; // 1 to 4 for quarter-based drawing
    int algorithm; // 0: Direct, 1: Polar, 2: Iterative Polar, 3: Midpoint, 4: Modified Midpoint, 5: Fill Lines, 6: Fill Circles
};

std::vector<Line> lines;
std::vector<Circle> circles;
COLORREF currentColor = RGB(0, 0, 0); // Default: black
HBRUSH bgBrush = CreateSolidBrush(RGB(255, 255, 255)); // White
POINT tempPoint;
bool firstClick = true;

enum ShapeType { NONE, LINE, CIRCLE };
ShapeType currentShapeType = LINE;
enum LineAlgorithm { BRESENHAM };
enum CircleAlgorithm { DIRECT, POLAR, ITERATIVE_POLAR, MIDPOINT, MODIFIED_MIDPOINT, FILL_LINES, FILL_CIRCLES };
LineAlgorithm currentLineAlgorithm = BRESENHAM;
CircleAlgorithm currentCircleAlgorithm = DIRECT;
int currentQuarter = 1; // Default quarter

// Circle functions
void Draw8Points(HDC hdc, int xc, int yc, int x, int y, COLORREF c) {
    SetPixel(hdc, xc + x, yc + y, c);
    SetPixel(hdc, xc - x, yc + y, c);
    SetPixel(hdc, xc + x, yc - y, c);
    SetPixel(hdc, xc - x, yc - y, c);
    SetPixel(hdc, xc + y, yc + x, c);
    SetPixel(hdc, xc - y, yc + x, c);
    SetPixel(hdc, xc + y, yc - x, c);
    SetPixel(hdc, xc - y, yc - x, c);
}

void DrawPointsQuarter(HDC hdc, int xc, int yc, int x, int y, COLORREF c, int quarter) {
    switch (quarter) {
    case 1: // Top-right
        SetPixel(hdc, xc + x, yc - y, c);
        SetPixel(hdc, xc + y, yc - x, c);
        break;
    case 2: // Top-left
        SetPixel(hdc, xc - x, yc - y, c);
        SetPixel(hdc, xc - y, yc - x, c);
        break;
    case 3: // Bottom-left
        SetPixel(hdc, xc - x, yc + y, c);
        SetPixel(hdc, xc - y, yc + x, c);
        break;
    case 4: // Bottom-right
        SetPixel(hdc, xc + x, yc + y, c);
        SetPixel(hdc, xc + y, yc + x, c);
        break;
    }
}

void CircleDirectQuarter(HDC hdc, int xc, int yc, int R, COLORREF c, int quarter) {
    int x = 0;
    int y = R;
    while (x <= y) {
        y = (int)round(sqrt(R * R - x * x));
        DrawPointsQuarter(hdc, xc, yc, x, y, c, quarter);
        x++;
    }
}

void DDAHorizontalLine(HDC hdc, int x1, int x2, int y, COLORREF c) {
    if (x1 > x2) std::swap(x1, x2);
    for (int x = x1; x <= x2; x++)
        SetPixel(hdc, x, y, c);
}

void FillCircleWithLines(HDC hdc, int xc, int yc, int R, int quarter, COLORREF c) {
    for (int y = 0; y <= R; y++) {
        int x = (int)round(sqrt(R * R - y * y));
        switch (quarter) {
        case 1: // Top-right
            DDAHorizontalLine(hdc, xc, xc + x, yc - y, c);
            break;
        case 2: // Top-left
            DDAHorizontalLine(hdc, xc - x, xc, yc - y, c);
            break;
        case 3: // Bottom-left
            DDAHorizontalLine(hdc, xc - x, xc, yc + y, c);
            break;
        case 4: // Bottom-right
            DDAHorizontalLine(hdc, xc, xc + x, yc + y, c);
            break;
        }
    }
}

void FillCircleWithCircles(HDC hdc, int xc, int yc, int R, int quarter, COLORREF c) {
    for (int r = 0; r <= R; r++) {
        CircleDirectQuarter(hdc, xc, yc, r, c, quarter);
    }
}

void CircleDirect(HDC hdc, int xc, int yc, int R, COLORREF c) {
    int x = 0;
    int y = R;
    while (x <= y) {
        y = (int)round(sqrt(R * R - x * x));
        Draw8Points(hdc, xc, yc, x, y, c);
        x++;
    }
}

void CirclePolar(HDC hdc, int xc, int yc, int R, COLORREF c) {
    int x, y;
    double theta = 0, dtheta = 1.0 / R;
    while (theta <= 3.14159 / 4) {
        x = (int)round(R * cos(theta));
        y = (int)round(R * sin(theta));
        Draw8Points(hdc, xc, yc, x, y, c);
        theta += dtheta;
    }
}

void CircleIterativePolar(HDC hdc, int xc, int yc, int R, COLORREF c) {
    double x = R, y = 0;
    double dtheta = 1.0 / R;
    double cos_d = cos(dtheta), sin_d = sin(dtheta);
    while (x > y) {
        Draw8Points(hdc, xc, yc, (int)round(x), (int)round(y), c);
        double x1 = x * cos_d - y * sin_d;
        y = x * sin_d + y * cos_d;
        x = x1;
    }
}

void CircleMidpoint(HDC hdc, int xc, int yc, int R, COLORREF c) {
    int x = 0, y = R;
    int d = 1 - R;
    Draw8Points(hdc, xc, yc, x, y, c);
    while (x < y) {
        if (d < 0)
            d += 2 * x + 3;
        else {
            d += 2 * (x - y) + 5;
            y--;
        }
        x++;
        Draw8Points(hdc, xc, yc, x, y, c);
    }
}

void CircleModifiedMidpoint(HDC hdc, int xc, int yc, int R, COLORREF c) {
    int x = 0, y = R;
    int d = 1 - R;
    int d1 = 3, d2 = 5 - 2 * R;
    Draw8Points(hdc, xc, yc, x, y, c);
    while (x < y) {
        x++;
        if (d < 0) {
            d += d1;
            d1 += 2;
            d2 += 2;
        }
        else {
            y--;
            d += d2;
            d1 += 2;
            d2 += 4;
        }
        Draw8Points(hdc, xc, yc, x, y, c);
    }
}

void DrawBresenhamLine(HDC hdc, int x1, int y1, int x2, int y2, COLORREF color) {
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    while (true) {
        SetPixel(hdc, x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void DrawAllShapes(HDC hdc) {
    // Draw lines
    for (auto& line : lines) {
        DrawBresenhamLine(hdc, line.start.x, line.start.y, line.end.x, line.end.y, line.color);
    }
    // Draw circles
    for (auto& circle : circles) {
        switch (circle.algorithm) {
        case DIRECT:
            CircleDirect(hdc, circle.xc, circle.yc, circle.R, circle.color);
            break;
        case POLAR:
            CirclePolar(hdc, circle.xc, circle.yc, circle.R, circle.color);
            break;
        case ITERATIVE_POLAR:
            CircleIterativePolar(hdc, circle.xc, circle.yc, circle.R, circle.color);
            break;
        case MIDPOINT:
            CircleMidpoint(hdc, circle.xc, circle.yc, circle.R, circle.color);
            break;
        case MODIFIED_MIDPOINT:
            CircleModifiedMidpoint(hdc, circle.xc, circle.yc, circle.R, circle.color);
            break;
        case FILL_LINES:
            FillCircleWithLines(hdc, circle.xc, circle.yc, circle.R, circle.quarter, circle.color);
            CircleDirect(hdc, circle.xc, circle.yc, circle.R, circle.color); // Draw outline
            break;
        case FILL_CIRCLES:
            FillCircleWithCircles(hdc, circle.xc, circle.yc, circle.R, circle.quarter, circle.color);
            CircleDirect(hdc, circle.xc, circle.yc, circle.R, circle.color); 
            break;
        }
    }
}

void SaveData() {
    std::ofstream file("shapes.txt");
    // Save lines
    file << "Lines\n";
    for (auto& line : lines) {
        file << line.start.x << " " << line.start.y << " "
            << line.end.x << " " << line.end.y << " "
            << (int)GetRValue(line.color) << " "
            << (int)GetGValue(line.color) << " "
            << (int)GetBValue(line.color) << "\n";
    }
    // Save circles
    file << "Circles\n";
    for (auto& circle : circles) {
        file << circle.xc << " " << circle.yc << " " << circle.R << " "
            << (int)GetRValue(circle.color) << " "
            << (int)GetGValue(circle.color) << " "
            << (int)GetBValue(circle.color) << " "
            << circle.quarter << " " << circle.algorithm << "\n";
    }
    file.close();
    std::cout << "Saved " << lines.size() << " line(s) and " << circles.size() << " circle(s) to shapes.txt\n";
}

void LoadData(HWND hwnd) {
    std::ifstream file("shapes.txt");
    if (!file.is_open()) {
        std::cout << "Could not open shapes.txt\n";
        return;
    }

    lines.clear();
    circles.clear();
    std::string lineType;
    int countLines = 0, countCircles = 0;

    while (std::getline(file, lineType)) {
        if (lineType == "Lines") {
            Line l;
            int r, g, b;
            while (file >> l.start.x >> l.start.y >> l.end.x >> l.end.y >> r >> g >> b) {
                l.color = RGB(r, g, b);
                lines.push_back(l);
                countLines++;
                if (file.peek() == '\n') break; // Move to next line type
            }
        }
        else if (lineType == "Circles") {
            Circle c;
            int r, g, b;
            while (file >> c.xc >> c.yc >> c.R >> r >> g >> b >> c.quarter >> c.algorithm) {
                c.color = RGB(r, g, b);
                circles.push_back(c);
                countCircles++;
                if (file.peek() == EOF || file.peek() == '\n') break;
            }
        }
    }
    file.close();
    std::cout << "Loaded " << countLines << " line(s) and " << countCircles << " circle(s) from shapes.txt\n";
    InvalidateRect(hwnd, NULL, TRUE);
}

void AddMenus(HWND hwnd) {
    HMENU hMenubar = CreateMenu();
    HMENU hFile = CreateMenu();
    HMENU hColor = CreateMenu();
    HMENU hLineAlgorithms = CreateMenu();
    HMENU hCircleAlgorithms = CreateMenu();
    HMENU hQuarter = CreateMenu();

    // File menu
    AppendMenu(hFile, MF_STRING, ID_BACKGROUND_WHITE, "Set Background White");
    AppendMenu(hFile, MF_STRING, ID_SCREEN_CLEAR, "Clear Screen");
    AppendMenu(hFile, MF_STRING, ID_SAVE, "Save");
    AppendMenu(hFile, MF_STRING, ID_LOAD, "Load");
    AppendMenu(hFile, MF_SEPARATOR, 0, NULL);
    AppendMenu(hFile, MF_STRING, ID_FILE_EXIT, "Exit");

    // Color menu
    AppendMenu(hColor, MF_STRING, ID_COLOR_RED, "Red");
    AppendMenu(hColor, MF_STRING, ID_COLOR_GREEN, "Green");
    AppendMenu(hColor, MF_STRING, ID_COLOR_BLUE, "Blue");

    // Line algorithms
    AppendMenu(hLineAlgorithms, MF_STRING, ID_LINE_BRESENHAM, "Bresenham Line");
    AppendMenu(hLineAlgorithms, MF_SEPARATOR, 0, NULL);
    AppendMenu(hLineAlgorithms, MF_STRING, 0, "Select to draw lines");

    // Circle algorithms
    AppendMenu(hCircleAlgorithms, MF_STRING, ID_CIRCLE_DIRECT, "Direct Circle");
    AppendMenu(hCircleAlgorithms, MF_STRING, ID_CIRCLE_POLAR, "Polar Circle");
    AppendMenu(hCircleAlgorithms, MF_STRING, ID_CIRCLE_ITERATIVE_POLAR, "Iterative Polar Circle");
    AppendMenu(hCircleAlgorithms, MF_STRING, ID_CIRCLE_MIDPOINT, "Midpoint Circle");
    AppendMenu(hCircleAlgorithms, MF_STRING, ID_CIRCLE_MODIFIED_MIDPOINT, "Modified Midpoint Circle");
    AppendMenu(hCircleAlgorithms, MF_STRING, ID_CIRCLE_FILL_LINES, "Fill with Lines");
    AppendMenu(hCircleAlgorithms, MF_STRING, ID_CIRCLE_FILL_CIRCLES, "Fill with Circles");
    AppendMenu(hCircleAlgorithms, MF_SEPARATOR, 0, NULL);
    AppendMenu(hCircleAlgorithms, MF_STRING, 0, "Select to draw circles");

    // Quarter selection
    AppendMenu(hQuarter, MF_STRING, ID_QUARTER_1, "Quarter 1 (Top-Right)");
    AppendMenu(hQuarter, MF_STRING, ID_QUARTER_2, "Quarter 2 (Top-Left)");
    AppendMenu(hQuarter, MF_STRING, ID_QUARTER_3, "Quarter 3 (Bottom-Left)");
    AppendMenu(hQuarter, MF_STRING, ID_QUARTER_4, "Quarter 4 (Bottom-Right)");

    // Main menu
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hFile, "File");
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hColor, "Shape Color");
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hLineAlgorithms, "Line Algorithms");
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hCircleAlgorithms, "Circle Algorithms");
    AppendMenu(hMenubar, MF_POPUP, (UINT_PTR)hQuarter, "Quarter Selection");

    SetMenu(hwnd, hMenubar);
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        AddMenus(hwnd);
        break;

    case WM_COMMAND:
        switch (wp) {
        case ID_FILE_EXIT:
            PostQuitMessage(0);
            break;
        case ID_COLOR_RED:
            currentColor = RGB(255, 0, 0);
            break;
        case ID_COLOR_GREEN:
            currentColor = RGB(0, 255, 0);
            break;
        case ID_COLOR_BLUE:
            currentColor = RGB(0, 0, 255);
            break;
        case ID_BACKGROUND_WHITE:
            bgBrush = CreateSolidBrush(RGB(255, 255, 255));
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case ID_SCREEN_CLEAR:
            lines.clear();
            circles.clear();
            InvalidateRect(hwnd, NULL, TRUE);
            break;
        case ID_SAVE:
            SaveData();
            break;
        case ID_LOAD:
            LoadData(hwnd);
            break;
        case ID_LINE_BRESENHAM:
            currentShapeType = LINE;
            currentLineAlgorithm = BRESENHAM;
            break;
        case ID_CIRCLE_DIRECT:
            currentShapeType = CIRCLE;
            currentCircleAlgorithm = DIRECT;
            break;
        case ID_CIRCLE_POLAR:
            currentShapeType = CIRCLE;
            currentCircleAlgorithm = POLAR;
            break;
        case ID_CIRCLE_ITERATIVE_POLAR:
            currentShapeType = CIRCLE;
            currentCircleAlgorithm = ITERATIVE_POLAR;
            break;
        case ID_CIRCLE_MIDPOINT:
            currentShapeType = CIRCLE;
            currentCircleAlgorithm = MIDPOINT;
            break;
        case ID_CIRCLE_MODIFIED_MIDPOINT:
            currentShapeType = CIRCLE;
            currentCircleAlgorithm = MODIFIED_MIDPOINT;
            break;
        case ID_CIRCLE_FILL_LINES:
            currentShapeType = CIRCLE;
            currentCircleAlgorithm = FILL_LINES;
            break;
        case ID_CIRCLE_FILL_CIRCLES:
            currentShapeType = CIRCLE;
            currentCircleAlgorithm = FILL_CIRCLES;
            break;
        case ID_QUARTER_1:
            currentQuarter = 1;
            break;
        case ID_QUARTER_2:
            currentQuarter = 2;
            break;
        case ID_QUARTER_3:
            currentQuarter = 3;
            break;
        case ID_QUARTER_4:
            currentQuarter = 4;
            break;
        }
        break;

    case WM_LBUTTONDOWN: {
        tempPoint.x = LOWORD(lp);
        tempPoint.y = HIWORD(lp);
        firstClick = true;
        SetCapture(hwnd);
        break;
    }

    case WM_LBUTTONUP: {
        POINT endPoint;
        endPoint.x = LOWORD(lp);
        endPoint.y = HIWORD(lp);
        HDC hdc = GetDC(hwnd);
        if (currentShapeType == LINE) {
            Line l;
            l.start = tempPoint;
            l.end = endPoint;
            l.color = currentColor;
            lines.push_back(l);
        }
        else if (currentShapeType == CIRCLE) {
            Circle c;
            c.xc = tempPoint.x;
            c.yc = tempPoint.y;
            c.R = (int)sqrt((endPoint.x - tempPoint.x) * (endPoint.x - tempPoint.x) +
                (endPoint.y - tempPoint.y) * (endPoint.y - tempPoint.y));
            c.color = currentColor;
            c.quarter = currentQuarter;
            c.algorithm = currentCircleAlgorithm;
            circles.push_back(c);
        }
        ReleaseCapture();
        InvalidateRect(hwnd, NULL, TRUE);
        ReleaseDC(hwnd, hdc);
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        FillRect(hdc, &ps.rcPaint, bgBrush);
        DrawAllShapes(hdc);
        EndPaint(hwnd, &ps);
        break;
    }

    case WM_SETCURSOR: {
        SetCursor(LoadCursor(NULL, IDC_CROSS));
        return TRUE;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wp, lp);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR args, int ncmdshow) {
    AllocConsole();
    FILE* stream;
    freopen_s(&stream, "CONOUT$", "w", stdout);

    std::cout << "Drawing App Started. Console linked.\n";

    WNDCLASSW wc = { 0 };
    wc.hbrBackground = bgBrush;
    wc.hCursor = LoadCursor(NULL, IDC_CROSS); // Use cross cursor consistently
    wc.hInstance = hInst;
    wc.lpszClassName = L"DrawingAppClass";
    wc.lpfnWndProc = WindowProcedure;

    if (!RegisterClassW(&wc))
        return -1;

    CreateWindowW(L"DrawingAppClass", L"2D Drawing Program", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        100, 100, 800, 600, NULL, NULL, NULL, NULL);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
