#pragma once
#include "Announcement.h"
#include <unordered_map>
#include <vector>

class Policy {
public:
    virtual ~Policy() = default;
    virtual void processAnnouncements(int currentASN) = 0;
    virtual void addToReceivedQueue(const std::string& prefix, const Announcement& announcement) = 0;
    virtual std::vector<Announcement> getAnnouncementsToSend() = 0;
};

class BGP : public Policy {
public:
    std::unordered_map<std::string, Announcement> localRIB;
    std::unordered_map<std::string, std::vector<Announcement>> receivedQueue;

    void processAnnouncements(int currentASN) override;
    void addToReceivedQueue(const std::string& prefix, const Announcement& announcement) override;
    std::vector<Announcement> getAnnouncementsToSend() override;
    void seedAnnouncement(const std::string& prefix, int originASN);
    virtual void seedAnnouncement(const std::string& prefix, int originASN, bool rovInvalid);

protected:
    Announcement selectBestAnnouncement(const std::vector<Announcement>& announcements);
    bool isBetterThan(const Announcement& candidate, const Announcement& existing);
};

class ROV : public BGP {
public:
    void addToReceivedQueue(const std::string& prefix, const Announcement& announcement) override;
    void seedAnnouncement(const std::string& prefix, int originASN, bool rovInvalid) override;
};
