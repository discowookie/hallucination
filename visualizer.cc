#include "visualizer.h"

#include "audio.h"
#include "debug.h"
#include "hair.h"

PhotogrammetryVisualizer::PhotogrammetryVisualizer(Fur* fur)
  : Visualizer(fur),
    lit_hair_(-1),
    last_change_(0) {}

void PhotogrammetryVisualizer::Draw(double time) {
  std::vector<Hair>& hairs = fur_->hairs;
  for (unsigned int i = 0; i < hairs.size(); ++i) {
    Hair& hair = hairs[i];
    float illumination = 0.0f;

    // In this mode, each hair is lit for 1/10th of a second. The hairs
    // are cycled through in random order.
    if (time - last_change_ > 0.1f) {
      lit_hair_ = (lit_hair_ + 1) % hairs.size();
      last_change_ = time;
    }

    illumination = (i == lit_hair_) ? 1.0f : -1.0f;

    hair.SetGrey(illumination);
    // TODO(cody): separate hair layer into a separate class to allow merging.
    hair.Draw();
  }
}

RandomWaveVisualizer::RandomWaveVisualizer(Fur* fur)
  : Visualizer(fur) {}

void RandomWaveVisualizer::Draw(double time) {
  std::vector<Hair>& hairs = fur_->hairs;
  for (unsigned int i = 0; i < hairs.size(); ++i) {
    Hair& hair = hairs[i];  
    float illumination = sin(hair.frequency * time + hair.phase);
    hair.SetGrey(illumination);
    hair.Draw();
  }
}

BeatVisualizer::BeatVisualizer(Fur* fur, AudioProcessor* audio)
  : Visualizer(fur),
    audio_(audio),
    num_beats_(0) {}

void BeatVisualizer::Draw(double time) {
  // Determine confidence that some audio event has happened. The onset detector
  // is checked first, and it assigns a confidence value. The beat detector is
  // checked second, and its confidence overrides that from the onset detector.
  // 
  // The idea is that the Disco Wookie should fall back on onset detection when
  // the beat is not know with any confidence.

  float confidence = 0.0f;

  // The audio processor tells us when an onset event has occurred since the 
  // last time through this OpenGL display loop.
  float last_onset_s;
  bool is_onset = audio_->IsOnset(last_onset_s);
  if (is_onset) {
    static int num_onsets = 0;
    if (DEBUG_MODE) printf("onset %d: time %.3f s\n", num_onsets++, last_onset_s);

    // The aubio library does not provide confidence values for onsets.
    // TODO(wcraddock): what the hell is the right idea here?
    confidence = 0.5f;
  }

  // The audio processor tells us when a beat event has occurred since the 
  // last time through this OpenGL display loop.
  float beat_confidence;
  float last_beat_s, tempo_bpm;
  bool is_beat = audio_->IsBeat(last_beat_s, tempo_bpm, beat_confidence);
  if (is_beat) {
    // If the beat_confidence is very low, don't count it as a beat at all.
    // Otherwise, make it a strong visual event by giving it high confidence.
    if (beat_confidence >= 0.2f) {
      confidence = 1.0f;
      if (DEBUG_MODE) {
        printf("beat %d: time %.3f s, tempo %.2f bpm, confidence %.2f\n",
              num_beats_++, last_beat_s, tempo_bpm, confidence);
      }
    }
  }

  std::vector<Hair>& hairs = fur_->hairs;
  for (unsigned int i = 0; i < hairs.size(); ++i) {
    Hair& hair = hairs[i];
    float illumination = hair.illumination;

    if (is_onset || is_beat) {
      // Pick random hairs to light up to max brightness. Add the confidence
      // to it, to make it brighter.
      float r = ((double)rand() / (RAND_MAX));
      if (r > 0.8f) {
	illumination = std::min(hair.illumination + confidence, 1.0f);
      }
    } else {
      // If there is no beat or onset, make all the hairs decay in brightness.
      // Decays to 0.0f. Make this ratio closer to 1 to make the decay slower.
      illumination = hair.illumination * (63.0f / 64.0f);
    }

    hair.illumination = illumination;
    illumination = 2.0f * illumination - 1.0f;
    hair.SetGrey(illumination);
    hair.Draw();
  }
}
