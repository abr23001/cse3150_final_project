#pragma once
#include "ASNode.h"
#include <unordered_map>
#include <string>
#include <functional>

class ASGraph {
public:
    std::unordered_map<int, ASNodePtr> nodes;
    std::vector<std::vector<int>> propagationRanks;

    ASNodePtr getOrCreateNode(int asn);
    void addRelationship(int as1, int as2, int relationship);
    bool loadFromFile(const std::string& filename);
    bool hasProviderCycle();
    bool hasCustomerCycle();
    void printStats();

    // BGP functionality
    void flattenGraph();
    void initializeBGPPolicies();
    void initializeBGPPolicies(const std::set<int>& rovASNs);
    void seedAnnouncement(int asn, const std::string& prefix);
    void propagateAnnouncements();
    void outputToCSV(const std::string& filename);

    // ROV functionality
    bool loadAnnouncementsFromCSV(const std::string& filename);
    bool loadROVASNs(const std::string& filename, std::set<int>& rovASNs);

private:
    bool hasCycleDFS(int start, std::set<int>& visited, std::set<int>& recStack,
                     const std::function<const std::set<int>&(const ASNodePtr&)>& getNeighbors);

    void propagateUpward();
    void propagateAcross();
    void propagateDownward();
};
