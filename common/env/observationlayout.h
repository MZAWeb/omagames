#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <cstddef>
#include <vector>

namespace OmaGames {

// How an env's observation sits in one flat buffer: named tensors, each with
// a dtype, a shape and a byte offset. A trainer reads the same layout from
// the env spec and lays zero-copy array views over the buffer, so this is
// the whole of the format.
//
// Every offset, and the total size, is a multiple of kAlignment: a batch is
// rows of `size()` bytes, and each row's tensors stay aligned for any dtype.
class ObservationLayout {
public:
    enum class DType { U8, I32, F32 };

    static constexpr int kAlignment = 8;

    // Appends a tensor and returns its index, which is what at() takes.
    // `labels` name the entries of its last axis (the columns of a table of
    // stats), so a trainer reads them by name rather than by position.
    int add(const QString &name, DType type, const std::vector<int> &shape, const QStringList &labels = {});

    int count() const { return int(m_tensors.size()); }
    int size() const { return m_size; }
    int elements(int tensor) const { return m_tensors[size_t(tensor)].elements; }

    // The tensor's first element inside an observation buffer.
    template <typename T>
    T *at(std::byte *buffer, int tensor) const {
        Q_ASSERT(sizeof(T) == bytesPer(m_tensors[size_t(tensor)].type));
        return reinterpret_cast<T *>(buffer + m_tensors[size_t(tensor)].offset);
    }

    // {size, tensors: [{name, dtype, shape, offset, labels?}]}
    QJsonObject toJson() const;

    static size_t bytesPer(DType type);
    static QString dtypeName(DType type);

private:
    struct Tensor {
        QString name;
        DType type;
        std::vector<int> shape;
        QStringList labels;
        int offset;
        int elements;
    };

    std::vector<Tensor> m_tensors;
    int m_size = 0;
};

}  // namespace OmaGames
