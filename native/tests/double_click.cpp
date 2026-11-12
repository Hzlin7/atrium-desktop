#include "ui/double_click.h"

#include <iostream>
#include <stdexcept>

using namespace atrium;

namespace {
void Require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

// Windows' double-click time is what the panel passes; 500 ms stands for it here.
constexpr unsigned long long kWindow = 500;

void CheckPair()
{
    DoubleClick clicks;
    Require(!clicks.Waiting(), "nothing waits before the first click");
    Require(!clicks.Click(L"icon:1", 1000, kWindow), "one click is not a double click");
    Require(clicks.Waiting(), "a click waits for its second one");
    Require(clicks.Click(L"icon:1", 1300, kWindow), "a second click on the same icon within the window is a double click");
    Require(!clicks.Waiting(), "a double click leaves nothing waiting");
    Require(!clicks.Click(L"icon:1", 1400, kWindow), "the click after a double click starts over");
    Require(!clicks.Click(L"icon:1", 2000, kWindow), "a second click too late is not a double click");
}

void CheckOtherIcons()
{
    DoubleClick clicks;
    Require(!clicks.Click(L"icon:1", 1000, kWindow), "the first click on an icon is not a double click");
    Require(!clicks.Click(L"icon:2", 1100, kWindow), "clicks on two different icons are not a double click");
    Require(!clicks.Click(L"icon:2", 1700, kWindow), "the window counts from the click before, not the first");
    Require(!clicks.Click(L"", 2000, kWindow) && !clicks.Click(L"", 2001, kWindow), "an unnamed hit never completes a pair");
}

void CheckEdges()
{
    DoubleClick clicks;
    Require(!clicks.Click(L"icon:1", 1000, kWindow), "a click at the window's edge is the first");
    Require(clicks.Click(L"icon:1", 1500, kWindow), "a second click right at the window's edge still counts");
    DoubleClick cleared;
    Require(!cleared.Click(L"icon:1", 1000, kWindow), "a click before a clear is the first");
    cleared.Clear();
    Require(!cleared.Click(L"icon:1", 1100, kWindow), "a cleared click is forgotten");
    DoubleClick backwards;
    Require(!backwards.Click(L"icon:1", 5000, kWindow) && !backwards.Click(L"icon:1", 4000, kWindow),
        "a clock that goes backwards does not make a double click");
}

} // namespace

int main()
{
    try {
        CheckPair();
        CheckOtherIcons();
        CheckEdges();
        std::cout << "Double-click pairing checks passed.\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
