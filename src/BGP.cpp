#include "Policy.h"
#include <algorithm>

void BGP::processAnnouncements(int currentASN) {
    for (auto& entry : receivedQueue) {
        const std::string& prefix = entry.first;
        std::vector<Announcement>& announcements = entry.second;

        if (!announcements.empty()) {
            // Create candidates with prepended AS paths for accurate comparison
            std::vector<Announcement> candidates;
            for (const auto& ann : announcements) {
                Announcement candidate = ann;
                candidate.asPath.insert(candidate.asPath.begin(), currentASN);
                candidates.push_back(candidate);
            }
            
            // Select best among prepended candidates
            Announcement best = selectBestAnnouncement(candidates);
            
            // Compare with existing route if any
            auto it = localRIB.find(prefix);
            if (it == localRIB.end() || isBetterThan(best, it->second)) {
                localRIB[prefix] = best;
            }
        }
    }

    receivedQueue.clear();
}

void BGP::addToReceivedQueue(const std::string& prefix, const Announcement& announcement) {
    receivedQueue[prefix].push_back(announcement);
}

std::vector<Announcement> BGP::getAnnouncementsToSend() {
    std::vector<Announcement> announcements;
    for (const auto& entry : localRIB) {
        announcements.push_back(entry.second);
    }
    return announcements;
}

void BGP::seedAnnouncement(const std::string& prefix, int originASN) {
    seedAnnouncement(prefix, originASN, false);
}

void BGP::seedAnnouncement(const std::string& prefix, int originASN, bool rovInvalid) {
    Announcement announcement;
    announcement.prefix = prefix;
    announcement.asPath = {originASN};
    announcement.nextHopASN = originASN;
    announcement.receivedFrom = Relationship::ORIGIN;
    announcement.rovInvalid = rovInvalid;

    localRIB[prefix] = announcement;
}

Announcement BGP::selectBestAnnouncement(const std::vector<Announcement>& announcements) {
    if (announcements.empty()) {
        return Announcement();
    }

    if (announcements.size() == 1) {
        return announcements[0];
    }

    // Sort by BGP decision process:
    // 1. Best relationship (customer > peer > provider)
    // 2. Shortest AS-Path
    // 3. Lowest next hop ASN

    auto best = announcements[0];

    for (const auto& announcement : announcements) {
        // Compare relationship preference
        int bestRelPref = (best.receivedFrom == Relationship::CUSTOMER) ? 3 :
                         (best.receivedFrom == Relationship::PEER) ? 2 : 1;
        int currRelPref = (announcement.receivedFrom == Relationship::CUSTOMER) ? 3 :
                         (announcement.receivedFrom == Relationship::PEER) ? 2 : 1;

        if (currRelPref > bestRelPref) {
            best = announcement;
        } else if (currRelPref == bestRelPref) {
            // Compare AS-Path length
            if (announcement.asPath.size() < best.asPath.size()) {
                best = announcement;
            } else if (announcement.asPath.size() == best.asPath.size()) {
                // Compare next hop ASN
                if (announcement.nextHopASN < best.nextHopASN) {
                    best = announcement;
                }
            }
        }
    }

    return best;
}

bool BGP::isBetterThan(const Announcement& candidate, const Announcement& existing) {
    // Compare relationship preference
    int candidateRelPref = (candidate.receivedFrom == Relationship::CUSTOMER) ? 3 :
                          (candidate.receivedFrom == Relationship::PEER) ? 2 : 1;
    int existingRelPref = (existing.receivedFrom == Relationship::CUSTOMER) ? 3 :
                         (existing.receivedFrom == Relationship::PEER) ? 2 : 1;

    if (candidateRelPref > existingRelPref) {
        return true;
    } else if (candidateRelPref == existingRelPref) {
        // Compare AS-Path length
        if (candidate.asPath.size() < existing.asPath.size()) {
            return true;
        } else if (candidate.asPath.size() == existing.asPath.size()) {
            // Compare next hop ASN
            return candidate.nextHopASN < existing.nextHopASN;
        }
    }
    return false;
}

// ROV Implementation
void ROV::addToReceivedQueue(const std::string& prefix, const Announcement& announcement) {
    // Drop announcements with ROV invalid flag set to true
    if (announcement.rovInvalid) {
        return; // Drop the announcement
    }

    // Otherwise, use the standard BGP behavior
    BGP::addToReceivedQueue(prefix, announcement);
}

void ROV::seedAnnouncement(const std::string& prefix, int originASN, bool rovInvalid) {
    // ROV-enabled ASNs should not seed invalid announcements
    if (rovInvalid) {
        return; // Drop the announcement
    }

    // Otherwise, use the standard BGP behavior
    BGP::seedAnnouncement(prefix, originASN, rovInvalid);
}
