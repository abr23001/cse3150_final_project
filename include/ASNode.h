#pragma once
#include <vector>
#include <memory>
#include <set>
#include "Policy.h"
class ASNode;
using ASNodePtr = std::shared_ptr<ASNode>;

class ASNode {
public:
    int asn;
    std::set<int> providers;
    std::set<int> customers;
    std::set<int> peers;
    int propagationRank;
    std::unique_ptr<Policy> policy;

    ASNode(int asn) : asn(asn), propagationRank(-1) {}
};
