@echo off
set JAVA_HOME="C:\Program Files\Java\jdk1.8.0_xx"
cl /LD /I "%JAVA_HOME%\include" /I "%JAVA_HOME%\include\win32" mmfplayer_native_jni.c /link /OUT:mmfplayer_native.dll
