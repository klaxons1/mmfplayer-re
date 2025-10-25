
// mmfplayer_native_jni.c
// JNI wrapper for emulator.media.MMFPlayer using ma3smwemu.dll (Java 8 compatible)
#include <windows.h>
#include <jni.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define EXPORT __declspec(dllexport)

static HMODULE g_hMaSound = NULL;
static int g_instanceId = 1;
static int g_currentSound = -1;
static int g_isPlaying = 0;

typedef int (__cdecl *fn_MS_INIT)(void);
typedef int (__cdecl *fn_MS_CREATE)(int);
typedef int (__cdecl *fn_MS_LOAD)(int, int, int, int, int, int);
typedef int (__cdecl *fn_MS_OPEN)(int, int, int, int);
typedef int (__cdecl *fn_MS_STANDBY)(int, int, int);
typedef int (__cdecl *fn_MS_START)(int, int, int, int);
typedef int (__cdecl *fn_MS_STOP)(int, int, int);
typedef int (__cdecl *fn_MS_PAUSE)(int, int, int);
typedef int (__cdecl *fn_MS_RESTART)(int, int, int);
typedef int (__cdecl *fn_MS_CLOSE)(int, int, int);
typedef int (__cdecl *fn_MS_UNLOAD)(int, int, int);
typedef int (__cdecl *fn_MS_DELETE)(int);
typedef int (__cdecl *fn_MS_CONTROL)(int, int, int, void*, int);

static fn_MS_INIT MaSound_Initialize = NULL;
static fn_MS_CREATE MaSound_Create = NULL;
static fn_MS_LOAD MaSound_Load = NULL;
static fn_MS_OPEN MaSound_Open = NULL;
static fn_MS_STANDBY MaSound_Standby = NULL;
static fn_MS_START MaSound_Start = NULL;
static fn_MS_STOP MaSound_Stop = NULL;
static fn_MS_PAUSE MaSound_Pause = NULL;
static fn_MS_RESTART MaSound_Restart = NULL;
static fn_MS_CLOSE MaSound_Close = NULL;
static fn_MS_UNLOAD MaSound_Unload = NULL;
static fn_MS_DELETE MaSound_Delete = NULL;
static fn_MS_CONTROL MaSound_Control = NULL;

static const char *dll_name = "ma3smwemu.dll";

static int load_exports() {
    if (!g_hMaSound) return 0;
    MaSound_Initialize = (fn_MS_INIT)GetProcAddress(g_hMaSound, "MaSound_Initialize");
    MaSound_Create = (fn_MS_CREATE)GetProcAddress(g_hMaSound, "MaSound_Create");
    MaSound_Load = (fn_MS_LOAD)GetProcAddress(g_hMaSound, "MaSound_Load");
    MaSound_Open = (fn_MS_OPEN)GetProcAddress(g_hMaSound, "MaSound_Open");
    MaSound_Standby = (fn_MS_STANDBY)GetProcAddress(g_hMaSound, "MaSound_Standby");
    MaSound_Start = (fn_MS_START)GetProcAddress(g_hMaSound, "MaSound_Start");
    MaSound_Stop = (fn_MS_STOP)GetProcAddress(g_hMaSound, "MaSound_Stop");
    MaSound_Pause = (fn_MS_PAUSE)GetProcAddress(g_hMaSound, "MaSound_Pause");
    MaSound_Restart = (fn_MS_RESTART)GetProcAddress(g_hMaSound, "MaSound_Restart");
    MaSound_Close = (fn_MS_CLOSE)GetProcAddress(g_hMaSound, "MaSound_Close");
    MaSound_Unload = (fn_MS_UNLOAD)GetProcAddress(g_hMaSound, "MaSound_Unload");
    MaSound_Delete = (fn_MS_DELETE)GetProcAddress(g_hMaSound, "MaSound_Delete");
    MaSound_Control = (fn_MS_CONTROL)GetProcAddress(g_hMaSound, "MaSound_Control");
    return 1;
}

JNIEXPORT jint JNICALL Java_emulator_media_MMFPlayer_initMMFLibrary(JNIEnv *env, jclass cls, jstring jpath) {
    (void)cls;
    const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (!path) return -1;
    g_hMaSound = LoadLibraryA(dll_name);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    if (!g_hMaSound) return -1;
    load_exports();
    if (MaSound_Initialize) MaSound_Initialize();
    if (MaSound_Create) MaSound_Create(g_instanceId);
    return 1;
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_initPlayer(JNIEnv *env, jclass cls, jbyteArray arr) {
    (void)cls;
    if (!MaSound_Load) return;
    jbyte buf[4]; (*env)->GetByteArrayRegion(env, arr, 0, 4, buf);
    int id = buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24);
    if (MaSound_Load) MaSound_Load(g_instanceId, id, 0, 1, 0, 0);
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_play(JNIEnv *env, jclass cls, jint loops, jint vol) {
    (void)env; (void)cls;
    unsigned char v = (unsigned char)(25 * (vol & 0xFF));
    if (MaSound_Control) MaSound_Control(g_instanceId, g_currentSound, 0, &v, 0);
    if (MaSound_Start) MaSound_Start(g_instanceId, g_currentSound, loops, 0);
    g_isPlaying = 1;
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_stop(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    if (MaSound_Stop) MaSound_Stop(g_instanceId, g_currentSound, 0);
    g_isPlaying = 0;
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_pause(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    if (MaSound_Pause) MaSound_Pause(g_instanceId, g_currentSound, 0);
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_resume(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    if (MaSound_Restart) MaSound_Restart(g_instanceId, g_currentSound, 0);
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_destroy(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    if (MaSound_Stop) MaSound_Stop(g_instanceId, g_currentSound, 0);
    if (MaSound_Delete) MaSound_Delete(g_instanceId);
    if (g_hMaSound) { FreeLibrary(g_hMaSound); g_hMaSound = NULL; }
    g_isPlaying = 0;
}

JNIEXPORT jboolean JNICALL Java_emulator_media_MMFPlayer_isPlaying(JNIEnv *env, jclass cls) {
    (void)env; (void)cls;
    return g_isPlaying ? JNI_TRUE : JNI_FALSE;
}

BOOL APIENTRY DllMain(HMODULE h, DWORD reason, LPVOID r) {
    (void)h; (void)r;
    return TRUE;
}
