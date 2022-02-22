#ifndef STORAGE_SQLITE_H
#define STORAGE_SQLITE_H
#include <sqlite3.h>
#include <string_view>

#include "../asts_interface.h"
#include "../util.h"

namespace ad::asts {

class SQLiteStorage {
private:
  char * tmp_buf;

  sqlite3* db_;
  sqlite3_stmt* ins_stmt = NULL;
  sqlite3_stmt* upd_stmt = NULL;

  inline void ExecOrThrow(std::string_view sql, std::string errormsg="Ошибка при выполнении запроса: ");
  inline void CheckRetCode(int e, const std::string& step, int expected = SQLITE_OK);
  bool IsStatementPrepared();
  void PrepareNextStatement(std::string& masked_tablename, std::shared_ptr<AstsTable> table, fld_count_t* fldnums, fld_count_t fldcount);

public:
  SQLiteStorage();
  ~SQLiteStorage();
  // MTESRL-related operaions
  void AddInterface(std::shared_ptr<AstsInterface> iface); // Connect
  void RemoveInterface(std::shared_ptr<AstsInterface> iface); // Disconnect

  void StartReadingRows(AstsOpenedTable* table);
  void ReadRowFromBuffer(AstsOpenedTable* table, ad::util::PointerHelper& buffer, fld_count_t* fldnums, fld_count_t* fldnums_prev, fld_count_t fldcount);
  void StopReadingRows();

  void CreateTable(std::shared_ptr<AstsInterface> iface, const std::string& tablename);
  void CloseTable(const std::string& tablename);
  void RefreshTable();
  void Query(std::string_view query);
};

} // ad::asts
#endif // STORAGE_SQLITE_H
