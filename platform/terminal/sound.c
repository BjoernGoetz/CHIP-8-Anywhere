/* Compiles miniaudio's implementation into this file. */
#define MINIAUDIO_IMPLEMENTATION

#include "sound.h"
#include "miniaudio.h"

#define SAMPLE_RATE 44100
#define TONE_HZ 441
#define SAMPLES_PER_HALF_WAVE (SAMPLE_RATE / (2 * TONE_HZ))
#define TONE_AMPLITUDE 3000

static ma_device device;
static volatile sig_atomic_t tone_on;
static uint32_t phase;

static ma_context dump_context;
static FILE *dump_file;

static void fill_samples(ma_device *d, void *output, const void *input,
                         ma_uint32 frames) {
  (void)d;
  (void)input;

  int16_t *samples = output;
  for (ma_uint32 i = 0; i < frames; i++) {
    if (!tone_on) {
      samples[i] = 0;
      continue;
    }

    bool high = (phase++ / SAMPLES_PER_HALF_WAVE) % 2 == 0;
    samples[i] = high ? TONE_AMPLITUDE : -TONE_AMPLITUDE;
  }

  if (dump_file != NULL) {
    fwrite(samples, sizeof samples[0], frames, dump_file);
  }
}

void sound_init(void) {
  ma_context *context = NULL;
  const char *dump_path = getenv("CHIP8_AUDIO_DUMP");
  if (dump_path != NULL) {
    ma_backend null_backend = ma_backend_null;
    ma_context_init(&null_backend, 1, NULL, &dump_context);
    context = &dump_context;
    dump_file = fopen(dump_path, "wb");
  }

  ma_device_config config = ma_device_config_init(ma_device_type_playback);
  config.playback.format = ma_format_s16;
  config.playback.channels = 1;
  config.sampleRate = SAMPLE_RATE;
  config.dataCallback = fill_samples;

  ma_device_init(context, &config, &device);
  ma_device_start(&device);
}

void sound_set_active(bool active) { tone_on = active; }

void sound_shutdown(void) {
  ma_device_uninit(&device);
  if (dump_file != NULL) {
    ma_context_uninit(&dump_context);
    fclose(dump_file);
  }
}
