#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_audio.h>
#include <SDL2/SDL_timer.h>

struct metronome_state {
    float *clip;
    // number of samples after the beat of the current position, may be non-integer
    double beat_offset;
    // uint64_t rand_state;
    double period;
};

const size_t CLIP_LEN = 4800;

const double TWO_PI = M_PI * 2.0;
const int SAMPLE_RATE = 48000;

// "angular velocity" of the tone in radians/sample
// const double TONE_ANGLE = 440.0 / SAMPLE_RATE * TWO_PI;
// frequency in cycles/sample

/*
uint64_t xorshift64star(uint64_t *x) {
    *x ^= *x >> 12;
    *x ^= *x << 25;
    *x ^= *x >> 27;
    return *x * 0x2545f4914f6cdd1dul;
}
*/

// const double TWO53 = (double) (1ul << 53ul);

/*
double rand_double(uint64_t *state) {
    uint64_t n = xorshift64star(state) & ((1ul << 53ul) - 1ul);
    return ((double) n) / TWO53;
}
*/

void audio_callback(void* userdata, uint8_t *bytes, int bytelen) {
    struct metronome_state *state = (struct metronome_state *)userdata;
    int count = bytelen / 4;
    float *samples = (float *)bytes;
    float *clip = state->clip;
    double period = state->period;
    // uint64_t *rand_state = &state->rand_state;

    double beat_offset = state->beat_offset;
    for (int i = 0; i < count; i++) {
        float sample = 0.0f;
        if (beat_offset < CLIP_LEN) {
            size_t index = ((uint32_t) beat_offset) % CLIP_LEN;
            sample = clip[index];
            // sample = (float) (sin(TONE_ANGLE * beat_offset));
            // double t = TONE_FREQ * beat_offset;
            // double t = TONE_FREQ * beat_offset + 0.5;
            // t = t - floor(t);
            // sample = t < 0.5 ? 1.0f : -1.0f;
            // sample = t * 2.0 - 1.0;
            // sample = fabs(t - 0.5) * 2.0;
            // sample = rand_double(rand_state) * 2.0 - 1.0;
            // sample = copysignf(powf(fabsf(sample), 0.1f), sample);
            // printf("%f\n", sample);
        }
        samples[i] = sample;

        beat_offset += 1.0;
        if (beat_offset > period) {
            beat_offset -= period;
        }
    }
    state->beat_offset = beat_offset;
}

float *read_audio() {
    size_t bytelen = CLIP_LEN * sizeof(float);
    uint8_t *buf = calloc(bytelen, 1);
    FILE *f = fopen("click-48k-f32.bin", "rb");
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
    if (argc >= 2) {
        bpm = atof(argv[1]);
        if (!(bpm >= 20.0 && bpm <= 400.0)) {
            printf("bpm must be between 20 and 400\n");
            return 1;
        }
        printf("using bpm %.2f\n", bpm);
    } else {
        printf("using default bpm %.2f\n", bpm);
    SDL_Init(SDL_INIT_AUDIO);
    }
    double period = 60.0 / bpm * SAMPLE_RATE;

    float *clip = read_audio();
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
    state->clip = clip;
    state->beat_offset = 0.0;
    state->period = period;
    // state->rand_state = 1ul;
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
    for (;;) {
        SDL_Delay(1000);
    }
    SDL_CloseAudioDevice(dev);
    free(state->clip);
    free(state);

    return 0;
}
