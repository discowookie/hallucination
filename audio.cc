#include "audio.h"

#include <stdio.h>

static int paCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags, void *userData) {
  AudioProcessor *ap = static_cast<AudioProcessor *>(userData);

  float *in = (float *)inputBuffer;

  // TODO(wcraddock): put these in a class.
  int win_size = 1024;
  int hop_size = win_size / 4;

  // Run the aubio onset and beat detectors.
  fvec_t in_vec = { hop_size, in };
  aubio_onset_do(ap->onset_obj_, &in_vec, ap->onset_out_);
  aubio_tempo_do(ap->tempo_obj_, &in_vec, ap->tempo_out_);

  return 0;
}

int AudioProcessor::Init() {
  // Initialize PortAudio
  PaError err = Pa_Initialize();
  if (err != paNoError) {
    printf("PortAudio error: %s\n", Pa_GetErrorText(err));
    return err;
  }

  // TODO(wcraddock): put these parameters into the class constructor.
  uint_t win_size = 1024;
  uint_t hop_size = win_size / 4;
  uint_t sample_rate = 44100;

  // Open an audio I/O stream for one input (microphone).
  PaStream *stream;
  err = Pa_OpenDefaultStream(
      &stream, 1,           /* mono input */
      0,                    /* no output channels */
      paFloat32,            /* 32 bit floating point output */
      sample_rate, hop_size, /* frames per buffer, i.e. the number
                               of sample frames that PortAudio will
                               request from the callback. Many apps
                               may want to use
                               paFramesPerBufferUnspecified, which
                               tells PortAudio to pick the best,
                               possibly changing, buffer size.*/
      paCallback, /* this is your callback function */
      this);         /* This is a pointer that will be passed to the callback */
  if (err != paNoError)
    return err;

  // Start the input audio stream
  err = Pa_StartStream(stream);
  if (err != paNoError)
    return err;

  // Create the aubio onset detector
  onset_out_ = new_fvec(1);
  onset_obj_ = new_aubio_onset("default", win_size, hop_size, sample_rate);
  aubio_onset_set_threshold(onset_obj_, 0.0f);
  aubio_onset_set_silence(onset_obj_, -90.0f);

  // Create the aubio beat detector.
  tempo_out_ = new_fvec(2);
  tempo_obj_ = new_aubio_tempo("default", win_size, hop_size, sample_rate);
  aubio_tempo_set_threshold(tempo_obj_, -10.0f);
  // aubio_tempo_set_silence (tempo_obj_, -90.0f);
  
  return paNoError;
}

bool AudioProcessor::IsBeat(float& last_beat_s, float& tempo_bpm, float& confidence) {
  smpl_t is_beat = fvec_get_sample(tempo_out_, 0);

  if (is_beat) {
    last_beat_s = aubio_tempo_get_last_s(tempo_obj_);
    tempo_bpm = aubio_tempo_get_bpm(tempo_obj_);
    confidence = aubio_tempo_get_confidence(tempo_obj_);
  }

  return is_beat;
}