#ifndef STORAGE_SQLITE_H
#define STORAGE_SQLITE_H
#include <sqlite3.h>
#include <string_view>

#include "../asts_interface.h"
#include "../util.h"

namespace ad::asts {

class SQLiteStorage {
private:
  sqlite3* db_;
  inline void ExecOrThrow(std::string_view sql, std::string errormsg="Ошибка при выполнении запроса: ");
public:
  SQLiteStorage();
  ~SQLiteStorage();
  // MTESRL-related operaions
  void AddInterface(std::shared_ptr<AstsInterface> iface); // Connect
  void RemoveInterface(std::shared_ptr<AstsInterface> iface); // Disconnect
  void CreateTable(std::shared_ptr<AstsInterface> iface, const std::string& tablename);
  void ReadRowFromBuffer(AstsOpenedTable* table, ad::util::PointerHelper& buffer, unsigned char* fldnums, unsigned char* fldnums_prev, unsigned char fldcount);
  void CloseTable(const std::string& tablename);
  void RefreshTable();
  void Query(std::string_view query);
};

} // ad::asts
#endif // STORAGE_SQLITE_H
