#include "ASGraph.h"
#include "Policy.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <functional>
#include <queue>
#include <algorithm>

ASNodePtr ASGraph::getOrCreateNode(int asn) {
    if (nodes.find(asn) == nodes.end()) {
        nodes[asn] = std::make_shared<ASNode>(asn);
    }
    return nodes[asn];
}

void ASGraph::addRelationship(int as1, int as2, int relationship) {
    ASNodePtr node1 = getOrCreateNode(as1);
    ASNodePtr node2 = getOrCreateNode(as2);

    if (relationship == -1) {
        // CAIDA standard: Provider-to-Customer (AS1 → AS2)
        // as1 is provider of as2
        node1->customers.insert(as2);
        node2->providers.insert(as1);
    } else if (relationship == 0) {
        // CAIDA standard: Peer-to-Peer
        node1->peers.insert(as2);
        node2->peers.insert(as1);
    } else if (relationship == 1) {
        // CAIDA standard: Sibling (usually ignored)
        // For now, treat as peer relationship
        node1->peers.insert(as2);
        node2->peers.insert(as1);
    }
}

bool ASGraph::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string as1_str, as2_str, rel_str, source;

        if (std::getline(iss, as1_str, '|') &&
            std::getline(iss, as2_str, '|') &&
            std::getline(iss, rel_str, '|') &&
            std::getline(iss, source)) {

            int as1 = std::stoi(as1_str);
            int as2 = std::stoi(as2_str);
            int relationship = std::stoi(rel_str);

            addRelationship(as1, as2, relationship);
        }
    }

    file.close();
    return true;
}

bool ASGraph::hasCycleDFS(int start, std::set<int>& visited, std::set<int>& recStack,
                          const std::function<const std::set<int>&(const ASNodePtr&)>& getNeighbors) {
    visited.insert(start);
    recStack.insert(start);

    if (nodes.find(start) != nodes.end()) {
        const std::set<int>& neighbors = getNeighbors(nodes[start]);
        for (int neighbor : neighbors) {
            if (recStack.find(neighbor) != recStack.end()) {
                return true;
            }
            if (visited.find(neighbor) == visited.end()) {
                if (hasCycleDFS(neighbor, visited, recStack, getNeighbors)) {
                    return true;
                }
            }
        }
    }

    recStack.erase(start);
    return false;
}

bool ASGraph::hasProviderCycle() {
    std::set<int> visited;
    std::set<int> recStack;

    auto getCustomers = [](const ASNodePtr& node) -> const std::set<int>& {
        return node->customers;
    };

    for (const auto& pair : nodes) {
        int asn = pair.first;
        if (visited.find(asn) == visited.end()) {
            if (hasCycleDFS(asn, visited, recStack, getCustomers)) {
                return true;
            }
        }
    }
    return false;
}

bool ASGraph::hasCustomerCycle() {
    std::set<int> visited;
    std::set<int> recStack;

    auto getProviders = [](const ASNodePtr& node) -> const std::set<int>& {
        return node->providers;
    };

    for (const auto& pair : nodes) {
        int asn = pair.first;
        if (visited.find(asn) == visited.end()) {
            if (hasCycleDFS(asn, visited, recStack, getProviders)) {
                return true;
            }
        }
    }
    return false;
}

void ASGraph::printStats() {
    int totalProviderLinks = 0;
    int totalCustomerLinks = 0;
    int totalPeerLinks = 0;

    for (const auto& pair : nodes) {
        const ASNodePtr& node = pair.second;
        totalProviderLinks += node->providers.size();
        totalCustomerLinks += node->customers.size();
        totalPeerLinks += node->peers.size();
    }

    std::cout << "AS Graph Statistics:" << std::endl;
    std::cout << "Total nodes: " << nodes.size() << std::endl;
    std::cout << "Provider links: " << totalProviderLinks << std::endl;
    std::cout << "Customer links: " << totalCustomerLinks << std::endl;
    std::cout << "Peer links: " << totalPeerLinks / 2 << " (bidirectional)" << std::endl;
}

// BGP Implementation

void ASGraph::flattenGraph() {
    propagationRanks.clear();
    
    // Initialize ranks to -1 (unassigned)
    std::unordered_map<int, int> rank;
    for (const auto& pair : nodes) {
        rank[pair.first] = -1;
        pair.second->propagationRank = -1;
    }
    
    // BFS queue for rank assignment (cycle-resistant)
    std::queue<int> queue;
    
    // Step 1: Find all ASes with no customers (rank 0)
    for (const auto& pair : nodes) {
        if (pair.second->customers.empty()) {
            rank[pair.first] = 0;
            pair.second->propagationRank = 0;
            queue.push(pair.first);
        }
    }
    
    // Step 2: BFS upward through providers (with cycle protection)
    std::set<int> inQueue;
    int maxIterations = nodes.size() * 3; // Reduced iteration limit
    int iterations = 0;
    
    while (!queue.empty() && iterations < maxIterations) {
        int currentASN = queue.front();
        queue.pop();
        inQueue.erase(currentASN);
        
        int currentRank = rank[currentASN];
        
        // For each provider, assign rank if it would be higher
        for (int providerASN : nodes[currentASN]->providers) {
            if (nodes.find(providerASN) != nodes.end()) {
                if (rank[providerASN] < currentRank + 1) {
                    rank[providerASN] = currentRank + 1;
                    nodes[providerASN]->propagationRank = currentRank + 1;
                    
                    // Only add to queue if not already in queue
                    if (inQueue.find(providerASN) == inQueue.end()) {
                        queue.push(providerASN);
                        inQueue.insert(providerASN);
                    }
                }
            }
        }
        iterations++;
    }
    
    // Step 3: Build propagation ranks vector
    int maxRank = -1;
    for (const auto& pair : rank) {
        maxRank = std::max(maxRank, pair.second);
    }
    
    if (maxRank >= 0) {
        propagationRanks.resize(maxRank + 1);
        for (const auto& pair : rank) {
            int asn = pair.first;
            int r = pair.second;
            if (r >= 0) {
                propagationRanks[r].push_back(asn);
            }
        }
    }
}

void ASGraph::initializeBGPPolicies() {
    for (auto& pair : nodes) {
        pair.second->policy = std::make_unique<BGP>();
    }
}

void ASGraph::initializeBGPPolicies(const std::set<int>& rovASNs) {
    for (auto& pair : nodes) {
        int asn = pair.first;
        if (rovASNs.find(asn) != rovASNs.end()) {
            pair.second->policy = std::make_unique<ROV>();
        } else {
            pair.second->policy = std::make_unique<BGP>();
        }
    }
}

void ASGraph::seedAnnouncement(int asn, const std::string& prefix) {
    if (nodes.find(asn) != nodes.end() && nodes[asn]->policy) {
        BGP* bgp = dynamic_cast<BGP*>(nodes[asn]->policy.get());
        if (bgp) {
            bgp->seedAnnouncement(prefix, asn);
        }
    }
}

void ASGraph::propagateAnnouncements() {
    propagateUpward();
    propagateAcross();
    propagateDownward();
}

void ASGraph::propagateUpward() {
    for (int rank = 0; rank < propagationRanks.size(); rank++) {
        // Send announcements to providers
        for (int asn : propagationRanks[rank]) {
            if (nodes.find(asn) == nodes.end()) continue;

            BGP* senderBGP = dynamic_cast<BGP*>(nodes[asn]->policy.get());
            if (!senderBGP) continue;

            std::vector<Announcement> announcementsToSend = senderBGP->getAnnouncementsToSend();

            for (int provider : nodes[asn]->providers) {
                if (nodes.find(provider) == nodes.end()) continue;

                BGP* receiverBGP = dynamic_cast<BGP*>(nodes[provider]->policy.get());
                if (!receiverBGP) continue;

                for (auto announcement : announcementsToSend) {
                    // Don't send back to the AS we received it from
                    if (announcement.nextHopASN == provider) continue;

                    Announcement propagated = announcement.createPropagated(asn, Relationship::CUSTOMER);
                    receiverBGP->addToReceivedQueue(propagated.prefix, propagated);
                }
            }
        }

        // Process received announcements for receivers (our working approach)
        for (int asn : propagationRanks[rank]) {
            if (nodes.find(asn) == nodes.end()) continue;

            for (int provider : nodes[asn]->providers) {
                if (nodes.find(provider) != nodes.end()) {
                    BGP* providerBGP = dynamic_cast<BGP*>(nodes[provider]->policy.get());
                    if (providerBGP) {
                        providerBGP->processAnnouncements(provider);
                    }
                }
            }
        }
    }
}

void ASGraph::propagateAcross() {
    // Send to peers
    for (const auto& pair : nodes) {
        int asn = pair.first;
        BGP* senderBGP = dynamic_cast<BGP*>(pair.second->policy.get());
        if (!senderBGP) continue;

        std::vector<Announcement> announcementsToSend = senderBGP->getAnnouncementsToSend();

        for (int peer : pair.second->peers) {
            if (nodes.find(peer) == nodes.end()) continue;

            BGP* receiverBGP = dynamic_cast<BGP*>(nodes[peer]->policy.get());
            if (!receiverBGP) continue;

            for (auto announcement : announcementsToSend) {
                // Don't send back to the AS we received it from
                if (announcement.nextHopASN == peer) continue;

                Announcement propagated = announcement.createPropagated(asn, Relationship::PEER);
                receiverBGP->addToReceivedQueue(propagated.prefix, propagated);
            }
        }
    }

    // Process received announcements
    for (const auto& pair : nodes) {
        int asn = pair.first;
        BGP* bgp = dynamic_cast<BGP*>(pair.second->policy.get());
        if (bgp) {
            bgp->processAnnouncements(asn);
        }
    }
}

void ASGraph::propagateDownward() {
    for (int rank = propagationRanks.size() - 1; rank >= 0; rank--) {
        // Send announcements to customers
        for (int asn : propagationRanks[rank]) {
            if (nodes.find(asn) == nodes.end()) continue;

            BGP* senderBGP = dynamic_cast<BGP*>(nodes[asn]->policy.get());
            if (!senderBGP) continue;

            std::vector<Announcement> announcementsToSend = senderBGP->getAnnouncementsToSend();

            for (int customer : nodes[asn]->customers) {
                if (nodes.find(customer) == nodes.end()) continue;

                BGP* receiverBGP = dynamic_cast<BGP*>(nodes[customer]->policy.get());
                if (!receiverBGP) continue;

                for (auto announcement : announcementsToSend) {
                    // Don't send back to the AS we received it from
                    if (announcement.nextHopASN == customer) continue;

                    Announcement propagated = announcement.createPropagated(asn, Relationship::PROVIDER);
                    receiverBGP->addToReceivedQueue(propagated.prefix, propagated);
                }
            }
        }

        // Process received announcements for receivers (our working approach)  
        for (int asn : propagationRanks[rank]) {
            if (nodes.find(asn) == nodes.end()) continue;

            for (int customer : nodes[asn]->customers) {
                if (nodes.find(customer) != nodes.end()) {
                    BGP* customerBGP = dynamic_cast<BGP*>(nodes[customer]->policy.get());
                    if (customerBGP) {
                        customerBGP->processAnnouncements(customer);
                    }
                }
            }
        }
    }
}

void ASGraph::outputToCSV(const std::string& filename) {
    std::ofstream file(filename);
    file << "asn,prefix,as_path" << std::endl;

    for (const auto& pair : nodes) {
        int asn = pair.first;
        BGP* bgp = dynamic_cast<BGP*>(pair.second->policy.get());
        if (!bgp) continue;

        for (const auto& ribEntry : bgp->localRIB) {
            const std::string& prefix = ribEntry.first;
            const Announcement& announcement = ribEntry.second;

            file << asn << "," << prefix << ",\"(";
            for (size_t i = 0; i < announcement.asPath.size(); i++) {
                if (i > 0) file << ", ";
                file << announcement.asPath[i];
            }
            if (announcement.asPath.size() == 1) file << ",";
            file << ")\"" << std::endl;
        }
    }

    file.close();
}

bool ASGraph::loadAnnouncementsFromCSV(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening announcements file: " << filename << std::endl;
        return false;
    }

    std::string line;
    bool firstLine = true;

    while (std::getline(file, line)) {
        if (firstLine) {
            firstLine = false;
            continue; // Skip header
        }

        if (line.empty()) continue;

        // Remove carriage return if present (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::istringstream iss(line);
        std::string asnStr, prefix, rovInvalidStr;

        if (std::getline(iss, asnStr, ',') &&
            std::getline(iss, prefix, ',') &&
            std::getline(iss, rovInvalidStr)) {

            // Remove any trailing whitespace/carriage returns
            if (!rovInvalidStr.empty() && rovInvalidStr.back() == '\r') {
                rovInvalidStr.pop_back();
            }

            int asn = std::stoi(asnStr);
            bool rovInvalid = (rovInvalidStr == "true" || rovInvalidStr == "True" ||
                              rovInvalidStr == "false" || rovInvalidStr == "False") ?
                              (rovInvalidStr == "true" || rovInvalidStr == "True") :
                              (rovInvalidStr == "1");

            // Ensure the AS exists in our graph
            if (nodes.find(asn) != nodes.end() && nodes[asn]->policy) {
                BGP* bgp = dynamic_cast<BGP*>(nodes[asn]->policy.get());
                if (bgp) {
                    bgp->seedAnnouncement(prefix, asn, rovInvalid);
                }
            }
        }
    }

    file.close();
    return true;
}

bool ASGraph::loadROVASNs(const std::string& filename, std::set<int>& rovASNs) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening ROV ASNs file: " << filename << std::endl;
        return false;
    }

    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // Remove carriage return if present (Windows line endings)
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        try {
            int asn = std::stoi(line);
            rovASNs.insert(asn);
        } catch (const std::exception& e) {
            // Skip malformed lines
        }
    }

    file.close();
    return true;
}
