#include "ASGraph.h"
#include <iostream>
#include <chrono>

int main(int argc, char* argv[]) {
    std::string filename = "prefix/CAIDAASGraphCollector_2025.10.15.txt";
    if (argc > 1) {
        filename = argv[1];
    }

    ASGraph graph;

    std::cout << "Loading CAIDA dataset: " << filename << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    if (!graph.loadFromFile(filename)) {
        std::cerr << "Failed to load dataset" << std::endl;
        return 1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Loaded in " << duration.count() << "ms" << std::endl;

    graph.printStats();

    std::cout << "\nChecking for cycles..." << std::endl;

    start = std::chrono::high_resolution_clock::now();
    bool providerCycle = graph.hasProviderCycle();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Provider cycle check: " << (providerCycle ? "FOUND" : "NONE")
              << " (" << duration.count() << "ms)" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    bool customerCycle = graph.hasCustomerCycle();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Customer cycle check: " << (customerCycle ? "FOUND" : "NONE")
              << " (" << duration.count() << "ms)" << std::endl;

    return 0;
}
