# Fails when a Tr()/Loc.T() call has no table entry, a table key is
# duplicated, or a format placeholder differs between Chinese and English.
# Dynamic keys (Loc.T(name), Tr(name)) cannot be checked here.
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$errors = [System.Collections.Generic.List[string]]::new()

function Read-Text([string]$path) {
    Get-Content -LiteralPath $path -Raw -Encoding utf8
}

function Line-Of([string]$text, [int]$index) {
    ($text.Substring(0, $index) -split "`n").Count
}

function Add-Duplicates($pairs, [string]$label) {
    $seen = @{}
    foreach ($pair in $pairs) {
        if ($seen.ContainsKey($pair.Key)) {
            $errors.Add("duplicate $label key: $($pair.Key)")
        } else {
            $seen[$pair.Key] = $pair.Value
        }
    }
    $seen
}

function Native-Placeholders([string]$text) {
    (@([regex]::Matches($text, '%[-+0-9.#]*[diufegsxX]') | ForEach-Object { $_.Value }) | Sort-Object) -join ','
}

function Managed-Placeholders([string]$text) {
    (@([regex]::Matches($text, '\{(\d+)(?:[^}]*)\}') | ForEach-Object { $_.Groups[1].Value }) |
        Sort-Object { [int]$_ }) -join ','
}

function Check-Calls {
    param(
        [string]$Label,
        [string]$Path,
        [string]$Pattern,
        [hashtable]$Table,
        [string]$PlaceholderKind
    )
    $text = Read-Text $Path
    foreach ($match in [regex]::Matches($text, $Pattern)) {
        $key = $match.Groups[1].Value
        if (-not $Table.ContainsKey($key)) {
            $line = Line-Of $text $match.Index
            $errors.Add("${Label}: ${Path}:${line}: missing ${PlaceholderKind} table entry for `"$key`"")
            continue
        }
        $en = $Table[$key]
        $zh = $key
        if ($PlaceholderKind -eq "native") {
            if ((Native-Placeholders $zh) -ne (Native-Placeholders $en)) {
                $line = Line-Of $text $match.Index
                $errors.Add("${Label}: ${Path}:${line}: placeholder mismatch for `"$key`" -> `"$en`"")
            }
        } else {
            if ((Managed-Placeholders $zh) -ne (Managed-Placeholders $en)) {
                $line = Line-Of $text $match.Index
                $errors.Add("${Label}: ${Path}:${line}: placeholder mismatch for `"$key`" -> `"$en`"")
            }
        }
    }
}

# --- the native table and its call sites ------------------------------------
$nativeDict = Read-Text (Join-Path $root "native/core/localization.cpp")
$nativePairs = [regex]::Matches($nativeDict, '\{\s*L"((?:[^"\\]|\\.)*)"\s*,\s*L"((?:[^"\\]|\\.)*)"\s*\}') |
    ForEach-Object { @{ Key = $_.Groups[1].Value; Value = $_.Groups[2].Value } }
$nativeTable = Add-Duplicates $nativePairs "native"
$nativeFiles = Get-ChildItem (Join-Path $root "native/core") -Recurse -File -Include *.cpp, *.h |
    Where-Object { $_.Name -ne "localization.cpp" }
foreach ($file in $nativeFiles) {
    Check-Calls -Label "native" -Path $file.FullName -Pattern '\bTr(?:Format)?\s*\(\s*L"((?:[^"\\]|\\.)*)"' `
        -Table $nativeTable -PlaceholderKind "native"
}

# --- the settings table and its call sites ----------------------------------
$settingsDict = Read-Text (Join-Path $root "settings/Atrium.Settings/Loc.cs")
$settingsPairs = [regex]::Matches($settingsDict, '\["((?:[^"\\]|\\.)*)"\]\s*=\s*"((?:[^"\\]|\\.)*)"') |
    ForEach-Object { @{ Key = $_.Groups[1].Value; Value = $_.Groups[2].Value } }
$settingsTable = Add-Duplicates $settingsPairs "settings"
$settingsFiles = Get-ChildItem (Join-Path $root "settings/Atrium.Settings") -File -Filter *.cs |
    Where-Object { $_.Name -ne "Loc.cs" }
foreach ($file in $settingsFiles) {
    Check-Calls -Label "settings" -Path $file.FullName -Pattern '\bLoc\.(?:T|F)\s*\(\s*"((?:[^"\\]|\\.)*)"' `
        -Table $settingsTable -PlaceholderKind "settings"
}

if ($errors.Count -gt 0) {
    foreach ($message in $errors) { Write-Host "ERROR: $message" }
    Write-Host "Localization check failed ($($errors.Count) error(s))."
    exit 1
}
Write-Host "Localization check passed: native $($nativeTable.Count) entries, settings $($settingsTable.Count) entries."
