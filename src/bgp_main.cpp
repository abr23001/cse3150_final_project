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
    
    std::cout << "\nFlattening graph..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    graph.flattenGraph();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Graph flattened in " << duration.count() << "ms" << std::endl;
    std::cout << "Propagation ranks: " << graph.propagationRanks.size() << std::endl;
    
    std::cout << "\nInitializing BGP policies..." << std::endl;
    graph.initializeBGPPolicies();
    
    std::cout << "\nSeeding announcements..." << std::endl;
    // Seed a few test announcements from different ASes
    graph.seedAnnouncement(1, "1.2.0.0/16");
    graph.seedAnnouncement(2, "2.3.0.0/16"); 
    graph.seedAnnouncement(15169, "8.8.8.0/24"); // Google's AS if it exists
    
    std::cout << "\nPropagating announcements..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    graph.propagateAnnouncements();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Propagation completed in " << duration.count() << "ms" << std::endl;
    
    std::cout << "\nOutputting results to caida_bgp_results.csv..." << std::endl;
    graph.outputToCSV("caida_bgp_results.csv");
    
    // Show some sample results
    std::cout << "\nSample results:" << std::endl;
    int count = 0;
    for (const auto& pair : graph.nodes) {
        if (count >= 10) break;
        
        int asn = pair.first;
        BGP* bgp = dynamic_cast<BGP*>(pair.second->policy.get());
        if (bgp && !bgp->localRIB.empty()) {
            std::cout << "AS" << asn << " has " << bgp->localRIB.size() << " routes" << std::endl;
            count++;
        }
    }
    
    return 0;
}