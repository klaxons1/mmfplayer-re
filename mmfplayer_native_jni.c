#include <windows.h>
#include <jni.h>

// Глобальные переменные для хранения состояния и функций библиотеки
HMODULE hModule = NULL;
int g_playerId = -1;
int g_isPlaying = 0;

// Указатели на функции из звуковой библиотеки
typedef int (__cdecl *MaSound_Initialize_t)();
typedef int (__cdecl *MaSound_Create_t)(int);
typedef int (__cdecl *MaSound_Load_t)(int, int, const char*, int, int, int);
typedef int (__cdecl *MaSound_Open_t)(int, int, int, int);
typedef int (__cdecl *MaSound_Standby_t)(int, int, int);
typedef int (__cdecl *MaSound_Start_t)(int, int, int, int);
typedef int (__cdecl *MaSound_Stop_t)(int, int, int);
typedef int (__cdecl *MaSound_Pause_t)(int, int, int);
typedef int (__cdecl *MaSound_Resume_t)(int, int, int);
typedef int (__cdecl *MaSound_Close_t)(int, int, int);
typedef int (__cdecl *MaSound_Unload_t)(int, int, int);
typedef int (__cdecl *MaSound_Delete_t)(int);

MaSound_Initialize_t MaSound_Initialize = NULL;
MaSound_Create_t MaSound_Create = NULL;
MaSound_Load_t MaSound_Load = NULL;
MaSound_Open_t MaSound_Open = NULL;
MaSound_Standby_t MaSound_Standby = NULL;
MaSound_Start_t MaSound_Start = NULL;
MaSound_Stop_t MaSound_Stop = NULL;
MaSound_Pause_t MaSound_Pause = NULL;
MaSound_Resume_t MaSound_Resume = NULL;
MaSound_Close_t MaSound_Close = NULL;
MaSound_Unload_t MaSound_Unload = NULL;
MaSound_Delete_t MaSound_Delete = NULL;

// Инициализация библиотеки
JNIEXPORT jint JNICALL Java_emulator_media_MMFPlayer_initMMFLibrary
  (JNIEnv *env, jobject obj, jstring libPath) {

    const char* nativeLibPath = (*env)->GetStringUTFChars(env, libPath, 0);
    hModule = LoadLibraryA(nativeLibPath);
    (*env)->ReleaseStringUTFChars(env, libPath, nativeLibPath);

    if (!hModule) return -1;

    // Загрузка функций из библиотеки
    MaSound_Initialize = (MaSound_Initialize_t)GetProcAddress(hModule, "MaSound_Initialize");
    MaSound_Create = (MaSound_Create_t)GetProcAddress(hModule, "MaSound_Create");
    MaSound_Load = (MaSound_Load_t)GetProcAddress(hModule, "MaSound_Load");
    MaSound_Open = (MaSound_Open_t)GetProcAddress(hModule, "MaSound_Open");
    MaSound_Standby = (MaSound_Standby_t)GetProcAddress(hModule, "MaSound_Standby");
    MaSound_Start = (MaSound_Start_t)GetProcAddress(hModule, "MaSound_Start");
    MaSound_Stop = (MaSound_Stop_t)GetProcAddress(hModule, "MaSound_Stop");
    MaSound_Pause = (MaSound_Pause_t)GetProcAddress(hModule, "MaSound_Pause");
    MaSound_Resume = (MaSound_Resume_t)GetProcAddress(hModule, "MaSound_Restart");
    MaSound_Close = (MaSound_Close_t)GetProcAddress(hModule, "MaSound_Close");
    MaSound_Unload = (MaSound_Unload_t)GetProcAddress(hModule, "MaSound_Unload");
    MaSound_Delete = (MaSound_Delete_t)GetProcAddress(hModule, "MaSound_Delete");

    if (!MaSound_Initialize || !MaSound_Create) return -1;

    int result = MaSound_Initialize();
    if (result < 0) return -1;

    result = MaSound_Create(1);
    return (result >= 0) ? 1 : -1;
}

// Инициализация плеера
JNIEXPORT jint JNICALL Java_emulator_media_MMFPlayer_initPlayer
  (JNIEnv *env, jobject obj, jstring filePath) {

    if (g_playerId != -1) {
        MaSound_Close(1, g_playerId, 0);
        MaSound_Unload(1, g_playerId, 0);
    }

    const char* nativeFilePath = (*env)->GetStringUTFChars(env, filePath, 0);
    int result = MaSound_Load(1, 0, nativeFilePath, 1, 0, 0);
    (*env)->ReleaseStringUTFChars(env, filePath, nativeFilePath);

    if (result < 0) return -1;

    g_playerId = result;
    MaSound_Open(1, g_playerId, 0, 0);
    MaSound_Standby(1, g_playerId, 0);
    return 1;
}

// Воспроизведение звука
JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_play
  (JNIEnv *env, jobject obj, jint loops, jboolean background) {

    if (g_isPlaying) Java_emulator_media_MMFPlayer_stop(env, obj);

    char volume = 25 * (background ? 1 : 0);
    MaSound_Start(1, g_playerId, loops, 0);
    g_isPlaying = 1;
}

// Проверка состояния воспроизведения
JNIEXPORT jboolean JNICALL Java_emulator_media_MMFPlayer_isPlaying
  (JNIEnv *env, jobject obj) {
    return g_isPlaying;
}

// Остановка воспроизведения
JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_stop
  (JNIEnv *env, jobject obj) {

    MaSound_Stop(1, g_playerId, 0);
    g_isPlaying = 0;
}

// Приостановка воспроизведения
JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_pause
  (JNIEnv *env, jobject obj) {
    MaSound_Pause(1, g_playerId, 0);
}

// Возобновление воспроизведения
JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_resume
  (JNIEnv *env, jobject obj) {
    MaSound_Resume(1, g_playerId, 0);
}

// Освобождение ресурсов
JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_destroy
  (JNIEnv *env, jobject obj) {

    if (g_playerId != -1) {
        MaSound_Close(1, g_playerId, 0);
        MaSound_Unload(1, g_playerId, 0);
        MaSound_Delete(1);
        g_playerId = -1;
    }
    g_isPlaying = 0;

    if (hModule) {
        FreeLibrary(hModule);
        hModule = NULL;
    }
}