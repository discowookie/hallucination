#!/usr/bin/env python

import sys
import re

scale = 1.0/16.0

if len(sys.argv) < 3:
  print("Usage: " + sys.argv[0] + " in-file out-file")
  sys.exit();

vertex_regex = re.compile("([-\d.]+)\s+([-\d.]+)\s+([-\d.]+)")
face_regex = re.compile("((\d+)/?(\d+)?/?(\d+)?)+")

output = list()

with open(sys.argv[1], 'r') as file:
  for line in file:
    # Vertex lines are passed straight through
    if line.startswith('v '):
      matches = vertex_regex.findall(line)
      vertex = [float(v) * scale for v in matches[0]]
      output.append('v %f %f %f' % tuple(vertex))

    # Face lines have to broken up
    elif line.startswith('f '):
      matches = face_regex.findall(line)
      if len(matches) == 3:
        face = (matches[0][1], matches[1][1], matches[2][1])
        output.append('f %s %s %s' % face)
      if len(matches) == 4:
        face = (matches[0][1], matches[1][1], matches[2][1])
        output.append('f %s %s %s' % face)
        face = (matches[0][1], matches[2][1], matches[3][1])
        output.append('f %s %s %s' % face)

with open(sys.argv[2], 'w') as file:
  file.write('\n'.join(output))
