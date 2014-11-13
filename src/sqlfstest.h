#ifndef SQLFSTEST_H
#define SQLFSTEST_H

#include <QSqlDatabase>
#include <QtTest/QtTest>

class SqlFileEngineHandler;

class SqlFsTest : public QObject
{
    Q_OBJECT

private:
    SqlFileEngineHandler *m_handler;

private slots:
    void initTestCase();
    void cleanupTestCase();

    void mkdir();
    void rmdir();
    void readWrite();

};

#endif // SQLFSTEST_H
