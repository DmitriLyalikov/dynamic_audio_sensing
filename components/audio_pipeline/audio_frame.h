#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */
/*                               Constants                                    */
/* -------------------------------------------------------------------------- */

#define AUDIO_FRAME_MAGIC 0x41554430  /* "AUD0" */

/* -------------------------------------------------------------------------- */
/*                              Scene Labels                                  */
/* -------------------------------------------------------------------------- */

typedef enum {
    SCENE_QUIET = 0,
    SCENE_SPEECH,
    SCENE_NOISE
} audio_scene_t;

/* -------------------------------------------------------------------------- */
/*                          Audio Frame Structure                              */
/* -------------------------------------------------------------------------- */
/*
 * This structure represents one processed audio frame and associated metadata.
 * It is passed between tasks (DSP → transport) via FreeRTOS queues.
 *
 * Ownership rules:
 *  - Producer allocates audio_frame_t
 *  - Consumer is responsible for freeing:
 *      - samples_in
 *      - samples_out
 *      - audio_frame_t itself
 */

typedef struct {
    uint32_t magic;           /* Sanity check value */
    uint32_t sample_count;    /* Number of samples per channel */

    /* Extracted features */
    float rms;
    float centroid;

    /* Classification result */
    audio_scene_t scene;
    float gain;

    /* Audio payload (16-bit PCM) */
    int16_t *samples_in;      /* Raw microphone input */
    int16_t *samples_out;     /* Gain-adjusted output */
} audio_frame_t;

#ifdef __cplusplus
}
#endif
