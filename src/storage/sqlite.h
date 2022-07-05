#ifndef STORAGE_SQLITE_H
#define STORAGE_SQLITE_H
#include <sqlite3.h>
#include <string_view>

#include "../generic_engine.h"

namespace ad::asts {

class SQLiteStorage : GenericStorage {
private:
  char * tmp_buf;

  sqlite3* db_;
  sqlite3_stmt* ins_stmt = NULL;
  sqlite3_stmt* upd_stmt = NULL;

  inline void ExecOrThrow(std::string_view sql, std::string errormsg="Ошибка при выполнении запроса: ");
  inline void CheckRetCode(int e, const std::string& step, int expected = SQLITE_OK);
  bool IsStatementPrepared();
  void PrepareNextStatement(std::string& masked_tablename, std::shared_ptr<AstsTable> table, fld_count_t* fldnums, fld_count_t fldcount);
  void TransactionControl(const std::string& action);

public:
  SQLiteStorage();
  ~SQLiteStorage();
  void AddInterface(std::shared_ptr<AstsInterface> iface);
  void RemoveInterface(std::shared_ptr<AstsInterface> iface);

  void StartReadingRows(AstsOpenedTable* table);
  void ReadRowFromBuffer(AstsOpenedTable* table, ad::util::PointerHelper& buffer, fld_count_t* fldnums, fld_count_t* fldnums_prev, fld_count_t fldcount);
  void EraseData(const std::string& tablename, const std::string& secboard="", const std::string& seccode="");
  void StopReadingRows();

  void CreateTable(std::shared_ptr<AstsInterface> iface, const std::string& tablename);
  void CloseTable(const std::string& tablename);
  void Query(std::string_view query, SqlResult& result, std::map<std::string, std::shared_ptr<AstsInterface> >& interfaces);
};

} // ad::asts
#endif // STORAGE_SQLITE_H
