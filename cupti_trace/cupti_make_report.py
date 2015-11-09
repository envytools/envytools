#!/usr/bin/python3

# Copyright (C) 2015 Samuel Pitoiset <samuel.pitoiset@gmail.com>
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.

import getopt
import os
import re
import subprocess
import shutil
import sys

def get_cupti_path():
    return os.getenv("CUPTI_PATH", "/opt/cuda/extras/CUPTI")

def cupti_query_parse_output(output, token):
    data = []
    for line in output.splitlines():
        if re.search(token, line):
            b = line.find('=') + 2
            e = line.find('\n')
            data.append(line[b:])
    return data

def cupti_get_domain_ids():
    return cupti_query_parse_output(cupti_query_domains(), "^Id")

def cupti_query(opts):
    cmd = get_cupti_path() + "/sample/cupti_query/cupti_query " + opts
    proc = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if not proc.returncode == 0:
        return proc.returncode
    return stdout.decode()

def cupti_query_domains():
    return cupti_query("-getdomains")

def cupti_query_events_by_domain(domain):
    return cupti_query("-domain " + str(domain) + " -getevents")

def cupti_query_metrics():
    return cupti_query("-getmetrics")

def cupti_save_domains_list():
    f = open("list_domains.txt", "w")
    f.write(cupti_query_domains())
    f.close()

def cupti_save_events_list():
    domain_ids = cupti_get_domain_ids()
    for domain_id in domain_ids:
        f = open("domain_" + str(domain_id) + ".txt", "w")
        f.write(cupti_query_events_by_domain(domain_id))
        f.close()

def cupti_save_metrics_list():
    f = open("list_metrics.txt", "w")
    f.write(cupti_query_metrics())
    f.close()

def cupti_trace(chipset, opts):
    cmd = "cupti_trace -a " + chipset + " " + opts
    proc = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if not proc.returncode == 0:
        return proc.returncode
    return stdout.decode()

def cupti_get_event_names(domain_id):
    output = cupti_query_events_by_domain(domain_id)
    return cupti_query_parse_output(output, "^Name")

def cupti_trace_all_events(chipset):
    f = open("report_events.txt", "w")
    domain_ids = cupti_get_domain_ids()
    for domain_id in domain_ids:
        print ("Domain #" + str(domain_id))
        event_names = cupti_get_event_names(domain_id)
        for event_name in event_names:
            print ("Event " + event_name)
            f.write(cupti_trace(chipset, "-e " + event_name))
    f.close()

def cupti_get_metric_names():
    return cupti_query_parse_output(cupti_query_metrics(), "^Name")

def cupti_trace_all_metrics(chipset):
    f = open("report_metrics.txt", "w")
    metric_names = cupti_get_metric_names()
    for metric_name in metric_names:
        print ("Metric " + metric_name)
        f.write(cupti_trace(chipset, "-m " + metric_name))
    f.close()

def dry_run_valgrind_mmt():
    cmd = "valgrind --tool=mmt"
    proc = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
    stdout, stderr = proc.communicate()
    if not proc.returncode == 1:
        return proc.returncode
    lines = stderr.decode().splitlines()
    if not lines[0] == "valgrind: no program specified":
        return 0
    return 1

def main():
    try:
        long_opts = ["chipset=",
                     "overwrite"]
        opts, args = getopt.getopt(sys.argv[1:], "a:o", long_opts)
    except getopt.GetoptError as err:
        print (str(err))
        sys.exit(2)

    chipset   = None
    overwrite = False
    for opt, arg in opts:
        if opt in ("-a", "--chipset"):
            chipset = str(arg)
        elif opt in ("-o", "--overwrite"):
            overwrite = True
        else:
            assert False, "Unknown option!"

    if chipset == None:
        print ("Must specify a chipset (-a)")
        sys.exit(2)

    output_dir = "nv" + chipset + "_cupti_report"
    if os.path.exists(output_dir):
        if not overwrite:
            print ("Output directory already exists, try --overwrite!")
            sys.exit(2)
        else:
            shutil.rmtree(output_dir, ignore_errors=True)
    os.mkdir(output_dir)
    os.chdir(output_dir)

    if not dry_run_valgrind_mmt():
        print ("You are not running valgrind-mmt!")
        sys.exit(2)
    if not shutil.which("demmt"):
        print ("Failed to find demt!")

    # Check CUPTI samples
    path = get_cupti_path() + "/sample/cupti_query/cupti_query"
    if not os.path.exists(path):
        print ("Failed to find cupti_query!")
    path = get_cupti_path() + "/sample/callback_event/callback_event"
    if not os.path.exists(path):
        print ("Failed to find callback_event!")
    path = get_cupti_path() + "/sample/callback_metric/callback_metric"
    if not os.path.exists(path):
        print ("Failed to find callback_metric!")

    cupti_save_domains_list()
    cupti_save_events_list()
    cupti_save_metrics_list()

    cupti_trace_all_events(chipset)
    cupti_trace_all_metrics(chipset)

    print ("Creating a tarball...")
    os.chdir("../")
    if shutil.which("tar"):
        archive_name = output_dir + ".tar.gz"
        cmd = "tar -czf " + archive_name + " " + output_dir
        proc = subprocess.Popen(cmd.split())
        stdout, stderr = proc.communicate()
        if not proc.returncode == 0:
            return proc.returncode
        if shutil.which("xz"):
            cmd = "xz " + archive_name
            proc = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE)
            stdout, stderr = proc.communicate()
            if not proc.returncode == 0:
                return proc.returncode

    print ("Thanks for running cupti_trace! :-)")

if __name__ == "__main__":
    main()
