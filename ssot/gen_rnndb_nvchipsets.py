from ssot.gpu import Gpu

import argparse

parser = argparse.ArgumentParser(description='Generate nvchipsets.xml.')
parser.add_argument('file', help='the output file name')
args = parser.parse_args()

with open(args.file, 'w') as f:
    f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
    f.write('<database>\n')
    f.write('<enum name="chipset" bare="yes">\n')
    for gpu in Gpu.instances:
        if gpu.id is None or gpu.id is ...:
            f.write('\t<value name="{}"/>\n'.format(gpu.name))
        else:
            f.write('\t<value value="{:#05x}" name="{}"/>\n'.format(gpu.id, gpu.name))
    f.write('</enum>\n')
    f.write('</database>\n')
