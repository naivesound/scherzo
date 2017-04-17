#include <jni.h>

#include "m.h"
#include "opensles.h"
#include "scherzo.h"

struct scherzo_context {
    struct opensles_engine E;
    scherzo_t *scherzo;
};

#define SF2DIR "/mnt/sdcard/sf2"

#define SCHERZO_BUFSZ 64
static short static_buf[SCHERZO_BUFSZ * 2];
static void scherzo_play_cb(SLAndroidSimpleBufferQueueItf q, void *p) {
    SLresult r;
    struct scherzo_context *context = (struct scherzo_context *)p;
    scherzo_write_stereo(context->scherzo, static_buf, SCHERZO_BUFSZ);
    r = (*q)->Enqueue(q, static_buf, sizeof(static_buf));
    if (r != SL_RESULT_SUCCESS) {
        OPENSLES_LOGD("error: Enqueue(): %d\n", r);
    }
}

JNIEXPORT jlong JNICALL Java_com_naivesound_scherzo_Scherzo_create(
    JNIEnv *env, jobject instance, jint sampleRate) {
    struct scherzo_context *context = malloc(sizeof(struct scherzo_context));
    if (context == NULL) {
        return 0;
    }
    if (opensles_open(&context->E) < 0) {
        free(context);
        return 0;
    }
    context->scherzo = scherzo_create(sampleRate, 64);
    if (context->scherzo == NULL) {
        return 0;
    }
    scherzo_load_instrument(context->scherzo, SF2DIR, 0);
    scherzo_midi(context->scherzo, MIDI_MSG_CC, MIDI_CC_VOL, 80);

    if (sampleRate == 48000) {
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
    opensles_close(&context->E);
    scherzo_destroy(context->scherzo);
    free(context);
}

JNIEXPORT void JNICALL Java_com_naivesound_scherzo_Scherzo_midi__JIII(
    JNIEnv *env, jobject instance, jlong ref, jint msg, jint a, jint b) {

    struct scherzo_context *context = (struct scherzo_context *)ref;
    if (context == NULL) {
        return;
    }
    scherzo_midi(context->scherzo, msg, a, b);
}
