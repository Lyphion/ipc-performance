#!/bin/bash

program=./cmake-build-release/ipc
handlers=("dbus" "fifo" "queue" "dgram" "stream" "udp" "tcp" "memory" "mapped" "file")

cpu_reader=0
cpu_writer=1
cpu_writer_dual=18

logs=./logs/$(date +%F_%H-%M-%S)
mkdir -p "$logs"

echo "Running latency benchmark"

iterations=1000
delay=10

for handler in "${handlers[@]}"; do
  echo "> Running $handler (single socket)"

  taskset -c "$cpu_reader" "$program" "latency" "$handler" "reader" "$iterations" "$delay" >> "$logs/latency_$handler""_reader.log" 2>&1 &
  sleep 0.2 && taskset -c "$cpu_writer" "$program" "latency" "$handler" "writer" "$iterations" "$delay" >> "$logs/latency_$handler""_writer.log" 2>&1 &

  wait && sleep 1
done

for handler in "${handlers[@]}"; do
  echo "> Running $handler (dual socket)"

  taskset -c "$cpu_reader" "$program" "latency" "$handler" "reader" "$iterations" "$delay" >> "$logs/latency_$handler""_reader.log" 2>&1 &
  sleep 0.2 && taskset -c "$cpu_writer_dual" "$program" "latency" "$handler" "writer" "$iterations" "$delay" >> "$logs/latency_$handler""_writer.log" 2>&1 &

  wait && sleep 1
done

echo "Running throughput benchmark"

iterations=1000000
sizes=(4 8 16 32 64 128 256 512)

for handler in "${handlers[@]}"; do
  for size in "${sizes[@]}"; do
    echo "> Running $handler with $size Bytes"

    taskset -c "$cpu_reader" "$program" "throughput" "$handler" "reader" "$iterations" "$size" >> "$logs/throughput_$handler""_reader.log" 2>&1 &
    sleep 0.2 && taskset -c "$cpu_writer" "$program" "throughput" "$handler" "writer" "$iterations" "$size" >> "$logs/throughput_$handler""_writer.log" 2>&1 &

    wait && sleep 1
  done
done

echo "Running execution time benchmark"

iterations=1000
sizes=(32 128 512)
delay=10

for handler in "${handlers[@]}"; do
  for size in "${sizes[@]}"; do
    echo "> Running $handler with $size Bytes"

    taskset -c "$cpu_reader" "$program" "execution" "$handler" "reader" "$iterations" "$size" "$delay" >> "$logs/execution_$handler""_reader.log" 2>&1 &
    sleep 0.2 && taskset -c "$cpu_writer" "$program" "execution" "$handler" "writer" "$iterations" "$size" "$delay" >> "$logs/execution_$handler""_writer.log" 2>&1 &

    wait && sleep 1
  done
done

echo "Running real world benchmark"

paths=("./testdata/fifo_trace.txt")
threshold=10

for handler in "${handlers[@]}"; do
  for path in "${paths[@]}"; do
    echo "> Running $handler with $path"

    taskset -c "$cpu_reader" "$program" "realworld" "$handler" "reader" "$path" "$threshold" >> "$logs/realworld_$handler""_reader.log" 2>&1 &
    sleep 0.2 && taskset -c "$cpu_writer" "$program" "realworld" "$handler" "writer" "$path" "$threshold" >> "$logs/realworld_$handler""_writer.log" 2>&1 &

    wait && sleep 1
  done
done

echo "Running reduced energy benchmark"

iterations=10000
delay=10

for handler in "${handlers[@]}"; do
  echo "> Running $handler with reduced data"

  metricq-summary -d -- bash -c "taskset -c $cpu_reader timeout 60 $program latency $handler reader $iterations $delay >> /dev/null &
    sleep 0.2 && taskset -c $cpu_writer timeout 59 $program latency $handler writer $iterations $delay >> /dev/null &
    wait" >> "$logs/energy_$handler""_reduced.log" 2>&1

  sleep 1
done

echo "Running full energy benchmark"

iterations=1000000000
size=128

for handler in "${handlers[@]}"; do
  echo "> Running $handler with many data"

  metricq-summary -d -- bash -c "taskset -c $cpu_reader timeout 60 $program throughput $handler reader $iterations $size >> /dev/null &
    sleep 0.2 && taskset -c $cpu_writer timeout 59 $program throughput $handler writer $iterations $size >> /dev/null &
    wait" >> "$logs/energy_$handler""_full.log" 2>&1

  sleep 1
done

echo "Finished benchmarks"
