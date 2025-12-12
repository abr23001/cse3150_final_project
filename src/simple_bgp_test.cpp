#include "ASGraph.h"
#include <iostream>

int main() {
    ASGraph graph;
    
    // Create simple linear hierarchy: AS1 <- AS2 <- AS3
    // (AS1 provides to AS2, AS2 provides to AS3)
    graph.addRelationship(1, 2, 0);  // 1 provides to 2
    graph.addRelationship(2, 3, 0);  // 2 provides to 3
    
    std::cout << "Graph topology created" << std::endl;
    
    graph.flattenGraph();
    std::cout << "Graph flattened, ranks: " << graph.propagationRanks.size() << std::endl;
    
    for (size_t i = 0; i < graph.propagationRanks.size(); i++) {
        std::cout << "Rank " << i << ": ";
        for (int asn : graph.propagationRanks[i]) {
            std::cout << asn << " ";
        }
        std::cout << std::endl;
    }
    
    graph.initializeBGPPolicies();
    std::cout << "BGP policies initialized" << std::endl;
    
    // Seed announcement from AS3 (should be at rank 0)
    graph.seedAnnouncement(3, "192.168.1.0/24");
    std::cout << "Announcement seeded from AS3" << std::endl;
    
    // Check AS3's RIB
    BGP* as3_bgp = dynamic_cast<BGP*>(graph.nodes[3]->policy.get());
    if (as3_bgp && !as3_bgp->localRIB.empty()) {
        auto& announcement = as3_bgp->localRIB.begin()->second;
        std::cout << "AS3 RIB: " << announcement.prefix << " AS-Path: ";
        for (int asn : announcement.asPath) {
            std::cout << asn << " ";
        }
        std::cout << std::endl;
    }
    
    // Propagate announcements
    std::cout << "Starting propagation..." << std::endl;
    graph.propagateAnnouncements();
    std::cout << "Propagation complete" << std::endl;
    
    // Check all RIBs
    for (int asn = 1; asn <= 3; asn++) {
        if (graph.nodes.find(asn) != graph.nodes.end()) {
            BGP* bgp = dynamic_cast<BGP*>(graph.nodes[asn]->policy.get());
            if (bgp) {
                std::cout << "AS" << asn << " RIB:" << std::endl;
                for (const auto& entry : bgp->localRIB) {
                    std::cout << "  " << entry.second.prefix << " AS-Path: ";
                    for (int path_asn : entry.second.asPath) {
                        std::cout << path_asn << " ";
                    }
                    std::cout << "NextHop: " << entry.second.nextHopASN << std::endl;
                }
            }
        }
    }
    
    graph.outputToCSV("debug_test.csv");
    
    return 0;
}