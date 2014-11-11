#!/bin/sh
echo 64000 > /sys/kernel/debug/tracing/buffer_size_kb
echo mmiotrace > /sys/kernel/debug/tracing/current_tracer
cat /sys/kernel/debug/tracing/trace_pipe
