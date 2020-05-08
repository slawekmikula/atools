/*****************************************************************************
* Copyright 2015-2020 Alexander Barthel alex@littlenavmap.org
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************/

#ifndef ATOOLS_SQL_SQLUTIL_H
#define ATOOLS_SQL_SQLUTIL_H

#include <QStringList>
#include <QVariant>
#include <functional>

namespace atools {
namespace sql {

class SqlDatabase;
class SqlQuery;
class SqlRecord;

class SqlUtil
{
public:
  /*
   * Create a util using the given database.
   */
  explicit SqlUtil(atools::sql::SqlDatabase *sqlDb);
  explicit SqlUtil(atools::sql::SqlDatabase& sqlDb);

  /*
   * Prints row counts for all tables in the database.
   * @param out QDebug, QTextStream or std::iostream object. The stream needs
   * to have an operator<< for QString.
   * @param endline Print endline or not. Useful for streams that automatically
   * insert endlines like QDebug.
   */
  void printTableStats(QDebug& out, const QStringList& tables = QStringList());

  /* Create a list of columns for the given table excluding all in excludeColumns */
  QStringList buildColumnList(const QString& tablename, const QStringList& excludeColumns = QStringList());

  void createColumnReport(QDebug& out, const QStringList& tables = QStringList());
  void reportDuplicates(QDebug& out, const QString& table, const QString& idColumn,
                        const QStringList& identityColumns);

  int bindAndExec(const QString& sql, QVector<std::pair<QString, QVariant> > params);
  int bindAndExec(const QString& sql, const QString& bind, const QVariant& value);

  /* Creates an insert statement including all columns for the given table. */
  QString buildInsertStatement(const QString& tablename, const QString& otherClause = QString(),
                               const QStringList& excludeColumns = QStringList(), bool namedBindings = true);

  /* Creates a select statement including all columns for the given table. */
  QString buildSelectStatement(const QString& tablename);
  QString buildSelectStatement(const QString& tablename, const QStringList& columns);

  /* @return true if table exists */
  bool hasTable(const QString& tablename);
  bool hasTableAndColumn(const QString& tablename, const QString& columnname);

  /* @return true if table exists and has rows */
  bool hasTableAndRows(const QString& tablename);

  /* @return number of distinct rows if table and column exist. Otherwise -1. */
  int getTableColumnAndDistinctRows(const QString& tablename, const QString& columnname);

  /* Get number of rows in table. Throws exception if the table does not exist. */
  int rowCount(const QString& tablename, const QString& criteria = QString());

  /* Faster than rowCount */
  bool hasRows(const QString& tablename, const QString& criteria = QString());

  /* Copy all values from one query to another
   * @param from a valid query as data source
   * @param to a valid prepared query having the same number of bind variables
   * as the from query columns
   * @param func Function that acts a filter. If return value is true the row is
   * inserted. Will be called after all variables are bound.
   * @return number of rows copied
   */
  static int copyResultValues(atools::sql::SqlQuery& from, atools::sql::SqlQuery& to,
                              std::function<bool(atools::sql::SqlQuery& from, atools::sql::SqlQuery& to)> func);
  static int copyResultValues(atools::sql::SqlQuery& from, atools::sql::SqlQuery& to);

  /*
   * Copies the values of one row from one statement to another.
   * @param from valid query as data source
   * @param to a valid prepared query having the same number of bind variables
   * as the from query columns
   */
  static void copyRowValues(const atools::sql::SqlQuery& from, atools::sql::SqlQuery& to);

  void reportRangeViolations(QDebug& out, const QString& table, const QStringList& reportCols,
                             const QString& column, const QVariant& minValue,
                             const QVariant& maxValue);

  typedef  std::function<bool (const atools::sql::SqlQuery&, atools::sql::SqlQuery&)> UpdateColFuncType;
  /* Calls func for earch row to allow complex calculations for each row in the table which could not
   * be done in the database
   * @param table Table to be modified
   * @param idColum Column to identify a row
   * @param queryColumns Columns needed in the source table
   * @param insertcolumns Columns in the target statement to be modified.
   */
  void updateColumnInTable(const QString& table, const QString& idColum, const QStringList& queryColumns,
                           const QStringList& insertcolumns, const QString& whereClause,
                           atools::sql::SqlUtil::UpdateColFuncType func);
  void updateColumnInTable(const QString& table, const QString& idColum, const QStringList& queryColumns,
                           const QStringList& insertcolumns, atools::sql::SqlUtil::UpdateColFuncType func);

private:
  SqlDatabase *db;

  QStringList buildTableList(const QStringList& tables);
  QStringList buildResultList(atools::sql::SqlQuery& query);

  static void copyRowValuesInternal(const atools::sql::SqlQuery& from, atools::sql::SqlQuery& to,
                                    const atools::sql::SqlRecord& fromRec, const QMap<QString, QVariant>& bound);

};

// -----------------------------------------------

} // namespace sql
} // namespace atools

#endif // ATOOLS_SQL_SQLUTIL_H
