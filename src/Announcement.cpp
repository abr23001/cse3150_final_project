#include "Announcement.h"

Announcement Announcement::createPropagated(int senderASN, Relationship rel) const {
    Announcement newAnn(*this);
    newAnn.nextHopASN = senderASN;
    newAnn.receivedFrom = rel;
    return newAnn;
}

bool Announcement::isBetterThan(const Announcement& other) const {
    //  BGP decision process: lower relationship values are better
    if (receivedFrom != other.receivedFrom) {
        return receivedFrom < other.receivedFrom;
    }

    // Shorter AS path is better
    if (asPath.size() != other.asPath.size()) {
        return asPath.size() < other.asPath.size();
    }

    // Lower next-hop ASN is better (tiebreaker)
    return nextHopASN < other.nextHopASN;
}
