#!/usr/bin/env python

import sys
import re

scale = 1.0

if len(sys.argv) < 2:
  print("Usage: " + sys.argv[0] + " in-file [out-file]")
  sys.exit();

if len(sys.argv) == 3:
  group_name = sys.argv[2]
else:
  group_name = "unknown.obj"

vertex_regex = re.compile("([-\d.]+)\s+([-\d.]+)\s+([-\d.]+)")
face_regex = re.compile("((\d+)/?(\d+)?/?(\d+)?)+")

vertices = list()
faces = list()

def write_output_to_file():
  print("About to write %d faces to file with name %s" %
              (len(faces), group_name))
  with open(group_name, 'w') as file:
    file.write('\n'.join(vertices + faces))

with open(sys.argv[1], 'r') as file:
  for line in file:
    # Vertex lines are passed straight through
    if line.startswith('v '):
      matches = vertex_regex.findall(line)
      vertex = [float(v) * scale for v in matches[0]]
      vertices.append('v %f %f %f' % tuple(vertex))

    # Start a new group
    elif line.startswith('g '):
      if len(faces):
        write_output_to_file()
        faces = list()
      g, group_name = line.split(' ')
      group_name = group_name.strip()
      print("Started new group name %s" % group_name)

    # Face lines have to broken up
    elif line.startswith('f '):
      matches = face_regex.findall(line)
      if len(matches) == 3:
        face = (matches[0][1], matches[1][1], matches[2][1])
        faces.append('f %s %s %s' % face)
      elif len(matches) == 4:
        face = (matches[0][1], matches[1][1], matches[2][1])
        faces.append('f %s %s %s' % face)
        face = (matches[0][1], matches[2][1], matches[3][1])
        faces.append('f %s %s %s' % face)
      else:
        print("Warning: ignored %d-sided polygon" % len(matches))

# Write the last group to disk, if there is one.
if len(faces):
  write_output_to_file()