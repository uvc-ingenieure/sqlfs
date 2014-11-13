/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 UVC Ingenieure http://uvc.de/
 * Author: Max Holtzberg <mh@uvc.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
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
