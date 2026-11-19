# Xozmag'ni yig'adi: build\Xozmag.exe va natija\Xozmag_Ornatuvchi_vX.Y.Z.exe
#   powershell -ExecutionPolicy Bypass -File qurish.ps1
# Kompilyator: WinLibs GCC (D:\mingw64\bin). Boshqa joyda bo'lsa $GCC ni o'zgartiring.
$ErrorActionPreference = 'Stop'
$ildiz = $PSScriptRoot
$GCC = 'D:\mingw64\bin'
$gpp = Join-Path $GCC 'g++.exe'
$windres = Join-Path $GCC 'windres.exe'
Set-Location $ildiz

$v = (Get-Content (Join-Path $ildiz 'versiya.txt') -Raw).Trim()
if ($v -notmatch '^(\d+)\.(\d+)\.(\d+)$') { throw "versiya.txt noto'g'ri: $v" }
$a, $b, $c = $Matches[1], $Matches[2], $Matches[3]
New-Item -ItemType Directory -Force build, natija | Out-Null
@"
#pragma once
#define VER_A $a
#define VER_B $b
#define VER_C $c
#define VER_MATN "$v"
#define XOZMAG_VERSIYA L"$v"
"@ | Set-Content -Encoding ASCII build\versiya_raqam.h

function Ishlat($exe, [string[]]$arg) {
    & $exe @arg
    if ($LASTEXITCODE -ne 0) { throw "$exe xato bilan tugadi ($LASTEXITCODE)" }
}

Write-Host "Xozmag v$v yig'ilmoqda..."
Ishlat $windres @('--use-temp-file', '-I', 'build', '-I', 'res', 'res\xozmag.rc', '-O', 'coff', '-o', 'build\xozmag_res.o')
$umumiy = @('-std=c++20', '-O2', '-Wall', '-Wno-unused-parameter', '-Wno-missing-field-initializers',
            '-municode', '-D_WIN32_WINNT=0x0A00', '-DNTDDI_VERSION=0x0A000005', '-Ibuild', '-Isrc')
# MinGW har bir exe'ga o'zining asInvoker manifestini (default-manifest.o) qo'shadi — bizniki
# (requireAdministrator, UAC) bilan ikkita bo'lib qoladi. -B orqali bo'sh nusxasi beriladi.
New-Item -ItemType Directory -Force build\bosh | Out-Null
if (-not (Test-Path build\bosh\default-manifest.o)) {
    '' | Set-Content -Encoding ASCII build\bosh\bosh.c
    Ishlat (Join-Path $GCC 'gcc.exe') @('-c', 'build\bosh\bosh.c', '-o', 'build\bosh\default-manifest.o')
}
$bog = @('-municode', '-mwindows', '-static', '-s', '-Bbuild\bosh\')
# Har bir .cpp alohida jarayonda, bir vaqtda — sekin kompyuterda ancha tezroq
New-Item -ItemType Directory -Force build\rel | Out-Null
$jarayonlar = @(); $obyektlar = @()
foreach ($f in Get-ChildItem src\*.cpp) {
    $o = "build\rel\$($f.BaseName).o"
    $obyektlar += $o
    $jarayonlar += Start-Process $gpp -ArgumentList ($umumiy + @('-c', "`"$($f.FullName)`"", '-o', $o)) `
        -NoNewWindow -PassThru
}
$jarayonlar | Wait-Process
foreach ($j in $jarayonlar) { if ($j.ExitCode -ne 0) { throw "Kompilyatsiya xatosi (yuqoriga qarang)" } }
Ishlat $gpp ($bog + $obyektlar + @('build\xozmag_res.o', 'tashqi\sqlite3.dll',
    '-ld2d1', '-ldwrite', '-lwindowscodecs', '-lshlwapi', '-lbcrypt',
    '-lcomctl32', '-lcomdlg32', '-lgdi32', '-lwinhttp', '-lole32', '-lshell32', '-luuid',
    '-lwinspool', '-lws2_32', '-liphlpapi', '-lnormaliz', '-lsetupapi', '-lcfgmgr32', '-o', 'build\Xozmag.exe'))
Copy-Item tashqi\sqlite3.dll build\sqlite3.dll -Force

Write-Host "O'rnatuvchi yig'ilmoqda..."
Ishlat $windres @('--use-temp-file', '-I', 'build', '-I', 'res', '-I', '.', 'ornatuvchi\ornatuvchi.rc', '-O', 'coff',
                  '-o', 'build\ornatuvchi_res.o')
$chiqish = "natija\Xozmag_Ornatuvchi_v$v.exe"
Ishlat $gpp ($umumiy + $bog + @('ornatuvchi\ornatuvchi.cpp', 'build\ornatuvchi_res.o', '-lcomctl32',
    '-lole32', '-lshell32', '-luuid', '-ladvapi32', '-o', $chiqish))
$hajm = [math]::Round((Get-Item $chiqish).Length / 1MB, 1)
Write-Host "Tayyor: $chiqish ($hajm MB)"
