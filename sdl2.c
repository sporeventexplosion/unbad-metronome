#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_timer.h>

struct metronome_state {
    float *clip_l;
    float *clip_h;
    // number of samples after the beat of the current position, may be non-integer
    double beat_offset;
    double period;
    int nbeats;
    int beat;
};

const size_t CLIP_LEN = 4800;

const double TWO_PI = M_PI * 2.0;
const int SAMPLE_RATE = 48000;

void audio_callback(void* userdata, uint8_t *bytes, int bytelen) {
    struct metronome_state *state = (struct metronome_state *)userdata;
    int count = bytelen / 4;
    float *samples = (float *)bytes;
    double period = state->period;
    int nbeats = state->nbeats;
    int beat = state->beat;

    float *clip = (nbeats > 1 && beat == 0)
        ? state->clip_h
        : state->clip_l;

    double beat_offset = state->beat_offset;
    for (int i = 0; i < count; i++) {
        float sample = 0.0f;
        if (beat_offset < CLIP_LEN) {
            size_t index = ((uint32_t) beat_offset) % CLIP_LEN;
            sample = clip[index];
        }
        samples[i] = sample;

        beat_offset += 1.0;
        if (beat_offset > period) {
            beat_offset -= period;
            beat = (beat + 1) % nbeats;

            clip = (nbeats > 1 && beat == 0)
                ? state->clip_h
                : state->clip_l;
        }
    }
    state->beat_offset = beat_offset;
    state->beat = beat;
}


float *read_audio_file(char *name) {
    size_t bytelen = CLIP_LEN * sizeof(float);
    uint8_t *buf = calloc(bytelen, 1);
    FILE *f = fopen(name, "rb");
    assert(f != NULL);
    size_t nread = 0;
    while (nread < bytelen) {
        size_t n = fread(buf + nread, 1, bytelen - nread, f);
        assert(n != 0);
        nread += n;
    }
    return (float *) buf;
}

int main(int argc, char **argv) {

    double bpm = 120.0;
    int nbeats = 1;
    if (argc >= 2) {
        bpm = atof(argv[1]);
        if (!(bpm >= 20.0 && bpm <= 400.0)) {
            printf("bpm must be between 20 and 400 inclusive\n");
            return 1;
        }
        printf("using bpm %.2f\n", bpm);
    } else {
        printf("using default bpm %.2f\n", bpm);
    }
    if (argc >= 3) {
        nbeats = atoi(argv[2]);
        if (!(nbeats >= 1 && nbeats <= 16)) {
            printf("nbeats must be between 1 and 16 inclusive\n");
            return 1;
        }
        printf("using %d beats per bar\n", nbeats);
    }
    double period = 60.0 / bpm * SAMPLE_RATE;

    float *clip_l = read_audio_file("click-48k-f32.bin");
    float *clip_h = read_audio_file("click-48k-f32-h.bin");
    SDL_Init(SDL_INIT_AUDIO);
    SDL_AudioSpec want, have;
    SDL_AudioDeviceID dev;

    SDL_memset(&want, 0, sizeof(want));
    want.freq = SAMPLE_RATE;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 2048;
    want.callback = audio_callback;

    struct metronome_state *state = malloc(sizeof(struct metronome_state));
    state->clip_l = clip_l;
    state->clip_h = clip_h;
    state->beat_offset = 0.0;
    state->period = period;
    state->nbeats = nbeats;
    state->beat = 0;
    want.userdata = state;

    dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    if (dev == 0) {
        printf("Failed to open audio: %s\n", SDL_GetError());
        return 1;
    }
    if (have.format != want.format || have.freq != want.freq) {
        printf("Failed to get requested format or frequency\n");
        return 1;
    }

    SDL_PauseAudioDevice(dev, 0);

    // block on stdin
    char scratch;
    while (fread(&scratch, 1, 1, stdin) == 1) {
      if (scratch == 'q' || scratch == 'Q') break;
    }

    SDL_CloseAudioDevice(dev);
    free(state->clip_l);
    free(state->clip_h);
    free(state);

    return 0;
}
