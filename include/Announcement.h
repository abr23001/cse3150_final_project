#pragma once
#include <string>
#include <vector>

enum class Relationship {
    ORIGIN = 0,      // Best - our own announcement
    CUSTOMER = 1,    // Good - makes money  
    PEER = 2,        // Neutral
    PROVIDER = 3     // Worst - costs money
};

class Announcement {
public:
    std::string prefix;
    std::vector<int> asPath;
    int nextHopASN;
    Relationship receivedFrom;
    bool rovInvalid;

    Announcement(const std::string& prefix, const std::vector<int>& asPath,
                int nextHopASN, Relationship receivedFrom, bool rovInvalid = false)
        : prefix(prefix), asPath(asPath), nextHopASN(nextHopASN), receivedFrom(receivedFrom), rovInvalid(rovInvalid) {}

    Announcement() : nextHopASN(0), receivedFrom(Relationship::ORIGIN), rovInvalid(false) {}

    // Ex2_-style propagation method
    Announcement createPropagated(int senderASN, Relationship rel) const;
    bool isBetterThan(const Announcement& other) const;
};
