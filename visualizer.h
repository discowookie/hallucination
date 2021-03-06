#ifndef __VISUALIZER_H__
#define __VISUALIZER_H__

#include <vector>

using std::vector;

class AudioProcessor;
class Fur;

class Visualizer {
 public:
  // Does not take ownership of fur.
  explicit Visualizer(Fur* fur) : fur_(fur) {}
  virtual ~Visualizer() {}

  // Called to redraw hairs at a particular time.
  void Draw(double time);

  // Called to set the illumination of all hairs.
  virtual void Illuminate(double time) = 0;

  // Called when hairs move.
  virtual void Reposition() {};

 protected:
  Fur* fur_;
};

class PhotogrammetryVisualizer : public Visualizer {
 public:
  explicit PhotogrammetryVisualizer(Fur* fur);
  virtual ~PhotogrammetryVisualizer() {}
  virtual void Illuminate(double time);

 private:
  int lit_hair_;
  double last_change_;
};

class RandomWaveVisualizer : public Visualizer {
 public:
  explicit RandomWaveVisualizer(Fur* fur);
  virtual ~RandomWaveVisualizer() {}
  virtual void Illuminate(double time);
  virtual void Reposition();

 private:
  vector<double> frequency_;
  vector<double> phase_;
};

class BeatVisualizer : public Visualizer {
 public:
  // Does not take ownership of audio, which must outlive this.
  BeatVisualizer(Fur* fur, AudioProcessor* audio);
  virtual ~BeatVisualizer() {}
  virtual void Illuminate(double time);
  virtual void Reposition();

 private:
  AudioProcessor* audio_;
  int num_beats_;
  vector<float> illumination_;
};

#endif // __VISUALIZER_H__
