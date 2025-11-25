// Créer un fichier irix_audio.h - Stub pour l'audio

#ifndef IRIX_AUDIO_H
#define IRIX_AUDIO_H

// Option 1 : Stubs (pour compiler sans audio)
#define ALsetparams(port, params, size) 0
#define ALopenport(name, mode, config) NULL
#define ALwritesamps(port, buf, count) 0
#define ALcloseport(port) 0

// Option 2 : À implémenter avec OpenAL ou PortAudio
// TODO: Implémenter avec une vraie bibliothèque audio multiplateforme

typedef void* ALport;
typedef void* ALconfig;

#define AL_INPUT 0
#define AL_OUTPUT 1

// Stubs basiques
static inline ALconfig ALnewconfig(void) { return NULL; }
static inline int ALsetchannels(ALconfig config, int channels) { return 0; }
static inline int ALsetwidth(ALconfig config, int width) { return 0; }
static inline int ALsetqueuesize(ALconfig config, int size) { return 0; }

#endif // IRIX_AUDIO_H