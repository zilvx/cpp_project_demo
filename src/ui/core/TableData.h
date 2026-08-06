#ifndef TABLE_DATA_H
#define TABLE_DATA_H

#include <QString>
#include <QStringList>
#include <QVector>

#ifdef USE_PROTO_DATA
#include "table_data.pb.h"
#endif

struct TableData {
    QStringList          columns;
    QVector<QStringList> rows;
    QVector<bool>        editable;

    bool isValid() const { return !columns.isEmpty(); }
    int  rowCount() const { return rows.size(); }
    int  columnCount() const { return columns.size(); }

#ifdef USE_PROTO_DATA
    static TableData fromProto(const table_data::TableData &msg) {
        TableData r;
        for (int i = 0; i < msg.columns_size(); ++i)
            r.columns.append(QString::fromStdString(msg.columns(i)));
        for (int i = 0; i < msg.editable_size(); ++i)
            r.editable.append(msg.editable(i));
        for (int i = 0; i < msg.rows_size(); ++i) {
            QStringList cells;
            for (int j = 0; j < msg.rows(i).cells_size(); ++j)
                cells.append(QString::fromStdString(msg.rows(i).cells(j)));
            r.rows.append(cells);
        }
        return r;
    }
#endif
};

#endif
