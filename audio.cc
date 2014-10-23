#include "audio.h"

// PortAudio includes
#include "portaudio.h"

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

  smpl_t is_onset = fvec_get_sample(ap->onset_out_, 0);
  if (is_onset) {
    ap->is_onset = true;
  }

  // If a beat occured in this hop, set the flag in AudioProcessor.
  // It will be picked up asynchronously by the OpenGL code.
  // TODO(wcraddock): might be best to use a real thread-safe queue, but I
  // don't see a synchronization problem with this.
  smpl_t is_beat = fvec_get_sample(ap->tempo_out_, 0);
  if (is_beat) {
    ap->is_beat = true;
  }

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

  PaStreamParameters inputParameters;
  inputParameters.device =
      Pa_GetDefaultInputDevice(); /* default input device */
  if (inputParameters.device == paNoDevice) {
    printf("Error: No default input device.\n");
    return paNoDevice;
  }
  inputParameters.channelCount = 1; /* mono input */
  inputParameters.sampleFormat = paFloat32;
  inputParameters.suggestedLatency =
      Pa_GetDeviceInfo(inputParameters.device)->defaultLowInputLatency;
  inputParameters.hostApiSpecificStreamInfo = NULL;

  // Open an audio I/O stream for one input (microphone).
  PaStream *stream;
  err = Pa_OpenStream(
      &stream,
      &inputParameters,          /* mono input */
      NULL,                      /* no output channels */
      sample_rate,
      hop_size,     /* frames per buffer, i.e. the number
                       of sample frames that PortAudio will
                       request from the callback. Many apps
                       may want to use
                       paFramesPerBufferUnspecified, which
                       tells PortAudio to pick the best,
                       possibly changing, buffer size.*/
      paNoFlag,
      paCallback, /* this is your callback function */
      this); /* This is a pointer that will be passed to the callback */
  if (err != paNoError)
    return err;

  // Start the input audio stream
  err = Pa_StartStream(stream);
  if (err != paNoError) {
    printf("Error: Could not start stream.\n");
    return err;
  }

  // Create the aubio onset detector
  onset_out_ = new_fvec(1);
  onset_obj_ = new_aubio_onset("default", win_size, hop_size, sample_rate);
  // aubio_onset_set_threshold(onset_obj_, 1.0f);
  aubio_onset_set_silence(onset_obj_, -40.0f);
  aubio_onset_set_minioi_s(onset_obj_, 0.01f); 

  // Create the aubio beat detector.
  tempo_out_ = new_fvec(2);
  tempo_obj_ = new_aubio_tempo("default", win_size, hop_size, sample_rate);
  // aubio_tempo_set_threshold(tempo_obj_, -50.0f);
  // aubio_tempo_set_silence (tempo_obj_, -90.0f);

  return paNoError;
}

bool AudioProcessor::IsBeat(float &last_beat_s, float &tempo_bpm,
                            float &confidence) {
  if (is_beat) {
    last_beat_s = aubio_tempo_get_last_s(tempo_obj_);
    tempo_bpm = aubio_tempo_get_bpm(tempo_obj_);
    confidence = aubio_tempo_get_confidence(tempo_obj_);

    is_beat = 0;
    return true;
  } else {
    return false;
  }
}

bool AudioProcessor::IsOnset(float &last_onset_s) {
  if (is_onset) {
    last_onset_s = aubio_onset_get_last_s(onset_obj_);

    is_onset = 0;
    return true;
  } else {
    return false;
  }
}

AudioProcessor::~AudioProcessor() { Pa_Terminate(); }