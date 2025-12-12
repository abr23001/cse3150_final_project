#include "ASGraph.h"
#include <iostream>
#include <cassert>

void testBasicFunctionality() {
    ASGraph graph;
    
    // Test provider-customer relationship (AS1 provides to AS2)
    graph.addRelationship(1, 2, 0);
    assert(graph.nodes[1]->customers.count(2) == 1);
    assert(graph.nodes[2]->providers.count(1) == 1);
    
    // Test customer-provider relationship (AS3 is customer of AS4)  
    graph.addRelationship(3, 4, -1);
    assert(graph.nodes[3]->providers.count(4) == 1);
    assert(graph.nodes[4]->customers.count(3) == 1);
    
    // Test peer relationship
    graph.addRelationship(5, 6, 1);
    assert(graph.nodes[5]->peers.count(6) == 1);
    assert(graph.nodes[6]->peers.count(5) == 1);
    
    std::cout << "Basic functionality tests passed!" << std::endl;
}

void testSimpleCycle() {
    ASGraph graph;
    
    // Create a simple cycle: 1 -> 2 -> 3 -> 1 (provider chain)
    graph.addRelationship(1, 2, 0); // 1 provides to 2
    graph.addRelationship(2, 3, 0); // 2 provides to 3  
    graph.addRelationship(3, 1, 0); // 3 provides to 1 (creates cycle)
    
    assert(graph.hasProviderCycle() == true);
    
    std::cout << "Cycle detection tests passed!" << std::endl;
}

int main() {
    testBasicFunctionality();
    testSimpleCycle();
    
    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}