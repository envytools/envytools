#!/usr/bin/env python3

import os, os.path
import re

valid_files = set()

for root, dirs, files in os.walk('.'):
	for fn in files:
		if fn.endswith('.txt'):
			valid_files.add(os.path.normpath(os.path.join(root, fn)))

index_files = set()

with open('index.txt') as index:
	r = re.compile(r'\[[- *]{5}\] [^-]')
	for line in index:
		if r.match(line):
			fname = line[8:]
			fname = fname[:fname.index(' ')]
			index_files.add(fname)

for f in valid_files:
	if f not in index_files:
		print("{} not in index".format(f))

for f in index_files:
	if f not in valid_files:
		print("{} not found".format(f))

r = re.compile(r'([^ ,\[\]]*\.txt)')

for root, dirs, files in os.walk('.'):
	for fn in files:
		if fn.endswith('.txt'):
			fname = os.path.join(root, fn)
			with open(fname) as f:
				for line in f.readlines():
					for m in r.findall(line):
						if m not in index_files:
							print(fname, line.rstrip())
