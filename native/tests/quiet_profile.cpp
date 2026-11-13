// The Do Not Disturb profile Atrium's "hide system banners" setting selects:
// anything short of alarms-only lets priority notifications (a system toast
// for a newly inserted USB drive, for example) through.

#include "shell/system_chrome.h"

#include <cstdio>
#include <string>

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

    auto plan = PlanQuiet(kQuietProfileOff, true, false, {});
    Check(plan.action == QuietAction::hide && plan.profile == kQuietProfile && plan.owns,
          "unrestricted is switched to alarms only");

    plan = PlanQuiet(L"Microsoft.QuietHoursProfile.PriorityOnly", true, false, {});
    Check(plan.action == QuietAction::hide && plan.profile == kQuietProfile && plan.owns,
          "priority only is switched to alarms only");

    plan = PlanQuiet(L"Microsoft.QuietHoursProfile.SomeNewMode", true, false, {});
    Check(plan.action == QuietAction::hide && plan.profile == kQuietProfile && plan.owns,
          "an unknown profile is switched to alarms only");

    plan = PlanQuiet(kQuietProfile, true, false, {});
    Check(plan.action == QuietAction::none, "alarms only is already what the setting asks for");

    plan = PlanQuiet(L"", true, false, {});
    Check(plan.action == QuietAction::none, "an unavailable service is left alone");

    plan = PlanQuiet(kQuietProfile, false, true, kQuietProfileOff);
    Check(plan.action == QuietAction::restore && plan.profile == kQuietProfileOff,
          "turning the setting off restores unrestricted");

    plan = PlanQuiet(kQuietProfile, false, true, L"Microsoft.QuietHoursProfile.PriorityOnly");
    Check(plan.action == QuietAction::restore && plan.profile == L"Microsoft.QuietHoursProfile.PriorityOnly",
          "turning the setting off restores the profile the user had");

    plan = PlanQuiet(kQuietProfile, false, true, L"");
    Check(plan.action == QuietAction::restore && plan.profile == kQuietProfileOff,
          "an empty previous profile falls back to unrestricted");

    plan = PlanQuiet(kQuietProfile, false, false, {});
    Check(plan.action == QuietAction::none, "a profile Atrium did not set is left alone");

    if (failures) {
        std::fprintf(stderr, "%d quiet profile check(s) failed\n", failures);
        return 1;
    }
    std::puts("quiet profile checks passed");
    return 0;
}
