
$Src = Join-Path -Path $PSScriptRoot -ChildPath ".." -Resolve
$Folders = @(".", "debug", "operators")
$Extensions = @("*.h", "*.hpp")
$OutputFile = "include_single/xrx/xrx_all.h"
$RegExpHeader = '#include(.*)((<(?<name>(.*))>)|"(?<name>(.*))")'
$RegExpPragmaOnce = '#pragma(.*)once'

function Get-RelativePath($Root, $FullPath)
{
    if ($FullPath.StartsWith($Root))
    {
        $start = $Root.Length
        $delta = $FullPath.Length - $start
        if ((-Not $Root.EndsWith('/')) -And (-Not $Root.EndsWith('\')))
        {
            $start = $start + 1
            $delta = $delta - 1
        }
        $path = $FullPath.Substring($start, $delta)
        return $path
    }
    throw "Failed to find $Root as beginning of $FullPath."
}

function Get-NormalizedPath($Path)
{
    return $Path.ToLower().Replace("\", "/")
}

function Get-OurHeaders()
{
    foreach ($folder in $Folders)
    {
        $path = Join-Path -Path $Src -ChildPath $folder -Resolve
        (Get-ChildItem -Path "$path\*" -Include $Extensions).FullName `
            | % { Get-RelativePath $Src $_ } `
            | % { Get-NormalizedPath $_ }
    }
}

function Get-FileIncludes($file)
{
    Get-Content $file | `
        % { $ok = $_ -match $RegExpHeader; if ($ok) { $Matches.name } }
}

function Get-FileOurIncludes($known_headers, $file, $folder = $Src)
{
    $path = Join-Path -Path $folder -ChildPath $file -Resolve
    Get-FileIncludes $path `
        | % { Get-NormalizedPath $_ } `
        | ? { $known_headers -contains $_ }
}

$our_headers = Get-OurHeaders
$includes = @{}
foreach ($own_header in $our_headers)
{
    $own_includes = @(Get-FileOurIncludes $our_headers $own_header)
    $includes[$own_header] = $own_includes
}

function Get-AllIncludesRecursive($file)
{
    $headers = $includes[$file]
    if ($headers.Count -eq 0)
    {
        return @()
    }
    $all_headers = $headers;
    foreach ($header in $headers)
    {
        $all_headers += Get-AllIncludesRecursive $header
    }
    return $all_headers | sort -Unique
}

$dependencies = @{}
foreach ($header in $our_headers)
{
    $dependencies[$header] = Get-AllIncludesRecursive $header
}

$info = @()
foreach ($key in $dependencies.Keys)
{
    $deps = $dependencies[$key]
    $info += [PSCustomObject]@{Header = $key; Deps = $deps; Count = $deps.Count}
}

function Get-Line($line)
{
    $ok = $_ -match $RegExpHeader
    if ($ok)
    {
        $name = Get-NormalizedPath $Matches.name
        if ($our_headers -contains $name)
        {
            return "// $line"
        }
    }
    $ok = $_ -match $RegExpPragmaOnce
    if ($ok)
    {
        return "// $line"
    }
    return $line
}

$ordered = $info | Sort -Property Count
$already_included = New-Object Collections.Generic.HashSet[string]

$OutFile = Join-Path -Path $Src -ChildPath $OutputFile
$prelude =
@'
#pragma once
// 
// Auto-generated single-header file:
// powershell -File tools/generate_single_header.ps1
// 
'@
$prelude | Out-File -FilePath $OutFile -Encoding utf8

foreach ($header in $ordered)
{
    $already_included.Add($header.Header) | Out-Null
    foreach ($dependency in $header.Deps)
    {
        if (-Not $already_included.Contains($dependency))
        {
            throw "Tryig to copy-paste $($header.Header) file, but its dependency is not yet included: $dependency."
        }
    }

    $path = Join-Path -Path $Src -ChildPath $header.Header

    "`n// Header: $($header.Header).`n" `
        | Out-File -FilePath $OutFile -Append -Encoding utf8
    Get-Content $path `
        | % { Get-Line $_ } `
        | Out-File -FilePath $OutFile -Append -Encoding utf8
}

