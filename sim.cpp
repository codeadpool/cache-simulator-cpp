#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <bits/stdc++.h>
#include <sstream>
#include <unordered_map>
#include <cmath>

using namespace std;

const uint32_t ADDRESS_BITS = 32;

struct CacheBlock {
    bool valid;
    bool dirty;
    uint32_t tag;
    uint32_t rank;
};

class Cache {
public:
    uint32_t block_size;
    uint32_t size;
    uint32_t associativity;
    uint32_t sets;
    uint32_t index_bits;
    uint32_t block_offset_bits;
    uint32_t tag_bits;
    vector<vector<CacheBlock>> cache;
    vector<uint32_t> current_size;
    Cache* next_level;  // Pointer to next level cache

    // Statistics
    uint32_t reads = 0;
    uint32_t read_misses = 0;
    uint32_t writes = 0;
    uint32_t write_misses = 0;
    uint32_t writebacks = 0;

    Cache(uint32_t blk_size, uint32_t cache_size, uint32_t assoc, Cache* next = nullptr) 
        : block_size(blk_size), size(cache_size), associativity(assoc), next_level(next) {
        if (size == 0 || associativity == 0) return;

        sets = size / (associativity * block_size);
        index_bits = sets > 1 ? static_cast<uint32_t>(log2(sets)) : 0;
        block_offset_bits = static_cast<uint32_t>(log2(block_size));
        tag_bits = ADDRESS_BITS - index_bits - block_offset_bits;

        cache.resize(sets, vector<CacheBlock>(associativity));
        current_size.resize(sets, 0);

        for (uint32_t i = 0; i < sets; i++) {
            for (uint32_t j = 0; j < associativity; j++) {
                cache[i][j] = {false, false, 0, j};
            }
        }
    }

    void parse_address(uint32_t addr, uint32_t &tag, uint32_t &index) {
        index = (sets > 1) ? ((addr >> block_offset_bits) & (sets - 1)) : 0;
        tag = addr >> (block_offset_bits + index_bits);
    }

    int find_block(uint32_t index, uint32_t tag) {
        for (uint32_t i = 0; i < associativity; i++) {
            if (cache[index][i].valid && cache[index][i].tag == tag) {
                return i;
            }
        }
        return -1;
    }

    void update_lru(uint32_t index, uint32_t loc) {
        uint32_t prev_rank = cache[index][loc].rank;
        for (uint32_t i = 0; i < associativity; i++) {
            if (cache[index][i].valid && cache[index][i].rank < prev_rank) {
                cache[index][i].rank++;
            }
        }
        cache[index][loc].rank = 0;
    }

    uint32_t find_lru_block(uint32_t index) {
        uint32_t lru_loc = 0;
        uint32_t max_rank = 0;
        for (uint32_t i = 0; i < associativity; i++) {
            if (cache[index][i].valid && cache[index][i].rank > max_rank) {
                max_rank = cache[index][i].rank;
                lru_loc = i;
            }
        }
        return lru_loc;
    }

    bool read(uint32_t addr, uint32_t &mem_traffic) {
        if (size == 0) {
            mem_traffic++;
            return false;
        }
        reads++;
        uint32_t tag, index;
        parse_address(addr, tag, index);

        int loc = find_block(index, tag);
        if (loc != -1) {
            update_lru(index, loc);
            return true;
        } else {
            read_misses++;
            handle_miss(addr, index, tag, 'r', mem_traffic);
            return false;
        }
    }

    bool write(uint32_t addr, uint32_t &mem_traffic) {
        if (size == 0) {
            mem_traffic++;
            return false;
        }
        writes++;
        uint32_t tag, index;
        parse_address(addr, tag, index);

        int loc = find_block(index, tag);
        if (loc != -1) {
            cache[index][loc].dirty = true;
            update_lru(index, loc);
            return true;
        } else {
            write_misses++;
            handle_miss(addr, index, tag, 'w', mem_traffic);
            return false;
        }
    }
    
    void print_contents(const string &cache_name) {
        cout << "===== " << cache_name << " contents =====" << endl;
        for (uint32_t i = 0; i < sets; i++) {
            cout << "set " << dec << i << ":";
            map<uint32_t, pair<uint32_t, bool>> block_map;
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

private:
    void handle_miss(uint32_t addr, uint32_t index, uint32_t tag, char type, uint32_t &mem_traffic) {
        bool next_level_hit = false;
        if (next_level) {
            next_level_hit = next_level->read(addr, mem_traffic);
        } else {
            mem_traffic++;
            next_level_hit = false;
        }

        if (current_size[index] >= associativity) {
            uint32_t lru_loc = find_lru_block(index);
            CacheBlock &evicted_block = cache[index][lru_loc];
            if (evicted_block.dirty) {
                writebacks++;
                if (next_level) {
                    uint32_t evicted_addr = (evicted_block.tag << (index_bits + block_offset_bits)) 
                                          | (index << block_offset_bits);
                    next_level->write(evicted_addr, mem_traffic);
                } else {
                    mem_traffic++;
                }
            }
        }

        uint32_t new_loc = find_lru_block(index);
        cache[index][new_loc] = {true, (type == 'w'), tag, 0};
        if (current_size[index] < associativity) current_size[index]++;
        update_lru(index, new_loc);
    }
};

class Simulator {
public:
    Cache l1_cache;
    Cache l2_cache;
    Cache dummy_cache;
    uint32_t mem_traffic = 0;

    Simulator(uint32_t blk_size, uint32_t l1_size, uint32_t l1_assoc, 
             uint32_t l2_size, uint32_t l2_assoc)
        : dummy_cache(0, 0, 0, nullptr),
          l2_cache(blk_size, l2_size, l2_assoc, &dummy_cache),
          l1_cache(blk_size, l1_size, l1_assoc, l2_size > 0 ? &l2_cache : &dummy_cache) {}

    void access(char rw, uint32_t addr) {
        if (rw == 'r') {
            l1_cache.read(addr, mem_traffic);
        } else {
            l1_cache.write(addr, mem_traffic);
        }
    }

    void print_stats() {
        auto print_stat = [](const string &label, const string &value) {
            cout << left << setw(30) << label << ": " << value << endl;
        };

        auto format_rate = [](uint32_t num, uint32_t den) {
            if (den == 0) return string("0.0000");
            stringstream ss;
            ss << fixed << setprecision(4) << static_cast<double>(num)/den;
            return ss.str();
        };

        cout << "===== Measurements =====" << endl;
        cout << "----- L1 Cache -----" << endl;
        print_stat("a. L1 reads", to_string(l1_cache.reads));
        print_stat("b. L1 read misses", to_string(l1_cache.read_misses));
        print_stat("c. L1 writes", to_string(l1_cache.writes));
        print_stat("d. L1 write misses", to_string(l1_cache.write_misses));
        print_stat("e. L1 miss rate", format_rate(l1_cache.read_misses + l1_cache.write_misses, 
                                                l1_cache.reads + l1_cache.writes));
        print_stat("f. L1 writebacks", to_string(l1_cache.writebacks));

        cout << "----- L2 Cache -----" << endl;
        print_stat("h. L2 reads (demand)", to_string(l2_cache.reads));
        print_stat("i. L2 read misses (demand)", to_string(l2_cache.read_misses));
        print_stat("n. L2 miss rate", format_rate(l2_cache.read_misses, l2_cache.reads));
        print_stat("o. L2 writebacks", to_string(l2_cache.writebacks));

        cout << "----- Memory -----" << endl;
        print_stat("q. Memory traffic", to_string(mem_traffic));
    }
};

int main(int argc, char *argv[]) {
    if (argc != 9) {
        cerr << "Error: Expected 8 arguments\n";
        exit(EXIT_FAILURE);
    }

    uint32_t block_size = atoi(argv[1]);
    uint32_t l1_size = atoi(argv[2]);
    uint32_t l1_assoc = atoi(argv[3]);
    uint32_t l2_size = atoi(argv[4]);
    uint32_t l2_assoc = atoi(argv[5]);
    char *trace_file = argv[8];

    Simulator sim(block_size, l1_size, l1_assoc, l2_size, l2_assoc);

    FILE *fp = fopen(trace_file, "r");
    if (!fp) {
        cerr << "Error opening trace file\n";
        exit(EXIT_FAILURE);
    }

    char rw;
    uint32_t addr;
    while (fscanf(fp, "%c %x\n", &rw, &addr) == 2) {
        sim.access(rw, addr);
    }

    sim.l1_cache.print_contents("L1");
    if (sim.l2_cache.size > 0) {
        sim.l2_cache.print_contents("L2");
    }
    sim.print_stats();

    fclose(fp);
    return 0;
}
