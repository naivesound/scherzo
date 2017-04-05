#include <jni.h>

#include "opensles.h"
#include "scherzo.h"
#include "m.h"

struct scherzo_context {
    struct opensles_engine E;
    struct scherzo scherzo;
};

#define SCHERZO_BUFSZ 64
static short static_buf[SCHERZO_BUFSZ*2];
static void scherzo_play_cb(SLAndroidSimpleBufferQueueItf q, void *p) {
    SLresult r;
    struct scherzo_context *context = (struct scherzo_context*) p;
    scherzo_write_stereo(&context->scherzo, static_buf, SCHERZO_BUFSZ);
    r = (*q)->Enqueue(q, static_buf, sizeof(static_buf));
    if (r != SL_RESULT_SUCCESS) {
        OPENSLES_LOGD("error: Enqueue(): %d\n", r);
    }
}

JNIEXPORT jlong JNICALL
Java_com_naivesound_scherzo_Scherzo_create(JNIEnv *env, jobject instance, jint sampleRate) {
    struct scherzo_context *context = malloc(sizeof(struct scherzo_context));
    if (context == NULL) {
        return 0;
    }
    if (opensles_open(&context->E) < 0) {
        free(context);
        return 0;
    }
    if (scherzo_create(&context->scherzo, sampleRate, 64, metronome_acoustic) < 0) {
        free(context);
        return 0;
    }
    scherzo_load_instrument(&context->scherzo, 0);
    scherzo_set_gain(&context->scherzo, 127);

    if (sampleRate == 48000) {
        opensles_play(&context->E, SL_SAMPLINGRATE_48, 2, scherzo_play_cb, context);
    } else {
        opensles_play(&context->E, SL_SAMPLINGRATE_44_1, 2, scherzo_play_cb, context);
    }
    return (jlong) context;
}

JNIEXPORT void JNICALL
Java_com_naivesound_scherzo_Scherzo_destroy(JNIEnv *env, jobject instance, jlong ref) {
    struct scherzo_context *context = (struct scherzo_context *)ref;
    if (context == NULL) {
        return;
    }
    opensles_close(&context->E);
    scherzo_destroy(&context->scherzo);
    free(context);
}

JNIEXPORT void JNICALL
Java_com_naivesound_scherzo_Scherzo_tapBPM__J(JNIEnv *env, jobject instance, jlong ref) {
    struct scherzo_context *context = (struct scherzo_context *)ref;
    if (context == NULL) {
        return;
    }
    scherzo_set_bpm(&context->scherzo, 0);
}

JNIEXPORT void JNICALL
Java_com_naivesound_scherzo_Scherzo_noteOn__JIII(JNIEnv *env, jobject instance, jlong ref,
                                                 jint chan, jint note, jint velocity) {
    struct scherzo_context *context = (struct scherzo_context *)ref;
    if (context == NULL) {
        return;
    }
    scherzo_note_on(&context->scherzo, chan, note, velocity);
}

JNIEXPORT void JNICALL
Java_com_naivesound_scherzo_Scherzo_noteOff__JII(JNIEnv *env, jobject instance, jlong ref,
                                                 jint chan, jint note) {

    struct scherzo_context *context = (struct scherzo_context *)ref;
    if (context == NULL) {
        return;
    }
    scherzo_note_off(&context->scherzo, chan, note);
}

JNIEXPORT void JNICALL
Java_com_naivesound_scherzo_Scherzo_midi__JIII(JNIEnv *env, jobject instance, jlong ref, jint msg,
                                               jint a, jint b) {

    struct scherzo_context *context = (struct scherzo_context *)ref;
    if (context == NULL) {
        return;
    }
    scherzo_midi(&context->scherzo, msg, a, b);
}

JNIEXPORT void JNICALL
Java_com_naivesound_scherzo_Scherzo_looperCommand__JZ(JNIEnv *env, jobject instance, jlong ref,
                                                      jboolean primary) {

    struct scherzo_context *context = (struct scherzo_context *)ref;
    if (context == NULL) {
        return;
    }
    scherzo_looper(&context->scherzo, primary ? 0 : 1);
}