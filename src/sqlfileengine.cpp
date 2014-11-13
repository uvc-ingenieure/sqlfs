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

#include <QtCore>
#include <QtSql>

#include "sqlfileengine.h"


class SqlFileEngineIterator : public QAbstractFileEngineIterator
{
public:
    SqlFileEngineIterator(SqlFileEngine *engine, QDir::Filters filters,
                          const QStringList &nameFilters) :
        QAbstractFileEngineIterator(filters, nameFilters),
        m_index(-1)
    {
        QSqlQuery qry(engine->m_db);
        qry.prepare(QString("SELECT name "
                         "FROM %1 "
                         "WHERE parent=:parent")
                 .arg(engine->m_tableName));
        qry.bindValue(":parent", engine->m_nodeId);
        qry.exec();

        while (qry.next()) {
            QString path = qry.value(0).toString();
            m_list.append(path);
        }
    }

    bool hasNext() const
    {
        return m_index < m_list.size() - 1;
    }

    QString next()
    {
        if (!hasNext())
            return QString();

        m_index++;
        return currentFilePath();
    }

    QString currentFileName() const
    {
        return m_list.at(m_index);
    }

private:
    QStringList m_list;
    int m_index;

};

QAbstractFileEngine *SqlFileEngineHandler::create(const QString &fileName) const
{
    QRegExp exp("sql:/([^/]+)/([^/]+)(.*)");
    if (exp.exactMatch(fileName)) {
        return new SqlFileEngine(fileName);
    }
    return NULL;
}

SqlFileEngine::SqlFileEngine(const QString &fileName) :
    QAbstractFileEngine(),
    m_urlRegExp("sql:/([^/]+)/(.*)"),
    m_absoluteFileName(fileName)
{
    m_urlRegExp.exactMatch(fileName);
    QStringList list = m_urlRegExp.capturedTexts();
    m_db = QSqlDatabase::database(list[1]);

    m_filePath  = list[2];

    list = splitPath(list[2]);

    m_tableName = list.first();
    m_fileName = list.last();

    list.removeLast();
    m_path = list.join('/');

    createTable(m_tableName, m_db);

    m_nodeId = node(m_filePath);
    loadFile();
}

SqlFileEngine::~SqlFileEngine()
{
}

QAbstractFileEngine::FileFlags SqlFileEngine::fileFlags(QAbstractFileEngine::FileFlags type) const
{
    Q_UNUSED(type);

    FileFlags ret = 0;

    if (m_filePath.isEmpty()) {
        ret = DirectoryType | ExistsFlag | ReadUserPerm | WriteUserPerm;
    } else {
        QSqlQuery qry(m_db);
        qry.prepare(QString("SELECT flags FROM %1 WHERE rowid=:nodeid")
                    .arg(m_tableName));
        qry.bindValue(":nodeid", m_nodeId);
        if (qry.exec() && qry.next())
            ret = static_cast<FileFlags>(qry.value(0).toUInt());
    }

    return ret;
}

QString SqlFileEngine::fileName(QAbstractFileEngine::FileName file) const
{
    switch (file) {
    case DefaultName:
    case AbsoluteName:
        return m_absoluteFileName;

    default:
        qDebug() << "DEFAULT" << file;

    }

    return m_absoluteFileName;
}

QAbstractFileEngine::Iterator *SqlFileEngine::beginEntryList(
        QDir::Filters filters, const QStringList &filterNames)
{
    return new SqlFileEngineIterator(this, filters, filterNames);
}

bool SqlFileEngine::open(QIODevice::OpenMode openMode)
{
    if (m_nodeId < 0) {
        QSqlQuery qry(m_db);

        qry.prepare(QString("INSERT INTO %1 (create_date, parent, flags, name) "
                            "VALUES (CURRENT_TIMESTAMP, :parent, :flags, :name)")
                    .arg(m_tableName));

        int parent = node(m_path);
        if (parent < 0)
             return false;

        qry.bindValue(":parent", parent);

        qry.bindValue(":flags", static_cast<int>(
                FileType | ExistsFlag | ReadUserPerm | WriteUserPerm));
        qry.bindValue(":name", m_fileName);

        if (qry.exec()) {
            m_nodeId = qry.lastInsertId().toInt();
        } else {
            return false;
        }
    }

    m_openMode = openMode;

    if (openMode & QIODevice::Truncate) {
        setSize(0);
        m_pos = 0;
        return true;
    }

    return loadFile();
}

bool SqlFileEngine::mkdir(const QString &dirName, bool createParentDirectories) const
{
    Q_UNUSED(createParentDirectories);

    if (m_urlRegExp.exactMatch(dirName)) {
        QStringList list = m_urlRegExp.capturedTexts();


        QSqlDatabase db = QSqlDatabase::database(list[1]);
        QString path = list[2];
        list = splitPath(path);

        QString tableName = list.first();

        createTable(tableName, db);

        // Check if directory already exists
        if (node(path, db) >= 0)
            return false;

        if (createParentDirectories) {
            list.removeFirst(); // remove root file

            QSqlQuery insQuery(m_db);
            insQuery.prepare(QString("INSERT INTO %1 (create_date, parent, name, flags) "
                                "VALUES (CURRENT_TIMESTAMP, :parent, :name, :flags)")
                        .arg(tableName));

            QSqlQuery selQuery(m_db);

            int parent = 0;
            for (int i = 0; i < list.size(); i++) {
                selQuery.prepare(QString("SELECT rowid "
                                         "FROM %1 "
                                         "WHERE parent=:parent AND name=:name")
                                 .arg(m_tableName));
                selQuery.bindValue(":parent", parent);
                insQuery.bindValue(":parent", parent);
                selQuery.bindValue(":name", list.at(i));
                selQuery.exec();
                if (selQuery.next()) {
                    parent = selQuery.value(0).toInt();
                } else {
                    // not found => create
                    insQuery.bindValue(":name", list.at(i));
                    insQuery.bindValue(":flags", static_cast<unsigned>(
                            DirectoryType | ExistsFlag | ReadUserPerm | WriteUserPerm));
                    insQuery.exec();
                    parent = insQuery.lastInsertId().toInt();
                }
            }
        } else {
            // Check for the parent directory to exist otherwise fail
            QString newDir = list.last();
            list.removeLast();

            int parent = node(list.join('/'), db);
            if (parent < 0 || newDir.isEmpty())
                return false;

            QSqlQuery qry(db);
            qry.prepare(QString("INSERT INTO %1 (create_date, parent, name, flags) "
                                "VALUES (CURRENT_TIMESTAMP, :parent, :name, :flags)")
                        .arg(tableName));
            qry.bindValue(":parent", parent);
            qry.bindValue(":name", newDir);
            qry.bindValue(":flags", static_cast<unsigned>(
                    DirectoryType | ExistsFlag | ReadUserPerm | WriteUserPerm));

            return qry.exec();
        }

    }
    return true;
}

bool SqlFileEngine::rmdir(const QString &dirName, bool recurseParentDirectories) const
{
    m_urlRegExp.exactMatch(QDir::fromNativeSeparators(dirName));
    QStringList list = m_urlRegExp.capturedTexts();

    QSqlDatabase db = QSqlDatabase::database(list[1]);

    QString filePath  = list[2];
    list = splitPath(list[2]);

    QString tableName = list.first();

    int nodeId = node(filePath, db);

    if (nodeId < 0)
        return false;

    QSqlQuery qry(db);

    if (!recurseParentDirectories) {
        qry.prepare(QString("SELECT COUNT(*) FROM %1 WHERE parent=:parent")
                    .arg(tableName));
        qry.bindValue(":parent", nodeId);
        if (!(qry.exec() && qry.next() && qry.value(0).toInt() == 0))
            return false;
    }

    qry.prepare(QString("DELETE FROM %1 WHERE rowid=:rowid")
                .arg(tableName));
    qry.bindValue(":rowid", nodeId);
    return qry.exec();
}

bool SqlFileEngine::remove()
{
    if (m_nodeId >= 0) {
        QSqlQuery qry(m_db);
        qry.prepare(QString("DELETE FROM %1 WHERE rowid=:rowid")
                    .arg(m_tableName));
        qry.bindValue(":rowid", m_nodeId);
        return qry.exec();
    }
    return false;
}

bool SqlFileEngine::rename(const QString &newName)
{
    if (m_nodeId < 0)
        return false;

    if (m_urlRegExp.exactMatch(QDir::fromNativeSeparators(newName))) {
        QStringList list = m_urlRegExp.capturedTexts();

        QSqlDatabase db = QSqlDatabase::database(list[1]);

        QString filePath  = list[2];
        list = splitPath(filePath);

        QString tableName = list.first();
        list.removeLast();
        QString destPath = list.join('/');

        QSqlQuery qry(db);
        int parent = node(destPath);
        if (parent < 0)
            return false;
        qry.prepare(QString("UPDATE %1 SET parent=:parent WHERE rowid=:rowid")
                    .arg(tableName));
        qry.bindValue(":parent", parent);

        qry.bindValue(":rowid", m_nodeId);
        return qry.exec();
    } else {
        // Copy out of our virtual file system

    }

    return false;
}

qint64 SqlFileEngine::size() const
{
    return m_buffer.size();
}

bool SqlFileEngine::setSize(qint64 size)
{
    m_buffer.resize(size);
    return true;
}

bool SqlFileEngine::seek(qint64 pos)
{
    if (pos < m_buffer.size()) {
        m_pos = pos;
        return true;
    }

    return false;
}

qint64 SqlFileEngine::write(const char *data, qint64 len)
{
    if (m_openMode & (QIODevice::ReadWrite | QIODevice::WriteOnly)) {
        if (len + m_pos > size()) {
            setSize(m_pos + len);
        }
        m_buffer.replace(m_pos, len, data, len);
        m_pos += len;
    } else {
        len = -1;
    }

    return len;
}

qint64 SqlFileEngine::read(char *data, qint64 maxlen)
{
    qint64 len = qMin<qint64>(m_buffer.size() - m_pos, maxlen);
    memcpy(data + m_pos, m_buffer.data(), len);
    m_pos += len;
    return len;
}

bool SqlFileEngine::flush()
{
    QSqlQuery qry(m_db);
    qry.prepare(QString("UPDATE %1 SET write_date=CURRENT_TIMESTAMP, "
                        "data=:data "
                        "WHERE rowid=:rowid")
                .arg(m_tableName));

    qry.bindValue(":rowid", m_nodeId);
    qry.bindValue(":data", m_buffer);
    return qry.exec();
}

bool SqlFileEngine::close()
{
    return flush();
}

bool SqlFileEngine::loadFile()
{
    QSqlQuery qry(m_db);
    qry.prepare(QString("SELECT data FROM %1 WHERE rowid=:rowid")
                .arg(m_tableName));
    qry.bindValue(":rowid", m_nodeId);
    if (qry.exec() && qry.next()) {
        m_buffer = qry.value(0).toByteArray();
        m_pos = 0;
        return true;
    }
    return false;
}

QStringList SqlFileEngine::splitPath(const QString &path) const
{
    QStringList list = path.split('/');
    list.removeAll("");
    return list;
}

void SqlFileEngine::createTable(const QString &tableName, QSqlDatabase db) const
{
    QSqlQuery qry(m_db);
    qry.exec(QString("CREATE TABLE IF NOT EXISTS %1 ("
                "create_date INT, "
                "write_date INT, "
                "parent INT, "
                "name TEXT, "
                "flags INT, "
                "data BLOB, "
                "FOREIGN KEY(parent) REFERENCES %1(rowid) ON DELETE CASCADE"
                ")").arg(tableName));
    bool ok = qry.exec();

    // We use this dummy root entry to avoid messing around with multiple
    // queries for NULL value handling. So NULL becomes 0
    qry.prepare(QString("INSERT OR IGNORE INTO %1 (rowid, parent, flags, name) "
                        "VALUES (0, NULL, :flags, :name)")
                .arg(tableName));
    qry.bindValue(":flags", static_cast<int>(
            DirectoryType | ExistsFlag | ReadUserPerm | WriteUserPerm));
    qry.bindValue(":name", tableName);
    ok = qry.exec();
}

int SqlFileEngine::node(const QString &path, QSqlDatabase db) const
{
    QSqlQuery qry;
    if (db.isValid())
        qry = QSqlQuery(db);
    else
        qry = QSqlQuery(m_db);

    /* Plan A was to let the db do the recursive lookup if nodes by path strings.
     * Unfortunately this is only supported since sqlite 3.8.3 so we do it
     * manually here. May this snippet can be useful in the future so we kept it.
    WITH RECURSIVE parsed(p, l, r, n) AS (
        SELECT NULL, '', 'dir1/dir2/dir3', 0
        UNION ALL
        SELECT (SELECT rowid FROM script_fs WHERE (parent IS NULL OR parent = p)
                AND name=(CASE WHEN INSTR(r, '/')=0 THEN r ELSE SUBSTR(r, 0, INSTR(r, '/')) END) ),
            l || CASE WHEN INSTR(r, '/')=0 THEN r ELSE SUBSTR(r, 0, INSTR(r, '/') + 1) END,
            CASE WHEN INSTR(r, '/')=0 THEN NULL ELSE SUBSTR(r, INSTR(r, '/')+1) END, n+1
        FROM parsed
        WHERE r IS NOT NULL AND n < 9
    )
    SELECT *
    FROM parsed
    */


    QStringList list = splitPath(path);
    QString tableName = list.first();

    int parent = -1;
    for (int i = 0; i < list.size(); i++) {
        if (parent < 0) {
            qry.prepare(QString("SELECT rowid "
                                "FROM %1 "
                                "WHERE parent IS NULL AND name=:name")
                        .arg(tableName));

        } else {
            qry.prepare(QString("SELECT rowid "
                                "FROM %1 "
                                "WHERE parent=:parent AND name=:name")
                        .arg(tableName));
            qry.bindValue(":parent", parent);
        }

        qry.bindValue(":name", list.at(i));

        if (!qry.exec() || !qry.next()) {
            parent = -1;
            break;
        }
        parent = qry.value(0).toInt();
    }

    return parent;
}


