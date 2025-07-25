#include "BBox.h"

QList<BBox> parseBBoxes(const QJsonArray& arr) {
    QList<BBox> result;
    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        BBox box;
        box.object_id = obj["id"].toInt();
        box.type = obj["type"].toString();
        box.confidence = obj["confidence"].toDouble();
        box.rect = QRect(
            obj["x"].toInt(),
            obj["y"].toInt(),
            obj["width"].toInt(),
            obj["height"].toInt()
            );
        result.append(box);
    }
    return result;
}
