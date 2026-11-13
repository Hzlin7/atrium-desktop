// Whether two clicks are one double click: the same thing clicked twice within
// Windows' double-click time. The shell counts a pair that way (USER32 turns the
// second click into WM_LBUTTONDBLCLK); Atrium's popups are not registered with
// CS_DBLCLKS, so the panel that forwards tray clicks counts it itself.
#pragma once

#include <string>
#include <vector>

namespace atrium {

// Display positions and tooltips can change between clicks. Prefer Windows'
// persistent record; an otherwise unknown UIA button has its own runtime ID.
// Without either identity, leave the clicks unpaired.
inline std::wstring TrayClickIdentity(const std::wstring& recordId, const std::vector<int>& runtimeId)
{
    if (!recordId.empty())
        return L"record:" + recordId;
    if (runtimeId.empty())
        return {};
    std::wstring id = L"uia";
    for (int part : runtimeId)
        id += L":" + std::to_wstring(part);
    return id;
}

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
    bool WaitingFor(const std::wstring& id) const { return !id.empty() && id == last; }
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
