
# MMFPlayer JNI Wrapper Project

This project provides a reverse-engineered JNI wrapper for `emulator.media.MMFPlayer`, dynamically linking to `ma3smwemu.dll`.

## Structure
- mmfplayer_native_jni.c — JNI implementation
- build.bat — Visual Studio build script
- build_mingw.sh — MinGW 32-bit build script
- .github/workflows/build-win32.yml — GitHub Actions workflow
- ma3smwemu.dll — dummy placeholder for linking and CI testing

## Build (Windows Visual Studio)
```
set JAVA_HOME=C:\Program Files\Java\jdk1.8.0_xx
cl /LD /I "%JAVA_HOME%\include" /I "%JAVA_HOME%\include\win32" mmfplayer_native_jni.c /link /OUT:mmfplayer_native.dll
```

## Build (MinGW 32-bit)
```
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk
i686-w64-mingw32-gcc -m32 -shared -o mmfplayer_native.dll mmfplayer_native_jni.c -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/win32" -Wl,--add-stdcall-alias
```

## GitHub Actions
Push to main branch — the CI will compile `mmfplayer_native.dll` using JDK 8 and MinGW.
