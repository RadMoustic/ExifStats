# ExifStats
Exif statistics of your JPEG and HEIF library

![](ExifStats.png)

## Compilation

## Static Compilation
### Compile Qt6
First you need to download and compile Qt in Static, if you don't want to use the turbo jpeg lib remove "-no-libjpeg"

call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
cd /d "C:\Dev\Qt\6.6.1\Src"
call configure.bat -prefix BuildStatic -static -static-runtime -no-libjpeg -release
cmake --build . --clean-first
cmake --install .
### Setup ExifStats to use Qt Static
This will build Qt in release only in "C:\Dev\Qt\6.6.1\Src\BuildStatic"

Then you need to set the following env var in User.Setup.bat or Project.Setup.bat:

set QT_STATIC_DIR=Src\BuildStatic
set QT_STATIC=true

## Heif / Turbojpeg plugins
### VCPKG / libheif / libjpeg-turbo
If you want to use the Heif and Turbojpeg plugins, you will need to setup vcpkg and install the libheif and turbojpeg libs:

git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat
vcpkg install libheif:x64-windows
vcpkg install libheif:x64-windows-static
vcpkg install libjpeg-turbo:x64-windows
vcpkg install libjpeg-turbo:x64-windows-static

### Setup ExifStats to use the plugins
Then set the some vcpkg var in the User.Setup.bat:

set VCPKG_TOOLCHAIN_FILE=C:/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake
set VCPKG_TARGET_TRIPLET=x64-windows
	or
set VCPKG_TARGET_TRIPLET=x64-windows-static
