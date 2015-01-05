#ifndef __VISUALIZER_H__
#define __VISUALIZER_H__

class AudioProcessor;
class Fur;

class Visualizer {
 public:
  // Does not take ownership of fur.
  explicit Visualizer(Fur* fur) : fur_(fur) {}
  virtual ~Visualizer() {}

  // Called to redraw hairs at a particular time.
  virtual void Draw(double time) = 0;

  // Called when hairs move.
  virtual void Reposition() {};

 protected:
  Fur* fur_;
};

#endif // __VISUALIZER_H__
