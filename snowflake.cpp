#include "snowflake.h"

#include <stdexcept>

/*
* \sa https://github.com/twitter-archive/snowflake Twitter snowflake
*/
namespace Constants {
const qint64 WorkerIdBits       = 5L;
const qint64 DatacenterIdBits   = 5L;
const qint64 SequenceBits       = 12L;
const qint64 MaxWorkerId        = -1L ^ (-1L << WorkerIdBits    );
const qint64 MaxDatacenterId    = -1L ^ (-1L << DatacenterIdBits);
const qint64 SequenceMask       = -1L ^ (-1L << SequenceBits    );
const qint64 WorkerIdShift      = SequenceBits;
const qint64 DatacenterIdShift  = SequenceBits + WorkerIdBits;
const qint64 TimestampLeftShift = SequenceBits + WorkerIdBits + DatacenterIdBits;
const qint64 Twepoch = 1288834974657L;
}

class Snowflake::Private
{
public:
    void initialize()
    {
        // sanity check for workerId
        if (workerId > Constants::MaxWorkerId || workerId < 0) {
            qFatal("worker Id can't be greater than %d or less than 0", Constants::MaxWorkerId);
        }

        if (datacenterId > Constants::MaxDatacenterId || datacenterId < 0) {
            qFatal("datacenter Id can't be greater than %d or less than 0", Constants::MaxDatacenterId);
        }

        qInfo("IdWorker starting. timestamp left shift %d, datacenter id bits %d, worker id bits %d, sequence bits %d, workerid %d",
            Constants::TimestampLeftShift,
            Constants::DatacenterIdBits,
            Constants::WorkerIdBits,
            Constants::SequenceBits,
            workerId);
    }

    qint64 timeGen()
    {
        return QDateTime::currentMSecsSinceEpoch();
    }

    qint64 tilNextMillis(qint64 value)
    {
        auto timestamp = timeGen();
        while (timestamp <= value) {
            timestamp = timeGen();
        }
        return timestamp;
    }

    qint64 nextId()
    {
        auto timestamp = timeGen();

        if (timestamp < lastTimestamp) {
            qCritical("clock is moving backwards. Rejecting requests until %d.", lastTimestamp);
            throw std::runtime_error(QString("Clock moved backwards.  Refusing to generate id for %1 milliseconds").arg(lastTimestamp - timestamp).toStdString());
        }

        if (lastTimestamp == timestamp) {
            sequence = (sequence + 1) & Constants::SequenceMask;
            if (sequence == 0) {
                timestamp = tilNextMillis(lastTimestamp);
            }
        } else {
            sequence = 0L;
        }

        lastTimestamp = timestamp;
        return ((timestamp - Constants::Twepoch)
            << Constants::TimestampLeftShift)
            | (datacenterId << Constants::DatacenterIdShift)
            | (workerId << Constants::WorkerIdShift)
            | sequence
            ;
    }

    qint64 workerId      = 0L;
    qint64 datacenterId  = 0L;
    qint64 sequence      = 0L;
    qint64 lastTimestamp = -1L;
    QMutex lock;
};

Snowflake::Snowflake(qint64 workerId, qint64 datacenterId, qint64 sequence)
    : d(new Private)
{
    d->workerId     = workerId;
    d->datacenterId = datacenterId;
    d->sequence     = sequence;
    d->initialize();
}

// synchronized
qint64 Snowflake::nextId()
{
    QMutexLocker locker(&d->lock);
    return d->nextId();
}

Snowflake::~Snowflake() = default;
