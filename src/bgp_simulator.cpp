#include "ASGraph.h"
#include <iostream>
#include <chrono>
#include <cstring>
#include <set>

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " --relationships <file> --announcements <file> --rov-asns <file>" << std::endl;
    std::cerr << "  --relationships: CAIDA AS relationship file" << std::endl;
    std::cerr << "  --announcements: CSV file with announcements (asn,prefix,rov_invalid)" << std::endl;
    std::cerr << "  --rov-asns: CSV file with ROV-enabled ASNs" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string relationshipsFile;
    std::string announcementsFile;
    std::string rovASNsFile;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--relationships") == 0 && i + 1 < argc) {
            relationshipsFile = argv[++i];
        } else if (strcmp(argv[i], "--announcements") == 0 && i + 1 < argc) {
            announcementsFile = argv[++i];
        } else if (strcmp(argv[i], "--rov-asns") == 0 && i + 1 < argc) {
            rovASNsFile = argv[++i];
        } else {
            std::cerr << "Unknown argument: " << argv[i] << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }

    // Check required arguments
    if (relationshipsFile.empty() || announcementsFile.empty() || rovASNsFile.empty()) {
        std::cerr << "Error: All three arguments are required." << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    ASGraph graph;

    // Load relationships
    std::cout << "Loading AS relationships from: " << relationshipsFile << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    if (!graph.loadFromFile(relationshipsFile)) {
        std::cerr << "Failed to load relationships file" << std::endl;
        return 1;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Loaded " << graph.nodes.size() << " nodes in " << duration.count() << "ms" << std::endl;

    // Check for cycles
    std::cout << "Checking for cycles in AS relationships..." << std::endl;
    if (graph.hasProviderCycle()) {
        std::cerr << "ERROR: Provider cycle detected in AS relationships!" << std::endl;
        return 1;
    }
    
    if (graph.hasCustomerCycle()) {
        std::cerr << "ERROR: Customer cycle detected in AS relationships!" << std::endl;
        return 1;
    }
    
    std::cout << "No cycles detected. Topology is valid." << std::endl;

    // Flatten graph for propagation
    std::cout << "Flattening graph for BGP propagation..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    graph.flattenGraph();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Graph flattened into " << graph.propagationRanks.size() << " ranks in " << duration.count() << "ms" << std::endl;

    // Load ROV ASNs
    std::cout << "Loading ROV-enabled ASNs from: " << rovASNsFile << std::endl;
    std::set<int> rovASNs;
    if (!graph.loadROVASNs(rovASNsFile, rovASNs)) {
        std::cerr << "Failed to load ROV ASNs file" << std::endl;
        return 1;
    }
    std::cout << "Loaded " << rovASNs.size() << " ROV-enabled ASNs" << std::endl;

    // Initialize BGP policies (some with ROV)
    std::cout << "Initializing BGP policies..." << std::endl;
    graph.initializeBGPPolicies(rovASNs);

    // Load and seed announcements
    std::cout << "Loading announcements from: " << announcementsFile << std::endl;
    if (!graph.loadAnnouncementsFromCSV(announcementsFile)) {
        std::cerr << "Failed to load announcements file" << std::endl;
        return 1;
    }

    // Count seeded announcements
    int totalSeededAnnouncements = 0;
    for (const auto& pair : graph.nodes) {
        BGP* bgp = dynamic_cast<BGP*>(pair.second->policy.get());
        if (bgp) {
            totalSeededAnnouncements += bgp->localRIB.size();
        }
    }
    std::cout << "Seeded " << totalSeededAnnouncements << " announcements" << std::endl;

    // Propagate announcements
    std::cout << "Propagating BGP announcements..." << std::endl;
    start = std::chrono::high_resolution_clock::now();
    graph.propagateAnnouncements();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "BGP propagation completed in " << duration.count() << "ms" << std::endl;

    // Output results
    std::string outputFile = "ribs.csv";
    std::cout << "Writing results to: " << outputFile << std::endl;
    graph.outputToCSV(outputFile);

    // Summary statistics
    int totalRoutes = 0;
    for (const auto& pair : graph.nodes) {
        BGP* bgp = dynamic_cast<BGP*>(pair.second->policy.get());
        if (bgp) {
            totalRoutes += bgp->localRIB.size();
        }
    }

    std::cout << "Simulation complete!" << std::endl;
    std::cout << "Total routes in all RIBs: " << totalRoutes << std::endl;

    return 0;
}
