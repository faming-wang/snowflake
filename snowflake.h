#pragma once

#include <memory>
#include <QtCore/qglobal.h>

class Snowflake
{
public:
    Snowflake(qint64 workerId, qint64 datacenterId, qint64 sequence = 0L);
    ~Snowflake();

    qint64 nextId(); // Note: This function is thread-safe.

private:
    class Private;
    std::unique_ptr<Private> d;
};
