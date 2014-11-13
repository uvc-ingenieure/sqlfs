**********************************************
sqlfs - Qt File System Engine with SQL Backend
**********************************************

Sqlfs is a Qt file system engine similar to the Qt resource system but not
read only. It was developed to provide transparent file storage for 
Qt applications. By now the engine is limited to SQLite. Support for other
DBMS's can easily be implemented (just a bit of SQL tweaking).

The main use case is to give applications the ability to host their own 
file systems in their own SQLite based applications files.

========
Features
========

* Transparent for Qt file system classes
* Can host QML code
* Uses only one table in the database
* Lightweight and simple code
* MIT license

=====
Usage
=====

Similar to Qt's resource system sqlfs uses an URL like prefix to route
file system access to sqlfs: `sql:/<database name>/<table name>/`

.. code-block:: c++

    // Open a database with an explicit connection name
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", "fsdb");
    db.setDatabaseName(":memory:");
    db.open();
	
	// Install sqlfs' file engine into Qt's factory
    SqlFileEngineHandler fileEngine();
	
	// Use the file system as usual
	QFile data("sql:/fsdb/fstable/output.txt");
    if (data.open(QFile::WriteOnly | QFile::Truncate)) {
        QTextStream out(&data);
        out << "This is stored into the database";
		data.close();
    }

.. footer:: Copyright (c) UVC Ingenieure http://uvc.de/