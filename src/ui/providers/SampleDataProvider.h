#ifndef SAMPLE_DATA_PROVIDER_H
#define SAMPLE_DATA_PROVIDER_H

#include "IDataProvider.h"

class SampleDataProvider : public IDataProvider {
    Q_OBJECT
public:
    explicit SampleDataProvider(int rowCount = 20, QObject *parent = nullptr);
    void fetchData() override;
private:
    int m_rowCount;
};

#endif
