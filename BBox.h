#ifndef BBOX_H
#define BBOX_H

#include <QString>
#include <QRect>
#include <QJsonArray>
#include <QJsonObject>
#include <QList>

struct BBox {
    int object_id;
    QString type;
    float confidence;
    QRect rect;
};

// 타임스탬프가 있는 BBox 구조체
struct TimestampedBBox {
    QList<BBox> bboxes;
    qint64 timestamp;
};

QList<BBox> parseBBoxes(const QJsonArray& arr);

#endif // BBOX_H
