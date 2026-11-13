// Whether two clicks are one double click: the same thing clicked twice within
// Windows' double-click time. The shell counts a pair that way (USER32 turns the
// second click into WM_LBUTTONDBLCLK); Atrium's popups are not registered with
// CS_DBLCLKS, so the panel that forwards tray clicks counts it itself.
#pragma once

#include <string>

namespace atrium {

struct DoubleClick {
    // Records a click on `id` at `now` (ms of an increasing clock): true when it
    // completes the pair, that is a second click on the same id no later than
    // `window` ms after the one before. A pair ends the count; any other click
    // is the first of the next one.
    bool Click(const std::wstring& id, unsigned long long now, unsigned long long window)
    {
        if (!id.empty() && id == last && now - lastMs <= window) {
            Clear();
            return true;
        }
        last = id;
        lastMs = now;
        return false;
    }
    // Whether a click waits for its second one.
    bool Waiting() const { return !last.empty(); }
    void Clear()
    {
        last.clear();
        lastMs = 0;
    }

private:
    std::wstring last;
    unsigned long long lastMs = 0;
};

} // namespace atrium
