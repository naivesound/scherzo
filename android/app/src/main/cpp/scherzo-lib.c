#include <jni.h>
#include <pthread.h>

#include "m.h"
#include "opensles.h"
#include "scherzo.h"

struct scherzo_context {
    pthread_mutex_t lock;
    struct opensles_engine E;
    scherzo_t *scherzo;
    int num_frames;
};

#define SF2DIR "/mnt/sdcard/sf2"

#define SCHERZO_BUFSZ (4096 * 8)
static void scherzo_play_cb(SLAndroidSimpleBufferQueueItf q, void *p) {
    SLresult r;
    static short static_buf[SCHERZO_BUFSZ];
    struct scherzo_context *context = (struct scherzo_context *)p;
    pthread_mutex_lock(&context->lock);
    scherzo_write_stereo(context->scherzo, static_buf, context->num_frames / 2);
    pthread_mutex_unlock(&context->lock);
    r = (*q)->Enqueue(q, static_buf, context->num_frames * sizeof(short));
    if (r != SL_RESULT_SUCCESS) {
        OPENSLES_LOGD("error: Enqueue(): %d", r);
    }
}

JNIEXPORT jlong JNICALL Java_com_naivesound_scherzo_Scherzo_create(
    JNIEnv *env, jobject instance, jint sample_rate, jint num_frames, jint max_poly) {
    struct scherzo_context *context = malloc(sizeof(struct scherzo_context));
    OPENSLES_LOGD("scherzo create %d %d %d", sample_rate, num_frames, max_poly);
    if (context == NULL) {
        return 0;
    }
    context->num_frames = num_frames;
    if (opensles_open(&context->E) < 0) {
        free(context);
        return 0;
    }
    pthread_mutex_init(&context->lock, NULL);
    context->scherzo = scherzo_create(sample_rate, max_poly);
    if (context->scherzo == NULL) {
        return 0;
    }
    scherzo_load_instrument(context->scherzo, SF2DIR, 0);
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
