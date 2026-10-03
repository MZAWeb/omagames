#include "observationlayout.h"

#include <QJsonArray>

namespace OmaGames {

namespace {

int aligned(int bytes) {
    const int a = ObservationLayout::kAlignment;
    return (bytes + a - 1) / a * a;
}

}  // namespace

int ObservationLayout::add(const QString &name, DType type, const std::vector<int> &shape,
                           const QStringList &labels) {
    Q_ASSERT(labels.isEmpty() || (!shape.empty() && labels.size() == shape.back()));
    int elements = 1;
    for (int extent : shape) {
        Q_ASSERT(extent > 0);
        elements *= extent;
    }
    const int offset = m_size;
    m_tensors.push_back({name, type, shape, labels, offset, elements});
    m_size = aligned(offset + elements * int(bytesPer(type)));
    return count() - 1;
}

QJsonObject ObservationLayout::toJson() const {
    QJsonArray tensors;
    for (const Tensor &tensor : m_tensors) {
        QJsonArray shape;
        for (int extent : tensor.shape)
            shape.append(extent);
        QJsonObject json{
            {QStringLiteral("name"), tensor.name},
            {QStringLiteral("dtype"), dtypeName(tensor.type)},
            {QStringLiteral("shape"), shape},
            {QStringLiteral("offset"), tensor.offset},
        };
        if (!tensor.labels.isEmpty())
            json.insert(QStringLiteral("labels"), QJsonArray::fromStringList(tensor.labels));
        tensors.append(json);
    }
    return {{QStringLiteral("size"), m_size}, {QStringLiteral("tensors"), tensors}};
}

size_t ObservationLayout::bytesPer(DType type) {
    switch (type) {
    case DType::U8:
        return 1;
    case DType::I32:
    case DType::F32:
        return 4;
    }
    return 1;
}

// numpy's names, so a trainer can hand them straight to np.dtype().
QString ObservationLayout::dtypeName(DType type) {
    switch (type) {
    case DType::U8:
        return QStringLiteral("uint8");
    case DType::I32:
        return QStringLiteral("int32");
    case DType::F32:
        return QStringLiteral("float32");
    }
    return {};
}

}  // namespace OmaGames
