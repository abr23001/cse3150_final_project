#include "ASGraph.h"
#include <iostream>

void createSimpleTestGraph() {
    ASGraph graph;
    
    // Create simple hierarchy: 1 -> 2 -> 3
    // AS1 provides to AS2, AS2 provides to AS3
    graph.addRelationship(1, 2, 0);  // 1 provides to 2
    graph.addRelationship(2, 3, 0);  // 2 provides to 3
    
    graph.flattenGraph();
    graph.initializeBGPPolicies();
    
    // Seed announcement from AS3
    graph.seedAnnouncement(3, "192.168.1.0/24");
    
    // Propagate announcements
    graph.propagateAnnouncements();
    
    // Output results
    graph.outputToCSV("simple_test.csv");
    
    std::cout << "Simple test completed. Results in simple_test.csv" << std::endl;
}

void createPeerTestGraph() {
    ASGraph graph;
    
    // Create graph with peers
    // AS1 peers with AS2, both provide to AS3
    graph.addRelationship(1, 2, 1);  // 1 peers with 2
    graph.addRelationship(1, 3, 0);  // 1 provides to 3
    graph.addRelationship(2, 3, 0);  // 2 provides to 3
    
    graph.flattenGraph();
    graph.initializeBGPPolicies();
    
    // Seed announcements from AS3 and AS2
    graph.seedAnnouncement(3, "10.0.1.0/24");
    graph.seedAnnouncement(2, "10.0.2.0/24");
    
    // Propagate announcements
    graph.propagateAnnouncements();
    
    // Output results
    graph.outputToCSV("peer_test.csv");
    
    std::cout << "Peer test completed. Results in peer_test.csv" << std::endl;
}

int main() {
    std::cout << "Running BGP tests..." << std::endl;
    
    createSimpleTestGraph();
    createPeerTestGraph();
    
    return 0;
}