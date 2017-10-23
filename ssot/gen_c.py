import argparse
import os
import importlib

parser = argparse.ArgumentParser(description='Generate C files from ssot.')
parser.add_argument('module', help='the module to process')
parser.add_argument('cdir', help='the output C directory name')
parser.add_argument('hdir', help='the output header directory name')
args = parser.parse_args()

cgen = importlib.import_module(args.module).cgen

os.makedirs(args.cdir + '/cgen', exist_ok=True)
os.makedirs(args.hdir + '/cgen', exist_ok=True)

with open('{}/cgen/{}.h'.format(args.hdir, cgen.name), 'w') as f:
    cgen.write_h(f)
with open('{}/cgen/{}.c'.format(args.cdir, cgen.name), 'w') as f:
    cgen.write_c(f)
