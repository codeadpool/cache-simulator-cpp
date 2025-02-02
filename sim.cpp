#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <bits/stdc++.h>
#include <sstream>
#include <unordered_map>

using namespace std;

const uint32_t ADDRESS_BITS = 32;
const uint32_t PREFETCH_DISABLED = 0;

// Cache block structure
struct CacheBlock {
    bool valid;    // Valid bit
    bool dirty;    // Dirty bit
    uint32_t tag;  // Tag bits
    uint32_t rank; // LRU rank
};

// Cache class
class Cache {
public:
    uint32_t block_size;              // Block size in bytes
    uint32_t size;                    // Cache size in bytes
    uint32_t associativity;           // Associativity
    uint32_t sets;                    // Number of sets
    uint32_t index_bits;              // Number of index bits
    uint32_t block_offset_bits;       // Number of block offset bits
    uint32_t tag_bits;                // Number of tag bits
    vector<vector<CacheBlock>> cache; // Cache structure
    vector<int> current_size;         // Current size of each set

    // Sttats
    uint32_t reads = 0;
    uint32_t read_misses = 0;
    uint32_t writes = 0;
    uint32_t write_misses = 0;
    uint32_t writebacks = 0;

    // Constructor
    Cache(uint32_t blk_size, uint32_t cache_size, uint32_t assoc) 
        : block_size(blk_size), size(cache_size), associativity(assoc) {
        if (size == 0 || associativity == 0) return; // Disabled cache

        sets = size / (associativity * block_size);
        index_bits = log2(sets);
        block_offset_bits = log2(block_size);
        tag_bits = ADDRESS_BITS - index_bits - block_offset_bits;

        // Initializ cache
        cache = vector<vector<CacheBlock>>(sets, vector<CacheBlock>(associativity));
        current_size = vector<int>(sets, 0);

        for (uint32_t i = 0; i < sets; i++) {
            for (uint32_t j = 0; j < associativity; j++) {
                cache[i][j] = {false, false, 0, j}; // Initialize all blocks
            }
        }
    }

    // Parse address into tag, index, and offset
    void parse_address(uint32_t addr, uint32_t &tag, uint32_t &index) {
        index = (addr >> block_offset_bits) & ((1 << index_bits) - 1);
        tag = addr >> (block_offset_bits + index_bits);
    }

    // Find a block in the cache
    int find_block(uint32_t index, uint32_t tag) {
        for (uint32_t i = 0; i < associativity; i++) {
            if (cache[index][i].valid && cache[index][i].tag == tag) {
                return i; // Block found
            }
        }
        return -1; // Block not found
    }

    // Update LRU ranks
    void update_lru(uint32_t index, uint32_t loc) {
        for (uint32_t i = 0; i < associativity; i++) {
            if (cache[index][i].rank < cache[index][loc].rank) {
                cache[index][i].rank++;
            }
        }
        cache[index][loc].rank = 0; // Mark as most recently used
    }

    // Find the block with the highest rank (LRU)
    uint32_t find_lru_block(uint32_t index) {
        uint32_t lru_loc = 0;
        uint32_t max_rank = 0;
        for (uint32_t i = 0; i < associativity; i++) {
            if (cache[index][i].rank > max_rank) {
                max_rank = cache[index][i].rank;
                lru_loc = i;
            }
        }
        return lru_loc;
    }

    // Handle a read request
    void read(uint32_t addr) {
        reads++;
        uint32_t tag, index;
        parse_address(addr, tag, index);

        int loc = find_block(index, tag);
        if (loc != -1) { // hit
            update_lru(index, loc);
        } else { // miss
            read_misses++;
            handle_miss(index, tag, 'r');
        }
    }

    // Handle a write request
    void write(uint32_t addr) {
        writes++;
        uint32_t tag, index;
        parse_address(addr, tag, index);

        int loc = find_block(index, tag);
        if (loc != -1) { // hit
            cache[index][loc].dirty = true;
            update_lru(index, loc);
        } else { // miss
            write_misses++;
            handle_miss(index, tag, 'w');
        }
    }

    // Handle a cache miss
    void handle_miss(uint32_t index, uint32_t tag, char type) {
        if (current_size[index] < associativity) { // No eviction needed
            current_size[index]++;
        } else { // Eviction needed
            uint32_t lru_loc = find_lru_block(index);
            if (cache[index][lru_loc].dirty) {
                writebacks++; // Write back if dirty
            }
        }

        // Allocate new block
        uint32_t new_loc = find_lru_block(index);
        cache[index][new_loc] = {true, (type == 'w'), tag, 0};
        update_lru(index, new_loc);
    }

    // Print cache contents
    void print_contents(const string &cache_name) {
        cout << "===== " << cache_name << " contents =====" << endl;
        for (uint32_t i = 0; i < sets; i++) {
            cout << "set " << dec << i << ":";
            map<uint32_t, pair<uint32_t, bool>> block_map; // Sort by rank
            for (uint32_t j = 0; j < associativity; j++) {
                if (cache[i][j].valid) {
                    block_map[cache[i][j].rank] = {cache[i][j].tag, cache[i][j].dirty};
                }
            }
            for (auto &entry : block_map) {
                cout << " " << hex << entry.second.first;
                if (entry.second.second) cout << " D";
            }
            cout << endl;
        }
    }
};

// Simulator class
class Simulator {
public:
    Cache l1_cache;
    Cache l2_cache;
    uint32_t mem_traffic = 0;

    // Constructor
    Simulator(uint32_t blk_size, uint32_t l1_size, uint32_t l1_assoc, uint32_t l2_size, uint32_t l2_assoc)
        : l1_cache(blk_size, l1_size, l1_assoc), l2_cache(blk_size, l2_size, l2_assoc) {}

    // Handle a memory access
    void access(char rw, uint32_t addr) {
        if (rw == 'r') {
            l1_cache.read(addr);
        } else if (rw == 'w') {
            l1_cache.write(addr);
        }
    }

  void print_stats() {
      // Helper function to print a single statistic
      auto print_stat = [](const string &label, const string &value) {
          cout << left << setw(30) << label << ": " << value << endl;
      };
  
      // Helper function to calculate and format a rate
      auto format_rate = [](uint32_t numerator, uint32_t denominator) -> string {
          if (denominator == 0) return "0.0000"; // Avoid division by zero
          double rate = (double)numerator / denominator;
          stringstream ss;
          ss << fixed << setprecision(4) << rate;
          return ss.str();
      };
  
      cout << "===== Measurements =====" << endl;
  
      // L1 Cache Statistics
      cout << "----- L1 Cache -----" << endl;
      print_stat("a. L1 reads", to_string(l1_cache.reads));
      print_stat("b. L1 read misses", to_string(l1_cache.read_misses));
      print_stat("c. L1 writes", to_string(l1_cache.writes));
      print_stat("d. L1 write misses", to_string(l1_cache.write_misses));
      print_stat("e. L1 miss rate", 
                 format_rate(l1_cache.read_misses + l1_cache.write_misses, l1_cache.reads + l1_cache.writes));
      print_stat("f. L1 writebacks", to_string(l1_cache.writebacks));
      print_stat("g. L1 prefetches", to_string(PREFETCH_DISABLED));
  
      // L2 Cache Statistics
      cout << "----- L2 Cache -----" << endl;
      print_stat("h. L2 reads (demand)", to_string(l2_cache.reads));
      print_stat("i. L2 read misses (demand)", to_string(l2_cache.read_misses));
      print_stat("j. L2 reads (prefetch)", to_string(PREFETCH_DISABLED));
      print_stat("k. L2 read misses (prefetch)", to_string(PREFETCH_DISABLED));
      print_stat("l. L2 writes", to_string(l2_cache.writes));
      print_stat("m. L2 write misses", to_string(l2_cache.write_misses));
      print_stat("n. L2 miss rate", format_rate(l2_cache.read_misses, l2_cache.reads));
      print_stat("o. L2 writebacks", to_string(l2_cache.writebacks));
      print_stat("p. L2 prefetches", to_string(PREFETCH_DISABLED));
  
      // Memory Traffic
      cout << "----- Memory -----" << endl;
      print_stat("q. Memory traffic", to_string(mem_traffic));
  }
};

// Main function
int main(int argc, char *argv[]) {
    if (argc != 9) {
        cerr << "Error: Expected 8 command-line arguments but was provided " << (argc - 1) << ".\n";
        exit(EXIT_FAILURE);
    }

    // Parse arguments
    uint32_t block_size = atoi(argv[1]);
    uint32_t l1_size = atoi(argv[2]);
    uint32_t l1_assoc = atoi(argv[3]);
    uint32_t l2_size = atoi(argv[4]);
    uint32_t l2_assoc = atoi(argv[5]);
    uint32_t pref_n = atoi(argv[6]);
    uint32_t pref_m = atoi(argv[7]);
    char *trace_file = argv[8];

    // Initialize simulator
    Simulator simulator(block_size, l1_size, l1_assoc, l2_size, l2_assoc);

    // Open trace file
    FILE *fp = fopen(trace_file, "r");
    if (!fp) {
        cerr << "Error: Unable to open file " << trace_file << endl;
        exit(EXIT_FAILURE);
    }

    // Process trace file
    char rw;
    uint32_t addr;
    while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {
        simulator.access(rw, addr);
    }

    // Print results
    simulator.l1_cache.print_contents("L1");
    simulator.l2_cache.print_contents("L2");
    simulator.print_stats();

    fclose(fp);
    return 0;
}
