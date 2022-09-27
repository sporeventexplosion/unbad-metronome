#include <stdio.h>
#include <stdlib.h>

#include <pulse/simple.h>
#include <pulse/error.h>

int main(int argc, char **argv) {
    pa_simple *s;
    pa_sample_spec ss;
    ss.format = PA_SAMPLE_S16NE;
    ss.channels = 1;
    ss.rate = 48000;
    s = pa_simple_new(NULL,
            "Foo Bar",
            PA_STREAM_PLAYBACK,
            NULL,
            "Music",
            &ss,
            NULL,
            NULL,
            NULL);
    if (s == NULL) {
        printf("Error connecting to PulseAudio\n");
        return 1;
    }
    int err;
    pa_usec_t latency = pa_simple_get_latency(s, &err);
    if (err) {
        printf("Error getting latency: %s\n", pa_strerror(err));
        return 1;
    }
    printf("Latency: %lu us", latency);
    return 0;
}
