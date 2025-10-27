// mmfplayer_rewrite.c
// Реконструкция логики mmfplayer.dll.c в читабельном, компилируемом виде.
// Компилировать как DLL. Экспортируемые функции имеют те же имена и соглашения,
// что и в дизассемблерном варианте: __stdcall Java_emulator_media_MMFPlayer_*

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
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

// ---------- Глобальные переменные (модули/указатели) ----------
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

// идентификаторы (как в дизассемблере — "handle" / индексы)
static int g_MaSoundInstance  = 1;    // dword_10007030
static int g_currentSoundId   = -1;   // dword_10007034
static int g_isPlayingFlag    = 0;    // dword_10009B68

// Heap used by replacement allocators (упрощённый)
static HANDLE g_hHeap = NULL;

// ---------- Вспомогательные функции ----------
static void EnsureHeap()
{
    if (!g_hHeap) {
        // Создаём приватный heap для модуля (как в оригинале).
        // initial size = 4096, max = 0 (growable)
        g_hHeap = HeapCreate(0, 4096, 0);
        if (!g_hHeap) {
            // fallback to process heap
            g_hHeap = GetProcessHeap();
        }
    }
}

static void* mmf_malloc(size_t size)
{
    EnsureHeap();
    if (size == 0) size = 1;
    // align to 16
    size_t aligned = (size + 15) & ~((size_t)15);
    return HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, aligned);
}

static void mmf_free(void *ptr)
{
    if (!ptr) return;
    if (!g_hHeap) {
        g_hHeap = GetProcessHeap();
    }
    HeapFree(g_hHeap, 0, ptr);
}

// ---------- Загрузка/инициализация MaSound ----------
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

    #define LOAD_FN(name) (FARPROC) GetProcAddress(g_hMaSoundModule, name)

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

// ---------- Exported API (реализация функций из дизассемблера) ----------
// Примечание: оригинальные сигнатуры взяты как __stdcall (stdcall).
// Третий параметр в initMMFLibrary используется как const char * – путь к DLL.
// Если реализация вызывающего кода даёт другой формат (JNI), нужно адаптировать сюда.

__declspec(dllexport) int __stdcall Java_emulator_media_MMFPlayer_initMMFLibrary(void *a1, int a2, const char *libPath)
{
    // a1/a2 – не используются в этой реализации; libPath — путь к MaSound DLL
    int createResult = -1;

    if (!libPath || !libPath[0]) {
        // нельзя загрузить без пути
        return -1;
    }

    // Попробуем загрузить библиотеку
    g_hMaSoundModule = LoadLibraryA(libPath);
    if (!g_hMaSoundModule) {
        ResetMaSoundFunctionPointers();
        return -1;
    }

    LoadMaSoundExports();

    // Вызываем MaSound_Initialize (если есть)
    if (g_pMaSound_Initialize) {
        createResult = g_pMaSound_Initialize();
    }

    // Вызываем MaSound_Create, передавая значение g_MaSoundInstance (как в дизассемблере).
    if (g_pMaSound_Create) {
        createResult = g_pMaSound_Create(g_MaSoundInstance);
    }

    if (createResult < 0) {
        // неуспешно
        return -1;
    }

    // Убедимся, что heap создан
    EnsureHeap();

    return 1;
}

__declspec(dllexport) int __stdcall Java_emulator_media_MMFPlayer_initPlayer(void *a1, int a2, int soundPathOrIndex)
{
    // В дизассемблере sub_10001530() и sub_10001550() брали параметры из Java env.
    // Здесь считаем, что soundPathOrIndex — индекс/идентификатор ресурса (int),
    // и второй вызов sub_10001550 возвращает target param v6 (см. дизассемблер).
    //
    // Для совместимости: если надо загрузить звук по индексу — передаём этот индекс
    // в MaSound_Load. Возвращаем результат MaSound_Load или -1 при ошибке.

    int v4 = soundPathOrIndex;
    int v6 = 0; // placeholder value (в оригинале брался через vtable)
    int loadRes = -1;

    // Если ранее был открыт другой звук — сначала закрываем / выгружаем его
    if (g_currentSoundId != -1) {
        if (g_pMaSound_Close) {
            g_pMaSound_Close(g_MaSoundInstance, g_currentSoundId, 0);
        }
        if (g_pMaSound_Unload) {
            g_pMaSound_Unload(g_MaSoundInstance, g_currentSoundId, 0);
        }
        g_currentSoundId = -1;
    }

    if (g_pMaSound_Load) {
        // Параметры: (instance, v6, v4, 1, 0, 0) — как в дизассемблере
        loadRes = g_pMaSound_Load(g_MaSoundInstance, v6, v4, 1, 0, 0);
    }

    if (loadRes >= 0) {
        g_currentSoundId = loadRes;
        // Open, Standby, Control(5,...) — как в дизассемблере
        if (g_pMaSound_Open) {
            g_pMaSound_Open(g_MaSoundInstance, g_currentSoundId, 0, 0);
        }
        if (g_pMaSound_Standby) {
            g_pMaSound_Standby(g_MaSoundInstance, g_currentSoundId, 0);
        }
        if (g_pMaSound_Control) {
            // MaSound_Control(instance, soundId, 5, 0, 0)
            return g_pMaSound_Control(g_MaSoundInstance, g_currentSoundId, 5, NULL, 0);
        }
        return loadRes;
    }

    return loadRes;
}

__declspec(dllexport) void __stdcall Java_emulator_media_MMFPlayer_play(int a1, int a2, int a3, char a4)
{
    // a3 — параметр, который в дизассемблере передавался в MaSound_Start
    // a4 — скорость/volume (?) — в дизассемблере умножался на 25 и передавался через MaSound_Control
    int result = -1;

    if (g_isPlayingFlag) {
        // Если уже играет — останавливаем
        Java_emulator_media_MMFPlayer_stop(a1, a2);
    }

    char controlByte = (char)(25 * (int)a4);
    if (g_pMaSound_Control && g_currentSoundId != -1) {
        result = g_pMaSound_Control(g_MaSoundInstance, g_currentSoundId, 0, &controlByte, 0);
        (void)result;
    }

    if (g_pMaSound_Start && g_currentSoundId != -1) {
        result = g_pMaSound_Start(g_MaSoundInstance, g_currentSoundId, a3, 0);
        (void)result;
    }

    g_isPlayingFlag = 1;
}

__declspec(dllexport) int __stdcall Java_emulator_media_MMFPlayer_destroy(int a1, int a2)
{
    int result = 0;

    if (g_pMaSound_Stop && g_currentSoundId != -1) {
        result = g_pMaSound_Stop(g_MaSoundInstance, g_currentSoundId, 0);
    }
    if (g_pMaSound_Unload && g_currentSoundId != -1) {
        result = g_pMaSound_Unload(g_MaSoundInstance, g_currentSoundId, 0);
    }
    if (g_pMaSound_Delete) {
        result = g_pMaSound_Delete(g_MaSoundInstance);
    }

    g_isPlayingFlag = 0;
    g_currentSoundId = -1;

    // Unload module
    if (g_hMaSoundModule) {
        FreeLibrary(g_hMaSoundModule);
        g_hMaSoundModule = NULL;
        ResetMaSoundFunctionPointers();
    }

    // Destroy heap if it was created by us
    if (g_hHeap) {
        // If it is not process heap, destroy
        if (g_hHeap != GetProcessHeap()) {
            HeapDestroy(g_hHeap);
        }
        g_hHeap = NULL;
    }

    return result;
}

__declspec(dllexport) char __stdcall Java_emulator_media_MMFPlayer_isPlaying(int a1, int a2)
{
    (void)a1; (void)a2;
    return (char)(g_isPlayingFlag ? 1 : 0);
}

__declspec(dllexport) int __stdcall Java_emulator_media_MMFPlayer_stop(int a1, int a2)
{
    (void)a1; (void)a2;
    int result = 0;
    if (g_pMaSound_Stop && g_currentSoundId != -1) {
        result = g_pMaSound_Stop(g_MaSoundInstance, g_currentSoundId, 0);
    }
    if (g_pMaSound_Control && g_currentSoundId != -1) {
        // В дизассемблере дополнительно вызывался MaSound_Control(..., 0,0,0) — вызвать при необходимости
        g_pMaSound_Control(g_MaSoundInstance, g_currentSoundId, 0, NULL, 0);
    }
    g_isPlayingFlag = 0;
    return result;
}

__declspec(dllexport) int __stdcall Java_emulator_media_MMFPlayer_pause(int a1, int a2)
{
    (void)a1; (void)a2;
    if (g_pMaSound_Pause && g_currentSoundId != -1) {
        return g_pMaSound_Pause(g_MaSoundInstance, g_currentSoundId, 0);
    }
    return 0;
}

__declspec(dllexport) int __stdcall Java_emulator_media_MMFPlayer_resume(int a1, int a2)
{
    (void)a1; (void)a2;
    if (g_pMaSound_Restart && g_currentSoundId != -1) {
        return g_pMaSound_Restart(g_MaSoundInstance, g_currentSoundId, 0);
    }
    return 0;
}

// ---------- DLL entry point (необязательно) ----------
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    (void)lpReserved;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        // ничего не делаем — lazy init
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        // очистка
        if (g_hMaSoundModule) {
            FreeLibrary(g_hMaSoundModule);
            g_hMaSoundModule = NULL;
        }
        if (g_hHeap && g_hHeap != GetProcessHeap()) {
            HeapDestroy(g_hHeap);
            g_hHeap = NULL;
        }
        break;
    }
    return TRUE;
}
