#pragma once
#include <string>
#include <vector>

enum class Relationship {
    CUSTOMER,
    PEER,
    PROVIDER,
    ORIGIN
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
};
