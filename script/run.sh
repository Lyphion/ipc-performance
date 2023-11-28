#!/bin/bash

program=./cmake-build-release/ipc
handlers=("dbus" "fifo" "queue" "dgram" "stream" "udp" "tcp" "memory" "mapped" "file")

cpu_reader=0
cpu_writer=1

logs=./logs/$(date +%F_%H-%M-%S)
mkdir -p "$logs"

echo "Running latency benchmark"

iterations=1000
delay=10

for handler in "${handlers[@]}"; do
  echo "> Running $handler"

  taskset -c "$cpu_reader" "$program" "latency" "$handler" "reader" "$iterations" "$delay" >> "$logs/latency_$handler""_reader.log" 2>&1 &
  sleep 0.2 && taskset -c "$cpu_writer" "$program" "latency" "$handler" "writer" "$iterations" "$delay" >> "$logs/latency_$handler""_writer.log" 2>&1 &

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

path=./testdata/fifo_trace.txt
threshold=10

for handler in "${handlers[@]}"; do
  echo "> Running $handler"

  taskset -c "$cpu_reader" "$program" "realworld" "$handler" "reader" "$path" "$threshold" >> "$logs/realworld_$handler""_reader.log" 2>&1 &
  sleep 0.2 && taskset -c "$cpu_writer" "$program" "realworld" "$handler" "writer" "$path" "$threshold" >> "$logs/realworld_$handler""_writer.log" 2>&1 &

  wait && sleep 1
done

echo "Finished benchmarks"
