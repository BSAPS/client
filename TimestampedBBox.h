#ifndef TIMESTAMPEDBBOX_H
#define TIMESTAMPEDBBOX_H

#include <QList>
#include "BBox.h"

// 타임스탬프가 있는 BBox 구조체
struct TimestampedBBox {
    QList<BBox> bboxes;
    qint64 timestamp;
};

#endif // TIMESTAMPEDBBOX_H
