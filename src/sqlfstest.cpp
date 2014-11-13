#include <QtSql>
#include <QtTest/QtTest>

#include "sqlfileengine.h"
#include "sqlfstest.h"

void SqlFsTest::initTestCase()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "fsdb");
    db.setDatabaseName(":memory:");
    QVERIFY2(db.open(), "Could not open database");
    m_handler = new SqlFileEngineHandler();
}

void SqlFsTest::cleanupTestCase()
{
    QSqlDatabase::database("fsdb").close();
    QSqlDatabase::removeDatabase("fsdb");
    delete m_handler;
}

void SqlFsTest::mkdir()
{
    QString path =  "sql:/fsdb/tblname/dir1";
    QDir dir(path);
    QVERIFY(!dir.exists());

    path = "sql:/fsdb/tblname";
    dir.setPath(path);
    dir.exists();
    QVERIFY(dir.exists());

    path = "sql:/fsdb/tblname/dir1";
    QVERIFY(dir.mkdir("dir1"));
    QVERIFY(dir.exists(path));
    dir.setPath(path);

    dir.mkdir("dir2");
    dir.setPath("sql:/fsdb/tblname/dir1/dir2");
    QVERIFY(dir.exists());
    QVERIFY(dir.exists("sql:/fsdb/tblname/dir1/dir2/"));

    path = "sql:/fsdb/tblname/dir2/dir3";
    QVERIFY(!dir.mkdir(path));
    QVERIFY(!dir.exists(path));

    // Again with mkpath
    QVERIFY(dir.mkpath(path));
    QVERIFY(dir.exists(path));
}

void SqlFsTest::rmdir()
{
    QDir dir("sql:/fsdb/tblname");

    QVERIFY(dir.mkpath("sql:/fsdb/tblname/dir1/dir2/dir3"));
    QVERIFY(!dir.rmdir("sql:/fsdb/tblname/dir1/dir2"));
    QVERIFY(!dir.rmdir("sql:/fsdb/tblname/dir1"));

    QVERIFY(dir.rmdir("sql:/fsdb/tblname/dir1/dir2/dir3"));
    QVERIFY(dir.rmdir("sql:/fsdb/tblname/dir1/dir2"));
    QVERIFY(dir.rmdir("sql:/fsdb/tblname/dir1"));

    // Same thing with trailing slashes
    QVERIFY(dir.mkpath("sql:/fsdb/tblname/dir1/dir2/dir3/"));
    QVERIFY(!dir.rmdir("sql:/fsdb/tblname/dir1/dir2/"));
    QVERIFY(!dir.rmdir("sql:/fsdb/tblname/dir1/"));

    QVERIFY(dir.rmdir("sql:/fsdb/tblname/dir1/dir2/dir3"));
    QVERIFY(dir.rmdir("sql:/fsdb/tblname/dir1/dir2/"));
    QVERIFY(dir.rmdir("sql:/fsdb/tblname/dir1"));

    QVERIFY(dir.mkpath("sql:/fsdb/tblname/dir1/dir2/dir3/"));
    QVERIFY(dir.rmpath("sql:/fsdb/tblname/dir1"));
    QVERIFY(!dir.exists("sql:/fsdb/tblname/dir1/dir2/dir3"));
    QVERIFY(!dir.exists("sql:/fsdb/tblname/dir1/dir2"));
    QVERIFY(!dir.exists("sql:/fsdb/tblname/dir1"));
}

void SqlFsTest::readWrite()
{
    QFile file("sql:/fsdb/tblname/dir1/file1");
    QVERIFY(!file.open(QIODevice::ReadWrite));

    QDir dir("sql:/fsdb/tblname/");
    QVERIFY(dir.mkdir("dir1"));

    QVERIFY(file.open(QIODevice::ReadWrite));
    QByteArray data1("First chunk of data");
    QVERIFY(file.write(data1) == data1.size());

    QByteArray data2("Second chunk of data");
    QVERIFY(file.write(data2) == data2.size());
    QVERIFY(file.size() == data1.size() + data2.size());
    file.close();

    QVERIFY(file.open(QIODevice::ReadOnly));
    QByteArray data = file.readAll();
    QVERIFY(data == (data1 + data2));
    QVERIFY(file.write(data) < 0);
    file.close();

}

QTEST_MAIN(SqlFsTest)
