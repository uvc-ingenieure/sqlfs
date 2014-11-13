#ifndef SQLITEFILEENGINE_H
#define SQLITEFILEENGINE_H

#include <QSqlDatabase>
#include <QRegExp>

#include "QtCore/private/qabstractfileengine_p.h"


class SqlFileEngineHandler : public QAbstractFileEngineHandler
{
public:
     QAbstractFileEngine *create(const QString &fileName) const;
};

class SqlFileEngine : public QAbstractFileEngine
{
    friend class SqlFileEngineIterator;

public:
    explicit SqlFileEngine(const QString &fileName);
    ~SqlFileEngine();

    FileFlags fileFlags(FileFlags type) const;
    QString fileName(FileName file) const;
    Iterator *beginEntryList(QDir::Filters filters, const QStringList &filterNames);
    bool open(QIODevice::OpenMode openMode);
    bool mkdir(const QString &dirName, bool createParentDirectories) const;
    bool rmdir(const QString &dirName, bool recurseParentDirectories) const;
    bool remove();
    bool rename(const QString &newName);
    qint64 size() const;
    bool setSize(qint64 size);
    bool seek(qint64 pos);
    qint64 write(const char *data, qint64 len);
    qint64 read(char *data, qint64 maxlen);
    bool flush();
    bool close();

private:
    bool tableExists();
    bool loadFile();
    QStringList splitPath(const QString &path) const;
    void createTable(const QString &tableName, QSqlDatabase db) const;
    int node(const QString &path, QSqlDatabase db=QSqlDatabase()) const;

    QIODevice::OpenMode m_openMode;

    QRegExp m_urlRegExp;
    mutable QSqlDatabase m_db;
    QString m_absoluteFileName;
    QString m_tableName;
    QString m_filePath;
    QString m_path;
    QString m_fileName;
    int m_nodeId;

    QByteArray m_buffer;
    qint64 m_pos;

};

#endif // SQLITEFILEENGINE_H
