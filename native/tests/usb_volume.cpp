// The pure parts of the USB removal detection: which drive letters went, and
// how the card names them.

#include "shell/usb_volume.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

int failures = 0;

void Check(bool ok, const char* what)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", what);
        ++failures;
    }
}

} // namespace

int main()
{
    using namespace atrium;

    auto letters = RemovedLetters(0x000A, 0x0002); // B and D were watched, B stayed
    Check(letters.size() == 1 && letters[0] == L'D', "the letter that went is found");

    Check(RemovedLetters(0x03FFFFFF, 0).size() == 26, "every letter can go at once");
    Check(RemovedLetters(0x0002, 0x000A).empty(), "a letter that arrived is no removal");

    Check(VolumeName(L"BACKUP", L'E') == L"BACKUP (E:)", "a labelled volume is named by its label");
    Check(VolumeName(L"", L'E') == L"E:", "a volume without a label is named by its letter");

    Check(JoinNames({L"E:"}) == L"E:", "a single name is not joined");
    Check(JoinNames({L"E:", L"F:"}) == std::wstring(L"E:") + Tr(L"、") + L"F:", "the names are joined");

    if (failures) {
        std::fprintf(stderr, "%d usb volume check(s) failed\n", failures);
        return 1;
    }
    std::puts("usb volume checks passed");
    return 0;
}
