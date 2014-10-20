#! /usr/bin/env python

from numpy.testing import TestCase, run_module_suite
from numpy.testing import assert_equal, assert_almost_equal
from aubio import onset

class aubio_onset_default(TestCase):

    def test_members(self):
        o = onset()
        assert_equal ([o.buf_size, o.hop_size, o.method, o.samplerate],
            [1024,512,'default',44100])

class aubio_onset_params(TestCase):

    samplerate = 44100

    def setUp(self):
        self.o = onset(samplerate = self.samplerate)

    def test_get_delay(self):
        assert_equal (self.o.get_delay(), int(4.3 * self.o.hop_size))

    def test_get_delay_s(self):
        assert_almost_equal (self.o.get_delay_s(), self.o.get_delay() / float(self.samplerate))

    def test_get_delay_ms(self):
        assert_almost_equal (self.o.get_delay_ms(), self.o.get_delay() * 1000. / self.samplerate, 5)

    def test_get_minioi(self):
        assert_almost_equal (self.o.get_minioi(), 0.02 * self.samplerate)

    def test_get_minioi_s(self):
        assert_almost_equal (self.o.get_minioi_s(), 0.02)

    def test_get_minioi_ms(self):
        assert_equal (self.o.get_minioi_ms(), 20.)

    def test_get_threshold(self):
        assert_almost_equal (self.o.get_threshold(), 0.3)

    def test_set_delay(self):
        val = 256
        self.o.set_delay(val)
        assert_equal (self.o.get_delay(), val)

    def test_set_delay_s(self):
        val = .05
        self.o.set_delay_s(val)
        assert_almost_equal (self.o.get_delay_s(), val)

    def test_set_delay_ms(self):
        val = 50.
        self.o.set_delay_ms(val)
        assert_almost_equal (self.o.get_delay_ms(), val)

    def test_set_minioi(self):
        val = 200
        self.o.set_minioi(val)
        assert_equal (self.o.get_minioi(), val)

    def test_set_minioi_s(self):
        val = 0.04
        self.o.set_minioi_s(val)
        assert_almost_equal (self.o.get_minioi_s(), val)

    def test_set_minioi_ms(self):
        val = 40.
        self.o.set_minioi_ms(val)
        assert_almost_equal (self.o.get_minioi_ms(), val)

    def test_set_threshold(self):
        val = 0.2
        self.o.set_threshold(val)
        assert_almost_equal (self.o.get_threshold(), val)

class aubio_onset_96000(aubio_onset_params):
    samplerate = 96000

class aubio_onset_32000(aubio_onset_params):
    samplerate = 32000

class aubio_onset_8000(aubio_onset_params):
    samplerate = 8000

if __name__ == '__main__':
    from unittest import main
    main()
