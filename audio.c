#include "common.h"
#include "audio.h"
#include <mpg123.h>
#include <alsa/asoundlib.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "sound_mp3.h"
static pthread_t audio_thread;

static int play = 0;

static void *audio_thread_func(void *arg)
{
    const char *filename = arg;

    mpg123_handle *mh = NULL;
    snd_pcm_t *pcm = NULL;
    unsigned char *buffer = NULL;

    int err;
    size_t done;
    long rate;
    int channels, encoding;

    mpg123_init();
    mh = mpg123_new(NULL, &err);

    if (mpg123_open(mh, filename) != MPG123_OK) {
        fprintf(stderr, "mpg123: failed to open %s\n", filename);
        goto cleanup;
    }

    mpg123_getformat(mh, &rate, &channels, &encoding);
    mpg123_format_none(mh);
    mpg123_format(mh, rate, channels, MPG123_ENC_SIGNED_16);

    if (snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0) {
        fprintf(stderr, "ALSA: failed to open device\n");
        goto cleanup;
    }

    snd_pcm_set_params(
        pcm,
        SND_PCM_FORMAT_S16_LE,
        SND_PCM_ACCESS_RW_INTERLEAVED,
        channels,
        rate,
        1,
        500000
    );

    buffer = malloc(4096);

    while (play) {
        if (mpg123_read(mh, buffer, 4096, &done) == MPG123_OK) {
            snd_pcm_writei(pcm, buffer, done / (2 * channels));
        } else {
            mpg123_seek(mh, 0, SEEK_SET); // loop
        }
    }

cleanup:
    if (buffer) free(buffer);
    if (pcm) snd_pcm_close(pcm);
    if (mh) {
        mpg123_close(mh);
        mpg123_delete(mh);
        mpg123_exit();
    }

    return NULL;
}

void audio_start(const char *filename)
{
    if (play) return;
    play = 1;
    pthread_create(&audio_thread, NULL, audio_thread_func, (void *)filename);
}

void audio_stop(void)
{
    if (!play) return;
    play = 0;
    pthread_join(audio_thread, NULL);
}
