# ExifStats
Exif statistics of your JPEG and HEIF library

![](ExifStats.png)

## Customize UI / QML

Download and extract the CustomizeQML.zip into the same directory as ExifStats.exe. Restart ExifStats.exe and it should load the QML from the Qml directory instead of the embedded one. It supports hot reload and all errors and warnings are logged into a file located in %localappdata%\ExifStats\ExifStats\ExifStats.log

## Compilation

# Windows
* Download and install Qt6.6+ https://doc.qt.io/qt-6/get-and-install-qt.html
  * MSVC 2019 64-bit
  * Sources
  * Qt 5 Compatibility Module
  * Qt Debug Information Files
  * Additional librairies:
    * Qt Image Formats
    * Qt Location
* Create a file **User.Setup.bat** at the root next to **CMakeLists.txt** with:
  * set QT_ROOT_DIR=c:\Dev\Qt
  * (optional) set QT_VERSION=6.6.2 (Project.Setup.bat already set a default value)
  * (optional) set QT_MSVC_DIR=msvc2019_64 (Project.Setup.bat already set a default value)
  * (optional) set HEIF_PLUGIN_ENABLE=true (See Heif / Turbojpeg plugins section)
  * (optional) set TURBOJPEG_PLUGIN_ENABLE=true (See Heif / Turbojpeg plugins section)
* Use a bat in the Scripts folder
  * **Scripts/CMake.bat**: CMake only
  * **Scripts/Build.bat**: Build release only (need CMake)
  * **Scripts/Deploy.bat**: Deploy files required to launch the debug/release/relwithdebuginfo binaries (dlls, qml ...)(need Build)
  * **Scripts/Run.bat**: Run the release bin (need Build and Deploy)
  * **Scripts/DeleteGenerated.bat**: Delete the Generated folder and all its content
  * **Scripts/OpenSln.bat**: Open the Visual Studio solution (need CMake)
  * **Scripts/OpenQtCreator.bat**: Open Qt Creator with the folder (Qt Creator is not officially supported)
  * **Scripts/BuildDeployRun.bat**: Build, Deploy, Run (Release)
  * **Scripts/CleanBuildDebugDeployOpenSln.bat**: Delete Generated, CMake, Build, Deploy, Open Sln (Debug)
  * **Scripts/CleanBuildRun.bat**: Delete Generated, CMake, Build, Run (Release)
* The binaries are and all generated files are located in the **generated** folder

# Other
Not supported. With a bit of work it should compile on all platforms supported by Qt.
  
## Static Compilation
### Compile Qt6
First you need to download and compile Qt in Static, if you don't want to use the turbo jpeg lib remove "-no-libjpeg"
```
call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d "C:\Dev\Qt\6.6.1\Src"
call configure.bat -prefix BuildStatic -static -static-runtime -no-libjpeg -release
cmake --build . --clean-first
cmake --install .
### Setup ExifStats to use Qt Static
This will build Qt in release only in "C:\Dev\Qt\6.6.1\Src\BuildStatic"
```
Then you need to set the following env var in User.Setup.bat or Project.Setup.bat:
```
set QT_STATIC_DIR=Src\BuildStatic
set QT_STATIC=true
```
## Heif / Turbojpeg plugins
### VCPKG / libheif / libjpeg-turbo
If you want to use the Heif and Turbojpeg plugins, you will need to setup vcpkg and install the libheif and turbojpeg libs:
```
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
vcpkg install libheif:x64-windows
vcpkg install libheif:x64-windows-static
vcpkg install libjpeg-turbo:x64-windows
vcpkg install libjpeg-turbo:x64-windows-static
```
### Setup ExifStats to use the plugins
Then set the some vcpkg var in the User.Setup.bat:
```
set VCPKG_TOOLCHAIN_FILE=C:/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake
set VCPKG_TARGET_TRIPLET=x64-windows
	or
set VCPKG_TARGET_TRIPLET=x64-windows-static
set HEIF_PLUGIN_ENABLE=true
set TURBOJPEG_PLUGIN_ENABLE=true
```
