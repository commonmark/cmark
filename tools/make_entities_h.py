# Creates C data structures for binary lookup table of entities,
# using python's html5 entity data.
# Usage: python3 tools/make_entities_h.py > src/entities.h

import html

# We use this simple hashing algorithm to convert a string
# to an integer:
def djb2(s):
  bs = list(s.encode('utf-8'))
  hash = 5381
  for b in bs:
    hash = (((hash << 5) + hash) + b) & 0xFFFFFFFF
  return hash

entities5 = html.entities.html5

# Note that most entries in the entity table end with ';', but in a few
# cases we have both a version with ';' and one without, so we strip out
# the latter to avoid duplicates:
hashed_data = sorted([[int(djb2(s[:-1])), entities5[s].encode('utf-8'), s]
                      for s in entities5.keys() if s[-1] == ';'])

# indices is a dictionary - given a hash it spits out the ordering
# of this entity in the list (the array index)
indices = {}
i = 0

for x in hashed_data:
  indices[x[0]] = i
  i = i + 1

# Formats integer as C octal escape.
def toesc(x):
  return '\\' + oct(x)[2:]

# Lines is the list of lines in the array.
# We don't fill them in order, so we initialize the whole array first.
lines = [""] * len(hashed_data)

# Takes hashed_data or some sublist of it, and a midpoint (array index)
# in this list.  Adds to lines a line for the midpoint, then calls
# itself recursively for the earlier and later elements.  Each node
# contains indices for elements with a lesser hash and elements with
# a greater hash.  An index of -1 means we're at a leaf node.
def to_binary_array(xs, mid):
  # divide in half, and form binary array from each half
  x = xs[mid]
  lesses = xs[0:mid]
  greaters = xs[mid+1:]
  midlesses = len(lesses) // 2
  midgreaters = len(greaters) // 2
  if len(lesses) == 0:
    ml = -1
  else:
    ml = indices[lesses[midlesses][0]]
  if len(greaters) == 0:
    mg = -1
  else:
    mg = indices[greaters[midgreaters][0]]
  lines[indices[x[0]]] = ("{" + str(x[0]) + ", (unsigned char*)\"" +
                          ''.join(map(toesc, x[1])) + "\", " + str(ml) +
                          ", " + str(mg) + "}, /* &" + x[2] + " */")
  if len(lesses) > 0:
    to_binary_array(lesses, midlesses)
  if len(greaters) > 0:
    to_binary_array(greaters, midgreaters)

# Now call this to fill up the array lines:
mid = len(hashed_data) // 2
to_binary_array(hashed_data, mid)

# Print out the header:
print("""#ifndef CMARK_ENTITIES_H
#define CMARK_ENTITIES_H

#ifdef __cplusplus
extern "C" {
#endif

struct cmark_entity_node {
	unsigned long value;
	unsigned char *bytes;
	int less;
	int greater;
};

#define CMARK_ENTITY_MIN_LENGTH 2
#define CMARK_ENTITY_MAX_LENGTH 31
""")

print("static const struct cmark_entity_node cmark_entities[] = {");

for line in lines:
  print(line);

print("};\n");

print("static const int cmark_entities_root = " + str(mid) + ";");

print("""
#ifdef __cplusplus
}
#endif

#endif
""")
