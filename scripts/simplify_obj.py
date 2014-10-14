#!/usr/bin/env python

import sys
import re

if len(sys.argv) < 3:
  print("Usage: " + sys.argv[0] + " in-file out-file")
  sys.exit();

face_regex = re.compile("(\d+)/(\d+)/(\d+)")

output = list()

with open(sys.argv[1], 'r') as file:
  for line in file:
    # Vertex lines are passed straight through
    if line.startswith('v '):
      output.append(line.strip())

    # Face lines have to broken up
    elif line.startswith('f '):
      matches = face_regex.findall(line)
      if len(matches) == 0:
        print("Warning: no matches for line %s" % line)

      for match in matches:
        output.append("f %s %s %s" % match)

with open(sys.argv[2], 'w') as file:
  file.write('\n'.join(output))
