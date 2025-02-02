# cache-simulator-cpp

## Overview
This project is a **cache simulator** that models the behavior of a multi-level cache hierarchy (L1 and L2 caches). It simulates cache reads, writes, misses, and writebacks, and provides detailed statistics about cache performance. The simulator is designed to be configurable, allowing users to specify cache sizes, associativity, block sizes, and other parameters.

The simulator reads a trace file containing memory access patterns and outputs cache statistics, including hit rates, miss rates, and memory traffic.

---

## Features
- Simulates **L1 and L2 caches** with configurable sizes, associativity, and block sizes.
- Supports **LRU (Least Recently Used)** replacement policy.
- Tracks cache statistics, including:
  - Reads, writes, read misses, and write misses.
  - Hit rates and miss rates.
  - Writebacks and memory traffic.
- Outputs cache contents and performance metrics in a readable format.

---

## Requirements
- **C++ Compiler**: The project requires a C++ compiler that supports C++11 or later (e.g., `g++`).
- **Makefile**: A `Makefile` is provided for easy compilation.
- **Trace File**: A trace file containing memory access patterns (e.g., `gcc_trace.txt`).

---

## Building the Simulator
1. Clone the repository or download the source code.
2. Navigate to the project directory:
   ```bash
   cd cache-simulator
   ```
3. Compile the simulator using the provided Makefile:
   ```bash
   make
   ```
   This will generate an executable named `cache_simulator`.

---

## Running the Simulator
To run the simulator, use the following command:
```bash
./cache_simulator <BLOCKSIZE> <L1_SIZE> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <PREF_N> <PREF_M> <trace_file>
```

### Parameters
| Parameter  | Description |
|------------|-------------|
| `BLOCKSIZE` | Size of each cache block (in bytes). |
| `L1_SIZE`   | Total size of the L1 cache (in bytes). |
| `L1_ASSOC`  | Associativity of the L1 cache (number of blocks per set). |
| `L2_SIZE`   | Total size of the L2 cache (in bytes). Set to 0 if L2 cache is disabled. |
| `L2_ASSOC`  | Associativity of the L2 cache (number of blocks per set). |
| `PREF_N`    | Prefetching parameter N (number of blocks to prefetch). |
| `PREF_M`    | Prefetching parameter M (stride for prefetching). |
| `trace_file`| Path to the trace file containing memory access patterns. |

### Example Command
```bash
./cache_simulator 32 8192 4 262144 8 3 10 gcc_trace.txt
```

---

## Trace File Format
The trace file should contain a series of memory access patterns, with each line formatted as follows:
```
<access_type> <address>
```
- `<access_type>`: Either `r` (read) or `w` (write).
- `<address>`: The memory address being accessed (in hexadecimal format).

### Example Trace File
```
r 0x1000
w 0x2000
r 0x3000
w 0x4000
```

---

## Output
The simulator outputs the following:

### Cache Contents:
Displays the contents of the L1 and L2 caches, including valid blocks, tags, and dirty bits.

### Performance Statistics:
- L1 and L2 cache statistics, including reads, writes, misses, hit rates, and writebacks.
- Memory traffic (number of accesses to main memory).

### Example Output
```
===== L1 contents =====
set 0: 0x1000 D 0x2000
set 1: 0x3000
===== L2 contents =====
set 0: 0x4000 D 0x5000
set 1: 0x6000
===== Measurements =====
----- L1 Cache -----
a. L1 reads                  : 1000
b. L1 read misses            : 200
c. L1 writes                 : 500
d. L1 write misses           : 100
e. L1 miss rate              : 0.2000
f. L1 writebacks             : 50
g. L1 prefetches             : 0
----- L2 Cache -----
h. L2 reads (demand)         : 300
i. L2 read misses (demand)   : 60
j. L2 reads (prefetch)       : 0
k. L2 read misses (prefetch) : 0
l. L2 writes                 : 150
m. L2 write misses           : 30
n. L2 miss rate              : 0.2000
o. L2 writebacks             : 20
p. L2 prefetches             : 0
----- Memory -----
q. Memory traffic            : 100
```

---

## Customization
- **Cache Parameters**: Modify the cache sizes, associativity, and block sizes by changing the command-line arguments.
- **Prefetching**: Implement prefetching logic by modifying the `handle_miss` function in the `Cache` class.
- **Replacement Policy**: Replace the LRU policy with another algorithm (e.g., FIFO, Random) by updating the `update_lru` and `find_lru_block` functions.
