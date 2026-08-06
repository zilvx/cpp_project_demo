#ifndef I_DATA_PROVIDER_H
#define I_DATA_PROVIDER_H

#include <QObject>
#include "../core/TableData.h"

class IDataProvider : public QObject {
    Q_OBJECT

public:
    explicit IDataProvider(QObject *parent = nullptr) : QObject(parent) {}

    virtual void fetchData() = 0;

signals:
    void dataReady(const TableData &data);
    void errorOccurred(const QString &message);
};

#endif
