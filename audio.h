#ifndef __HALLUCINATION_AUDIO_H__
#define __HALLUCINATION_AUDIO_H__

// PortAudio includes
#include "portaudio.h"

// Aubio includes
#include <aubio/aubio.h>
#include <aubio/fvec.h>
#include <aubio/onset/onset.h>

class AudioProcessor {
public:
  AudioProcessor() {}

  int Init();

  // TODO(wcraddock): try to make these member variables private.

  // Aubio onset detector and state.
  fvec_t *onset_out_;
  aubio_onset_t *onset_obj_;

  // Aubio beat detector and state.
  fvec_t *tempo_out_;
  aubio_tempo_t *tempo_obj_;
};

#endif // __HALLUCINATION_AUDIO_H__