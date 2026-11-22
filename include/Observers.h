#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "Events.h"

// журнал сповіщень, що фіксує всі події у вигляді рядків
class AuditLogObserver : public IObserver
{
public:
    explicit AuditLogObserver(std::ostream* out = nullptr);

    void update(const NotificationEvent& event) override;
    const std::vector<std::string>& entries() const { return entries_; }

private:
    std::ostream* out_;
    std::vector<std::string> entries_;
};

//  лічильники створених, доставлених і невдалих
class DeliveryStatsObserver : public IObserver
{
public:
    void update(const NotificationEvent& event) override;

    int created() const { return created_; }
    int delivered() const { return delivered_; }
    int failed() const { return failed_; }

    double successRate() const;

private:
    int created_ = 0;
    int delivered_ = 0;
    int failed_ = 0;
};
