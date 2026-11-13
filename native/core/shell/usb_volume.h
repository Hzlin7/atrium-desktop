// Removable drives that went, for a card of Atrium's own. Windows' own
// "safe to remove" toast is a balloon of its tray icon, and Do Not Disturb
// swallows it whole (it never reaches the notification centre, where the
// listener reads), so an eject is mirrored by nothing: the host window's
// WM_DEVICECHANGE events are read here instead.
#pragma once

#include "localization.h"

#include <windows.h>
#include <dbt.h>

#include <map>
#include <string>
#include <vector>

namespace atrium {

// A removable volume that has gone: its letter, the label it had, and whether
// it was ejected (an orderly removal) rather than pulled out.
struct RemovedVolume {
    wchar_t letter = 0;  // 'E'
    std::wstring label;  // the volume label, empty when it had none
    bool ejected = false;
};

// The letters set in `before` and not in `after`, ascending (bit 0 is A:).
inline std::vector<wchar_t> RemovedLetters(unsigned before, unsigned after)
{
    std::vector<wchar_t> letters;
    const unsigned gone = before & ~after;
    for (int i = 0; i < 26; ++i)
        if (gone & (1u << i))
            letters.push_back(wchar_t(L'A' + i));
    return letters;
}

// How a card names a volume: "label (E:)", or just "E:" when it had no label.
inline std::wstring VolumeName(const std::wstring& label, wchar_t letter)
{
    const std::wstring drive = std::wstring(1, letter) + L":";
    return label.empty() ? drive : label + L" (" + drive + L")";
}

// The names of one removal, joined for the card's body.
inline std::wstring JoinNames(const std::vector<std::wstring>& names)
{
    std::wstring joined;
    for (const auto& name : names) {
        if (!joined.empty())
            joined += Tr(L"、");
        joined += name;
    }
    return joined;
}

// The drive letters Windows reports as removable right now, as a bit mask
// (bit 0 = A:, bit 1 = B: ...).
unsigned RemovableVolumeMask();

// Every drive letter present right now, as a bit mask.
unsigned PresentVolumeMask();

class UsbVolumes {
public:
    // The WM_DEVICECHANGE that brought an event: its kind, and the volume
    // payload's unitmask and DBTF_MEDIA flag (both empty for other payloads,
    // which only ask for a re-read of the drives).
    struct Event {
        UINT kind = 0; // DBT_DEVICEARRIVAL, DBT_DEVICEREMOVEPENDING, DBT_DEVICEREMOVECOMPLETE
        unsigned unitmask = 0;
        bool media = false;
    };

    UsbVolumes(); // records what is attached now, so a drive present since before is watched too

    // Reads the drives again. The watched letters that have gone since the
    // last call come back, most recently seen labels included.
    std::vector<RemovedVolume> Sync(const Event& event);

private:
    unsigned watched_ = 0;  // the letters to report when they go
    unsigned ejecting_ = 0; // the letters of a device Windows is ejecting (a REMOVEPENDING came)
    std::map<wchar_t, std::wstring> labels_;
};

} // namespace atrium
