#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstdlib>
#include <ctime>
#include <vector>

void generate_trace(int num_operations, const std::string& filename) {
    std::ofstream trace_file(filename);
    if (!trace_file.is_open()) {
        std::cerr << "Error opening file!" << std::endl;
        return;
    }

    // Set the random seed
    std::srand(std::time(0));

    // Helper function to generate a random address
    auto generate_address = []() -> std::string {
        // Generate a random 32-bit hex address
        unsigned int addr = std::rand() % 0xFFFFFFFF;
        std::stringstream ss;
        ss << "0x" << std::uppercase << std::hex << addr;
        return ss.str();
    };

    // Generate the trace file with operations
    for (int i = 0; i < num_operations; ++i) {
        std::string address = generate_address();
        // Alternate between read and write operations
        if (i % 2 == 0) {
            trace_file << "r " << address << std::endl;
        } else {
            trace_file << "w " << address << std::endl;
        }
    }

    trace_file.close();
    std::cout << "Trace file '" << filename << "' generated successfully!" << std::endl;
}

int main() {
    int num_operations;
    std::string filename;

    // Ask user for the number of operations and output file name
    std::cout << "Enter the number of operations: ";
    std::cin >> num_operations;

    std::cout << "Enter the output file name (e.g., trace.txt): ";
    std::cin >> filename;

    // Generate the trace file
    generate_trace(num_operations, filename);

    return 0;
}
