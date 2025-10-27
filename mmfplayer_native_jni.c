// mmfplayer_rewrite.c
// Полностью совместимая JNI-версия реконструированного mmfplayer.dll
// Собирается под x86, Visual Studio 2022, совместима с Java классом emulator.media.MMFPlayer

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <jni.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

// ---------- Типы для функций MaSound ----------
typedef int (__cdecl *PFN_MaSound_Initialize)(void);
typedef int (__cdecl *PFN_MaSound_Create)(int);
typedef int (__cdecl *PFN_MaSound_Load)(int, int, int, int, int, int);
typedef int (__cdecl *PFN_MaSound_Open)(int, int, int, int);
typedef int (__cdecl *PFN_MaSound_Standby)(int, int, int);
typedef int (__cdecl *PFN_MaSound_Seek)(int, int, int, int, int);
typedef int (__cdecl *PFN_MaSound_Start)(int, int, int, int);
typedef int (__cdecl *PFN_MaSound_Stop)(int, int, int);
typedef int (__cdecl *PFN_MaSound_Pause)(int, int, int);
typedef int (__cdecl *PFN_MaSound_Restart)(int, int, int);
typedef int (__cdecl *PFN_MaSound_Close)(int, int, int);
typedef int (__cdecl *PFN_MaSound_Unload)(int, int, int);
typedef int (__cdecl *PFN_MaSound_Delete)(int);
typedef int (__cdecl *PFN_MaSound_Control)(int, int, int, void*, int);
typedef int (__cdecl *PFN_MaSound_End)(void);

// ---------- Глобальные переменные ----------
static HMODULE g_hMaSoundModule = NULL;

static PFN_MaSound_Initialize g_pMaSound_Initialize = NULL;
static PFN_MaSound_Create     g_pMaSound_Create     = NULL;
static PFN_MaSound_Load       g_pMaSound_Load       = NULL;
static PFN_MaSound_Open       g_pMaSound_Open       = NULL;
static PFN_MaSound_Standby    g_pMaSound_Standby    = NULL;
static PFN_MaSound_Seek       g_pMaSound_Seek       = NULL;
static PFN_MaSound_Start      g_pMaSound_Start      = NULL;
static PFN_MaSound_Stop       g_pMaSound_Stop       = NULL;
static PFN_MaSound_Pause      g_pMaSound_Pause      = NULL;
static PFN_MaSound_Restart    g_pMaSound_Restart    = NULL;
static PFN_MaSound_Close      g_pMaSound_Close      = NULL;
static PFN_MaSound_Unload     g_pMaSound_Unload     = NULL;
static PFN_MaSound_Delete     g_pMaSound_Delete     = NULL;
static PFN_MaSound_Control    g_pMaSound_Control    = NULL;
static PFN_MaSound_End        g_pMaSound_End_Func   = NULL;

static int g_MaSoundInstance  = 1;
static int g_currentSoundId   = -1;
static int g_isPlayingFlag    = 0;

static HANDLE g_hHeap = NULL;

// ---------- Вспомогательные функции ----------
static void EnsureHeap()
{
    if (!g_hHeap) {
        g_hHeap = HeapCreate(0, 4096, 0);
        if (!g_hHeap)
            g_hHeap = GetProcessHeap();
    }
}

static void* mmf_malloc(size_t size)
{
    EnsureHeap();
    if (size == 0) size = 1;
    size_t aligned = (size + 15) & ~((size_t)15);
    return HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, aligned);
}

static void mmf_free(void *ptr)
{
    if (ptr && g_hHeap)
        HeapFree(g_hHeap, 0, ptr);
}

static void ResetMaSoundFunctionPointers()
{
    g_pMaSound_Initialize = NULL;
    g_pMaSound_Create     = NULL;
    g_pMaSound_Load       = NULL;
    g_pMaSound_Open       = NULL;
    g_pMaSound_Standby    = NULL;
    g_pMaSound_Seek       = NULL;
    g_pMaSound_Start      = NULL;
    g_pMaSound_Stop       = NULL;
    g_pMaSound_Pause      = NULL;
    g_pMaSound_Restart    = NULL;
    g_pMaSound_Close      = NULL;
    g_pMaSound_Unload     = NULL;
    g_pMaSound_Delete     = NULL;
    g_pMaSound_Control    = NULL;
    g_pMaSound_End_Func   = NULL;
}

static void LoadMaSoundExports()
{
    if (!g_hMaSoundModule) return;
    #define LOAD_FN(name) (FARPROC)GetProcAddress(g_hMaSoundModule, name)
    g_pMaSound_Initialize = (PFN_MaSound_Initialize) LOAD_FN("MaSound_Initialize");
    g_pMaSound_Create     = (PFN_MaSound_Create)     LOAD_FN("MaSound_Create");
    g_pMaSound_Load       = (PFN_MaSound_Load)       LOAD_FN("MaSound_Load");
    g_pMaSound_Open       = (PFN_MaSound_Open)       LOAD_FN("MaSound_Open");
    g_pMaSound_Standby    = (PFN_MaSound_Standby)    LOAD_FN("MaSound_Standby");
    g_pMaSound_Seek       = (PFN_MaSound_Seek)       LOAD_FN("MaSound_Seek");
    g_pMaSound_Start      = (PFN_MaSound_Start)      LOAD_FN("MaSound_Start");
    g_pMaSound_Stop       = (PFN_MaSound_Stop)       LOAD_FN("MaSound_Stop");
    g_pMaSound_Pause      = (PFN_MaSound_Pause)      LOAD_FN("MaSound_Pause");
    g_pMaSound_Restart    = (PFN_MaSound_Restart)    LOAD_FN("MaSound_Restart");
    g_pMaSound_Close      = (PFN_MaSound_Close)      LOAD_FN("MaSound_Close");
    g_pMaSound_Unload     = (PFN_MaSound_Unload)     LOAD_FN("MaSound_Unload");
    g_pMaSound_Delete     = (PFN_MaSound_Delete)     LOAD_FN("MaSound_Delete");
    g_pMaSound_Control    = (PFN_MaSound_Control)    LOAD_FN("MaSound_Control");
    g_pMaSound_End_Func   = (PFN_MaSound_End)        LOAD_FN("MaSound_End");
    #undef LOAD_FN
}

// ---------- Внутренние реализации ----------
int MMF_initMMFLibrary_internal(const char *libPath)
{
    int createResult = -1;
    if (!libPath || !libPath[0])
        return -1;

    g_hMaSoundModule = LoadLibraryA(libPath);
    if (!g_hMaSoundModule) {
        ResetMaSoundFunctionPointers();
        return -1;
    }

    LoadMaSoundExports();
    if (g_pMaSound_Initialize)
        createResult = g_pMaSound_Initialize();

    if (g_pMaSound_Create)
        createResult = g_pMaSound_Create(g_MaSoundInstance);

    EnsureHeap();
    return createResult < 0 ? -1 : 1;
}

void MMF_initPlayer_internal()
{
    int v6 = 0;
    int v4 = 0;
    int loadRes = -1;

    if (g_currentSoundId != -1) {
        if (g_pMaSound_Close)
            g_pMaSound_Close(g_MaSoundInstance, g_currentSoundId, 0);
        if (g_pMaSound_Unload)
            g_pMaSound_Unload(g_MaSoundInstance, g_currentSoundId, 0);
        g_currentSoundId = -1;
    }

    if (g_pMaSound_Load)
        loadRes = g_pMaSound_Load(g_MaSoundInstance, v6, v4, 1, 0, 0);

    if (loadRes >= 0) {
        g_currentSoundId = loadRes;
        if (g_pMaSound_Open)
            g_pMaSound_Open(g_MaSoundInstance, g_currentSoundId, 0, 0);
        if (g_pMaSound_Standby)
            g_pMaSound_Standby(g_MaSoundInstance, g_currentSoundId, 0);
        if (g_pMaSound_Control)
            g_pMaSound_Control(g_MaSoundInstance, g_currentSoundId, 5, NULL, 0);
    }
}

void MMF_play_internal(int a, int b)
{
    if (g_isPlayingFlag)
        if (g_pMaSound_Stop)
            g_pMaSound_Stop(g_MaSoundInstance, g_currentSoundId, 0);

    char controlByte = (char)(25 * b);
    if (g_pMaSound_Control)
        g_pMaSound_Control(g_MaSoundInstance, g_currentSoundId, 0, &controlByte, 0);
    if (g_pMaSound_Start)
        g_pMaSound_Start(g_MaSoundInstance, g_currentSoundId, a, 0);
    g_isPlayingFlag = 1;
}

void MMF_destroy_internal()
{
    if (g_pMaSound_Stop && g_currentSoundId != -1)
        g_pMaSound_Stop(g_MaSoundInstance, g_currentSoundId, 0);
    if (g_pMaSound_Unload && g_currentSoundId != -1)
        g_pMaSound_Unload(g_MaSoundInstance, g_currentSoundId, 0);
    if (g_pMaSound_Delete)
        g_pMaSound_Delete(g_MaSoundInstance);

    g_isPlayingFlag = 0;
    g_currentSoundId = -1;

    if (g_hMaSoundModule) {
        FreeLibrary(g_hMaSoundModule);
        g_hMaSoundModule = NULL;
        ResetMaSoundFunctionPointers();
    }

    if (g_hHeap && g_hHeap != GetProcessHeap()) {
        HeapDestroy(g_hHeap);
        g_hHeap = NULL;
    }
}

jboolean MMF_isPlaying_internal() { return g_isPlayingFlag ? JNI_TRUE : JNI_FALSE; }

void MMF_stop_internal()
{
    if (g_pMaSound_Stop && g_currentSoundId != -1)
        g_pMaSound_Stop(g_MaSoundInstance, g_currentSoundId, 0);
    g_isPlayingFlag = 0;
}

void MMF_pause_internal()
{
    if (g_pMaSound_Pause && g_currentSoundId != -1)
        g_pMaSound_Pause(g_MaSoundInstance, g_currentSoundId, 0);
}

void MMF_resume_internal()
{
    if (g_pMaSound_Restart && g_currentSoundId != -1)
        g_pMaSound_Restart(g_MaSoundInstance, g_currentSoundId, 0);
}

// ---------- JNI wrappers ----------
JNIEXPORT jint JNICALL Java_emulator_media_MMFPlayer_initMMFLibrary(JNIEnv *env, jclass cls, jstring jPath)
{
    (void)cls;
    const char *path = (*env)->GetStringUTFChars(env, jPath, NULL);
    int result = MMF_initMMFLibrary_internal(path);
    (*env)->ReleaseStringUTFChars(env, jPath, path);
    return result;
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_initPlayer(JNIEnv *env, jclass cls, jbyteArray data)
{
    (void)env; (void)cls; (void)data;
    MMF_initPlayer_internal();
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_play(JNIEnv *env, jclass cls, jint a, jint b)
{
    (void)env; (void)cls;
    MMF_play_internal(a, b);
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_destroy(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    MMF_destroy_internal();
}

JNIEXPORT jboolean JNICALL Java_emulator_media_MMFPlayer_isPlaying(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    return MMF_isPlaying_internal();
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_stop(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    MMF_stop_internal();
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_pause(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    MMF_pause_internal();
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_resume(JNIEnv *env, jclass cls)
{
    (void)env; (void)cls;
    MMF_resume_internal();
}

// ---------- DllMain ----------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    (void)hModule; (void)reserved;
    switch (reason)
    {
        case DLL_PROCESS_DETACH:
            if (g_hHeap && g_hHeap != GetProcessHeap())
                HeapDestroy(g_hHeap);
            break;
    }
    return TRUE;
}
