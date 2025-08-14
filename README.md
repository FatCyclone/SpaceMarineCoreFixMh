# SpaceMarineCoreFixMh
[![License](https://img.shields.io/badge/License-BSD%202--Clause-orange.svg)](https://opensource.org/licenses/BSD-2-Clause)

A rewrite of the [SpaceMarineCoreFix](https://github.com/adrian-lebioda/SpaceMarineCoreFix) project from Adrian Lebodia.

This one also impersonates DirectInput8 DLL and hooks necessary functions but with [MinHook](https://github.com/TsudaKageyu/minhook) instead of Microsoft [Detours](https://github.com/microsoft/Detours).
## USAGE
1. Download **DINPUT8.dll** from [releases](https://github.com/FatCyclone/SpaceMarineCoreFixMh/releases).
2. Download and install the latest [Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist).
3. Put the **DINPUT8.dll** file in the root folder of the game (%SteamInstallRoot%\Steam\steamapps\common\Warhammer 40,000 Space Marine)
4. Launch the game and enjoy !

## BUILD
Simply open the solution file and build the release version.
