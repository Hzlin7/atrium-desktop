#include "shell/usb_volume.h"

#include "log.h"

namespace atrium {

namespace {

std::wstring RootOf(wchar_t letter)
{
    return std::wstring(1, letter) + L":\\";
}

// The volume's label as of now; empty when it has none (or reads none: the
// letter is gone by the time the removal is noticed, so UsbVolumes keeps the
// labels of the drives it watches).
std::wstring LabelOf(wchar_t letter)
{
    wchar_t name[MAX_PATH + 1] = {};
    DWORD flags = 0;
    if (GetVolumeInformationW(RootOf(letter).c_str(), name, MAX_PATH + 1, nullptr, nullptr, &flags, nullptr, 0))
        return name;
    return {};
}

} // namespace

unsigned RemovableVolumeMask()
{
    const DWORD drives = GetLogicalDrives();
    unsigned mask = 0;
    for (int i = 0; i < 26; ++i)
        if (drives & (1u << i) && GetDriveTypeW(RootOf(wchar_t(L'A' + i)).c_str()) == DRIVE_REMOVABLE)
            mask |= 1u << i;
    return mask;
}

unsigned PresentVolumeMask()
{
    const DWORD drives = GetLogicalDrives();
    unsigned mask = 0;
    for (int i = 0; i < 26; ++i)
        if (drives & (1u << i))
            mask |= 1u << i;
    return mask;
}

UsbVolumes::UsbVolumes()
    : watched_(RemovableVolumeMask())
{
    for (wchar_t letter : RemovedLetters(watched_, 0))
        labels_[letter] = LabelOf(letter);
}

std::vector<RemovedVolume> UsbVolumes::Sync(const Event& event)
{
    // A REMOVEPENDING only precedes an orderly removal (an eject), never a
    // drive pulled out; an arrival of the same letter means that eject did not
    // happen after all.
    if (event.kind == DBT_DEVICEARRIVAL)
        ejecting_ &= ~event.unitmask;
    else if (event.kind == DBT_DEVICEREMOVEPENDING)
        ejecting_ |= event.unitmask;

    // A drive Windows reports as fixed (a USB disk) is watched from its volume
    // events: the letters of an arrival carrying DBTF_MEDIA join the watch.
    const unsigned present = PresentVolumeMask();
    if (event.media && event.kind == DBT_DEVICEARRIVAL)
        watched_ |= event.unitmask & present;
    const unsigned now = RemovableVolumeMask();
    watched_ |= now; // the removable drives present are watched from here on

    const unsigned gone = watched_ & ~present;
    std::vector<RemovedVolume> removed;
    for (wchar_t letter : RemovedLetters(watched_, present)) {
        RemovedVolume volume;
        volume.letter = letter;
        if (const auto it = labels_.find(letter); it != labels_.end())
            volume.label = it->second;
        volume.ejected = (ejecting_ & (1u << (letter - L'A'))) != 0;
        const std::wstring label = volume.label.empty() ? L"" : L" (" + volume.label + L")";
        Log(L"usb: %c: %s%s", letter, volume.ejected ? L"ejected" : L"removed", label.c_str());
        removed.push_back(std::move(volume));
    }
    watched_ &= present;
    ejecting_ &= ~gone; // the flags of the gone letters were spent on the report
    // The labels of the drives still watched are read again: they change (a
    // re-formatted drive, a card swapped in a reader) and an event costs no
    // more than a few reads.
    labels_.clear();
    for (wchar_t letter : RemovedLetters(watched_, 0))
        labels_[letter] = LabelOf(letter);
    return removed;
}

} // namespace atrium
