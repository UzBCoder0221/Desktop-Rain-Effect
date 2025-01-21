#include <windows.h>
#include <random>

const int NUM_RAINDROPS = 100;  // Reduced number of raindrops for performance
const int RAINDROP_SPEED_X = 3;  // Speed of horizontal movement
const int RAINDROP_SPEED_Y = 4;  // Speed of vertical movement

struct Raindrop {
    int x, y; // Position of the raindrop
    int length; // Length of the raindrop
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static Raindrop raindrops[NUM_RAINDROPS];
    static bool initialized = false;
    static HDC memDC = NULL;
    static HBITMAP memBitmap = NULL;
    static HPEN pen = NULL;  // Reused pen object

    if (!initialized) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> distX(0, 1920); // Screen width
        std::uniform_int_distribution<> distY(0, 1080); // Screen height
        std::uniform_int_distribution<> distLength(10, 30);

        for (int i = 0; i < NUM_RAINDROPS; ++i) {
            raindrops[i].x = distX(gen);
            raindrops[i].y = distY(gen);
            raindrops[i].length = distLength(gen);
        }

        // Set up the off-screen buffer (memory DC and bitmap)
        HDC hdc = GetDC(hwnd);
        memDC = CreateCompatibleDC(hdc);
        memBitmap = CreateCompatibleBitmap(hdc, 1920, 1080); // Match the screen resolution
        SelectObject(memDC, memBitmap);
        ReleaseDC(hwnd, hdc);

        // Create pen once for reuse
        pen = CreatePen(PS_SOLID, 3, RGB(173, 216, 230)); // Light blue raindrops

        initialized = true;
    }

    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Only need to do this once at the beginning or when the window is resized
        if (ps.rcPaint.left == 0 && ps.rcPaint.top == 0 && ps.rcPaint.right == 1920 && ps.rcPaint.bottom == 1080) {
            // Draw the static background once (avoid repeated fills)
            HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0)); // Black background
            FillRect(memDC, &ps.rcPaint, hBrush);  // Fill the window area
            DeleteObject(hBrush);
        }

        // Draw the raindrops to the memory DC (cached frame)
        SelectObject(memDC, pen);

        for (int i = 0; i < NUM_RAINDROPS; ++i) {
            MoveToEx(memDC, raindrops[i].x, raindrops[i].y, NULL);
            LineTo(memDC, raindrops[i].x + raindrops[i].length / .9, raindrops[i].y + raindrops[i].length / .5); // Slanted lines
        }

        // Copy the off-screen buffer to the screen (BitBlt operation)
        BitBlt(hdc, 0, 0, 1920, 1080, memDC, 0, 0, SRCCOPY);

        // Set transparency (remove black background from the window)
        SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER: {
        bool updated = false;  // Flag to track if we need to invalidate the window

        for (int i = 0; i < NUM_RAINDROPS; ++i) {
            raindrops[i].x += RAINDROP_SPEED_X; // Move to the right
            raindrops[i].y += RAINDROP_SPEED_Y; // Move downward

            // Check if raindrop is off-screen and reset it
            if (raindrops[i].x > 1920) {
                raindrops[i].x = -50;  // Reset to the left side off-screen
                raindrops[i].y = rand() % 1080;  // Reset to a random vertical position
                raindrops[i].length = rand() % 20 + 10; // Randomize length
                updated = true;
            }

            if (raindrops[i].y > 1080) {
                raindrops[i].x = rand() % 1920;  // Reset to a random horizontal position
                raindrops[i].y = -50;  // Start just above the screen
                raindrops[i].length = rand() % 20 + 10; // Randomize length
                updated = true;
            }
        }

        // Only invalidate if the raindrops moved (to reduce unnecessary redrawing)
        if (updated) {
            // Instead of invalidating the entire window, only invalidate the region where the raindrops are
            InvalidateRect(hwnd, NULL, TRUE); // Request window redraw
        }

        return 0;
    }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    const char CLASS_NAME[] = "RainEffect";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = nullptr;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT,
        CLASS_NAME,
        "Rain Effect",
        WS_POPUP,
        0, 0, 1920, 1080,
        nullptr, nullptr, hInstance, nullptr
    );

    if (hwnd == nullptr) return 0;

    // Set transparency
    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    ShowWindow(hwnd, nCmdShow);

    // Add a timer for updating the window (faster animation)
    SetTimer(hwnd, 1, 2, NULL); // Timer ID = 1, 20ms interval (~50 FPS)

    MSG msg = { };
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    KillTimer(hwnd, 1); // Clean up timer
    return 0;
}
