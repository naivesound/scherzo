#include <jni.h>
#include <pthread.h>
#include <unistd.h>

#include <android/looper.h>

#include <scherzo.h>

#include "opensles.h"

#define MIN_CALLBACK_INTERVAL 100

struct scherzo_context {
    pthread_mutex_t lock;
    struct opensles_engine E;
    ALooper *looper;
    int pipe[2];
    JNIEnv *env;
    jobject cb;
    jmethodID cb_method;

    scherzo_t *scherzo;
    int sample_rate;
    int num_frames;

    int events;
    int notes_buf[129];
    int debounce;
};

#define SF2DIR "/mnt/sdcard/sf2"

#define SCHERZO_BUFSZ (4096 * 8)
static void scherzo_play_cb(SLAndroidSimpleBufferQueueItf q, void *p) {
    SLresult r;
    int cb_events = 0;
    static short static_buf[SCHERZO_BUFSZ];
    struct scherzo_context *context = (struct scherzo_context *)p;
    pthread_mutex_lock(&context->lock);
    int events = scherzo_write_stereo(context->scherzo, static_buf,
                                      context->num_frames / 2);
    context->events |= events;
    for (int i = 0; i < 128; i++) {
        int vol = scherzo_get_note(context->scherzo, i);
        if (vol > 0) {
            context->notes_buf[i] = vol;
        }
    }
    if (context->debounce++ >= context->sample_rate * MIN_CALLBACK_INTERVAL /
                                   context->num_frames / 1000) {
        cb_events = context->events;
        context->debounce = 0;
        context->events = 0;
        memset(&context->notes_buf, 0, sizeof(context->notes_buf));
    }
    pthread_mutex_unlock(&context->lock);
    r = (*q)->Enqueue(q, static_buf, context->num_frames * sizeof(short));
    if (r != SL_RESULT_SUCCESS) {
        OPENSLES_LOGD("error: Enqueue(): %d", r);
    }

    if (cb_events) {
        write(context->pipe[1], &cb_events, sizeof(cb_events));
    }
}

static int scherzo_pipe_cb(int fd, int fd_events, void *p) {
    int events = 0;
    struct scherzo_context *context = (struct scherzo_context *)p;
    read(fd, &events, sizeof(events));
    if (context->cb != NULL) {
        (*(context->env))
            ->CallVoidMethod(context->env, context->cb, context->cb_method,
                             events);
    }
    return 1;
}

JNIEXPORT jlong JNICALL Java_com_naivesound_scherzo_Scherzo_create(
    JNIEnv *env, jobject instance, jint sample_rate, jint num_frames,
    jint max_poly, jobject cb) {
    struct scherzo_context *context = malloc(sizeof(struct scherzo_context));
    OPENSLES_LOGD("scherzo create %d %d %d", sample_rate, num_frames, max_poly);
    if (context == NULL) {
        return 0;
    }

    context->env = env;
    if (cb != NULL) {
        context->cb = (*env)->NewGlobalRef(env, cb);
        jclass cls = (*env)->GetObjectClass(env, cb);
        context->cb_method = (*env)->GetMethodID(env, cls, "invoke", "(I)V");
        (*env)->DeleteLocalRef(env, cls);

        OPENSLES_LOGD("found method: %p\n", context->cb_method);
    } else {
        context->cb = NULL;
    }

    context->sample_rate = sample_rate;
    context->num_frames = num_frames;
    context->events = 0;
    memset(&context->notes_buf, 0, sizeof(context->notes_buf));

    if (opensles_open(&context->E) < 0) {
        free(context);
        return 0;
    }

    if (pipe(context->pipe) < 0) {
        opensles_close(&context->E);
        free(context);
        return 0;
    }
    context->looper = ALooper_forThread();
    ALooper_acquire(context->looper);
    ALooper_addFd(context->looper, context->pipe[0], 0, ALOOPER_EVENT_INPUT,
                  scherzo_pipe_cb, context);

    pthread_mutex_init(&context->lock, NULL);
    context->scherzo = scherzo_create(sample_rate, max_poly);
    if (context->scherzo == NULL) {
        return 0;
    }
    scherzo_set_instrument_path(context->scherzo, 0, SF2DIR "/font.sf2");
    scherzo_load_instrument(context->scherzo, 0);
    scherzo_midi(context->scherzo, MIDI_MSG_CC, MIDI_CC_VOL, 80);

    if (sample_rate == 48000) {
        opensles_play(&context->E, SL_SAMPLINGRATE_48, 2, scherzo_play_cb,
                      context);
    } else {
        opensles_play(&context->E, SL_SAMPLINGRATE_44_1, 2, scherzo_play_cb,
                      context);
    }
    return (jlong)context;
}

JNIEXPORT void JNICALL Java_com_naivesound_scherzo_Scherzo_destroy(
    JNIEnv *env, jobject instance, jlong ref) {
    struct scherzo_context *context = (struct scherzo_context *)ref;
    if (context == NULL) {
        return;
    }
    pthread_mutex_lock(&context->lock);
    if (context->cb != NULL) {
        (*env)->DeleteGlobalRef(env, context->cb);
    }
    opensles_close(&context->E);
    scherzo_destroy(context->scherzo);
    pthread_mutex_unlock(&context->lock);
    free(context);
}

JNIEXPORT void JNICALL Java_com_naivesound_scherzo_Scherzo_midi__JIII(
    JNIEnv *env, jobject instance, jlong ref, jint msg, jint a, jint b) {
    struct scherzo_context *context = (struct scherzo_context *)ref;
    if (context == NULL) {
        return;
    }
    OPENSLES_LOGD("midi: %02x %02x %02x", msg & 0xff, a & 0xff, b & 0xff);
    pthread_mutex_lock(&context->lock);
    scherzo_midi(context->scherzo, msg, a, b);
    pthread_mutex_unlock(&context->lock);
}
