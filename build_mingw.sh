#!/bin/bash
export JAVA_HOME=/usr/lib/jvm/java-8-openjdk
i686-w64-mingw32-gcc -m32 -shared -o mmfplayer_native.dll mmfplayer_native_jni.c -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/win32" -Wl,--add-stdcall-alias
