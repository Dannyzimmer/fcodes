# mingw-prepare.ps1: Prepare the msys2 environment in GitLab CI

# Make sure powershell exists on errors
$ErrorActionPreference = "Stop"

# Disable progress bar to speed up download
$ProgressPreference = "SilentlyContinue"

# Disable Windows Defender
Add-MpPreference -ExclusionPath 'C:\'

# Install msys2
$msys2_installer = "msys2-x86_64-latest.sfx.exe"
Invoke-WebRequest https://repo.msys2.org/distrib/$msys2_installer -OutFile $env:TEMP\$msys2_installer
Invoke-Expression "$env:TEMP/$msys2_installer -y -oC:\"

# Update base packages and package database
# From https://github.com/msys2/setup-msys2/blob/main/main.js
C:\msys64\msys2_shell.cmd -defterm -here -no-start -l -c "pacman-key --init && pacman-key --populate msys2 && pacman-key --refresh-keys || echo errors ignored"
C:\msys64\msys2_shell.cmd -defterm -here -no-start -l -c "sed -i 's/^CheckSpace/#CheckSpace/g' /etc/pacman.conf"
C:\msys64\msys2_shell.cmd -defterm -here -no-start -l -c "pacman -Syuu --noconfirm || echo errors ignored"
taskkill.exe /F /FI "MODULES eq msys-2.0.dll"
C:\msys64\msys2_shell.cmd -defterm -here -no-start -l -c "pacman -Syuu --noconfirm"
