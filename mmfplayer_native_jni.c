
// mmfplayer_native_jni.c
// JNI wrapper for emulator.media.MMFPlayer using ma3smwemu.dll (Java 8 compatible)
#include <windows.h>
#include <jni.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define EXPORT __declspec(dllexport)

static HMODULE g_hMaSound = NULL;
static BYTE* EmuBuf;
static BYTE* EmuP;
static int g_instanceId = 1;
static int g_currentSound = -1;
static int g_isPlaying = 0;

static int (*MaSound_EmuInitialize)(DWORD, DWORD, BYTE*);
static int (*MaSound_Initialize)(int, BYTE*, int);
static int (*MaSound_DeviceControl)(int, int, int, int);
static int (*MaSound_Terminate)();
static int (*MaSound_Create)(int);
static int (*MaSound_Load)(int, BYTE*, DWORD, int, int, int);
static int (*MaSound_Control)(int, int, int, int*, int);
static int (*MaSound_Open)(int, int, int, int);
static int (*MaSound_Standby)(int, int, int);
static int (*MaSound_Start)(int, int, int, int);
static int (*MaSound_GetEmuInfo)(int);
static int (*MaSound_Stop)(int, int, int);
static int (*MaSound_Pause)(int, int, int);
static int (*MaSound_Restart)(int, int, int);
static int (*MaSound_Seek)(int, int, int, int, int);
static int (*MaSound_Close)(int, int, int);
static int (*MaSound_Unload)(int, int, int);
static int (*MaSound_Delete)(int);
static int (*MaSound_Terminate)();
static int (*MaSound_EmuTerminate)();
static int (*SetMidiMsg)(BYTE*, DWORD);

static int load_exports() {
	if(!((FARPROC)MaSound_EmuInitialize	= GetProcAddress(g_hMaSound, "MaSound_EmuInitialize")))	return FALSE;
	if(!((FARPROC)MaSound_Initialize	= GetProcAddress(g_hMaSound, "MaSound_Initialize")))		return FALSE;
	if(!((FARPROC)MaSound_DeviceControl	= GetProcAddress(g_hMaSound, "MaSound_DeviceControl")))	return FALSE;
	if(!((FARPROC)MaSound_Terminate		= GetProcAddress(g_hMaSound, "MaSound_Terminate")))		return FALSE;
	if(!((FARPROC)MaSound_Create		= GetProcAddress(g_hMaSound, "MaSound_Create")))			return FALSE;
	if(!((FARPROC)MaSound_Load			= GetProcAddress(g_hMaSound, "MaSound_Load")))			return FALSE;
	if(!((FARPROC)MaSound_Control		= GetProcAddress(g_hMaSound, "MaSound_Control")))			return FALSE;
	if(!((FARPROC)MaSound_Open			= GetProcAddress(g_hMaSound, "MaSound_Open")))			return FALSE;
	if(!((FARPROC)MaSound_Standby		= GetProcAddress(g_hMaSound, "MaSound_Standby")))			return FALSE;
	if(!((FARPROC)MaSound_Start			= GetProcAddress(g_hMaSound, "MaSound_Start")))			return FALSE;
	if(!((FARPROC)MaSound_GetEmuInfo	= GetProcAddress(g_hMaSound, "MaSound_GetEmuInfo")))		return FALSE;
	if(!((FARPROC)MaSound_Stop			= GetProcAddress(g_hMaSound, "MaSound_Stop")))			return FALSE;
	if(!((FARPROC)MaSound_Pause			= GetProcAddress(g_hMaSound, "MaSound_Pause")))			return FALSE;
	if(!((FARPROC)MaSound_Restart			= GetProcAddress(g_hMaSound, "MaSound_Restart")))			return FALSE;
	if(!((FARPROC)MaSound_Seek			= GetProcAddress(g_hMaSound, "MaSound_Seek")))			return FALSE;
	if(!((FARPROC)MaSound_Close			= GetProcAddress(g_hMaSound, "MaSound_Close")))			return FALSE;
	if(!((FARPROC)MaSound_Unload		= GetProcAddress(g_hMaSound, "MaSound_Unload")))			return FALSE;
	if(!((FARPROC)MaSound_Delete		= GetProcAddress(g_hMaSound, "MaSound_Delete")))			return FALSE;
	if(!((FARPROC)MaSound_Terminate		= GetProcAddress(g_hMaSound, "MaSound_Terminate")))		return FALSE;
	if(!((FARPROC)MaSound_EmuTerminate	= GetProcAddress(g_hMaSound, "MaSound_EmuTerminate")))	return FALSE;
	if(!((FARPROC)SetMidiMsg			= GetProcAddress(g_hMaSound, "SetMidiMsg")))				return FALSE;

    
    return 1;
}

JNIEXPORT jint JNICALL Java_emulator_media_MMFPlayer_initMMFLibrary(JNIEnv *env, jclass cls, jstring jpath) {
    const char *path = (*env)->GetStringUTFChars(env, jpath, NULL);
    if (!path) return -1;
    
    // Загружаем DLL по указанному пути
    g_hMaSound = LoadLibraryA(path);
    (*env)->ReleaseStringUTFChars(env, jpath, path);
    
    if (!g_hMaSound) return -2;
    
    if (!load_exports()) return -3;
	
	if (EmuBuf) {
		free(EmuBuf);
	}
	
	EmuBuf = malloc(1024);
	EmuP = EmuBuf;
	while(((DWORD)EmuP & 0xFF) != 0x81) EmuP++;
    
    int result = 0;
    if (!MaSound_Initialize) {
		return -5;
	}
	if(MaSound_EmuInitialize(48000, 2, EmuP)) {
		return -6;
	}
	if(MaSound_Initialize(0, EmuBuf, 0))	{
		return -7;
	}
	if(MaSound_DeviceControl(0x0D, 0, 0, 0)) {
		return -8;
	}
	if(MaSound_DeviceControl(0x05, 2, 0, 0)) {
		return 0;
	}
	if(MaSound_DeviceControl(0x06, 0, 0, 0)) {
		return 0;
	}
	if(MaSound_DeviceControl(0x08, 2, 0, 0)) {
		return 0;
	}
	if(MaSound_DeviceControl(0x09, 0, 0, 0)) {
		return 0;
	}
    
    if (MaSound_Create) {
        result = MaSound_Create(g_instanceId);
    }
    
    return result;
}

JNIEXPORT jint JNICALL Java_emulator_media_MMFPlayer_initPlayer(JNIEnv *env, jclass cls, jbyteArray arr) {
    if (!MaSound_Load) return -1;
    
    // Получаем ID из byte array (4 байта)
    jint len = (*env)->GetArrayLength(env, arr);
    jbyte* data = (*env)->GetByteArrayElements(env, arr, NULL);
    
    // Закрываем предыдущий звук если был
    if (g_currentSound != -1) {
        if (MaSound_Close) MaSound_Close(g_instanceId, g_currentSound, 0);
        if (MaSound_Unload) MaSound_Unload(g_instanceId, g_currentSound, 0);
    }
    
    // Загружаем новый звук
    int result = MaSound_Load(g_instanceId, data, len, 1, 0, 0);
    if (result < 0) return -2;
    
    g_currentSound = result;
    
    // Дополнительные операции как в оригинале
    if (MaSound_Open(g_instanceId, g_currentSound, 0, 0)) return -3;
    if (MaSound_Standby(g_instanceId, g_currentSound, 0)) return -4;
    //if (MaSound_Control) MaSound_Control(g_instanceId, g_currentSound, 5, 0, 0);
	int pitch = 0;
	MaSound_Control(g_instanceId, g_currentSound, 2, &pitch, 0);
	int tempo = 100;
	MaSound_Control(g_instanceId, g_currentSound, 1, &tempo, 0);
	
	return 0;
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_play(JNIEnv *env, jclass cls, jint loops, jint vol) {
    if (g_currentSound == -1) return;
    
    // Устанавливаем громкость (0-255 -> 0-100 scale)
    int volume = (int)((vol & 0xFF) * 100 / 255);
    if (MaSound_Control) MaSound_Control(g_instanceId, g_currentSound, 0, &volume, 0);
    
    // Воспроизводим
    if (MaSound_Start) MaSound_Start(g_instanceId, g_currentSound, loops, 0);
    g_isPlaying = 1;
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_stop(JNIEnv *env, jclass cls) {
    if (g_currentSound == -1) return;
    
    if (MaSound_Stop) MaSound_Stop(g_instanceId, g_currentSound, 0);
    g_isPlaying = 0;
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_pause(JNIEnv *env, jclass cls) {
    if (g_currentSound == -1) return;
    
    if (MaSound_Pause) MaSound_Pause(g_instanceId, g_currentSound, 0);
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_resume(JNIEnv *env, jclass cls) {
    if (g_currentSound == -1) return;
    
    if (MaSound_Restart) MaSound_Restart(g_instanceId, g_currentSound, 0);
}

JNIEXPORT void JNICALL Java_emulator_media_MMFPlayer_destroy(JNIEnv *env, jclass cls) {
    if (g_isPlaying && MaSound_Stop) {
        MaSound_Stop(g_instanceId, g_currentSound, 0);
    }
    
    if (MaSound_Delete) MaSound_Delete(g_instanceId);
    
    if (g_hMaSound) {
        FreeLibrary(g_hMaSound);
        g_hMaSound = NULL;
    }
    
    g_isPlaying = 0;
    g_currentSound = -1;
}

JNIEXPORT jboolean JNICALL Java_emulator_media_MMFPlayer_isPlaying(JNIEnv *env, jclass cls) {
    return g_isPlaying ? JNI_TRUE : JNI_FALSE;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
        case DLL_THREAD_DETACH:
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
