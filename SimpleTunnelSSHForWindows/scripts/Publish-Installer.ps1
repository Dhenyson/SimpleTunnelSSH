$ErrorActionPreference = 'Stop'

$projectPath = Join-Path $PSScriptRoot '..\src\SimpleTunnelSSH.App\SimpleTunnelSSH.App.csproj'
$installerPath = Join-Path $PSScriptRoot '..\installer\SimpleTunnelSSH.iss'
$dotnetPath = Join-Path $env:LOCALAPPDATA 'Microsoft\dotnet\dotnet.exe'
$iconGeneratorPath = Join-Path $PSScriptRoot 'Generate-AppIcon.ps1'
$publishedExePath = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\artifacts\publish\win-x64\SimpleTunnelSSH.exe'))

if (-not (Test-Path $dotnetPath)) {
    $dotnetPath = 'dotnet'
}

& $iconGeneratorPath

Get-Process SimpleTunnelSSH -ErrorAction SilentlyContinue |
    Where-Object {
        try {
            $_.Path -eq $publishedExePath
        }
        catch {
            $false
        }
    } |
    Stop-Process -Force

if (Test-Path $publishedExePath) {
    Remove-Item $publishedExePath -Force -ErrorAction SilentlyContinue
}

& $dotnetPath publish $projectPath -c Release -p:PublishProfile=WinX64SingleFile

$innoSetupCompiler = Get-Command iscc.exe -ErrorAction SilentlyContinue

if ($null -eq $innoSetupCompiler) {
    $knownCompilerPaths = @(
        (Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'),
        (Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6\ISCC.exe'),
        (Join-Path $env:ProgramFiles 'Inno Setup 6\ISCC.exe')
    )

    $compilerPath = $knownCompilerPaths | Where-Object { $_ -and (Test-Path $_) } | Select-Object -First 1

    if ($compilerPath) {
        & $compilerPath $installerPath
        exit $LASTEXITCODE
    }
}

if ($null -eq $innoSetupCompiler) {
    throw "Inno Setup Compiler was not found. Install it and rerun this script. Suggested command: winget install JRSoftware.InnoSetup"
}

& $innoSetupCompiler.Source $installerPath