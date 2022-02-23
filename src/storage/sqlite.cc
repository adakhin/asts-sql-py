#include "sqlite.h"
#include "../util.h"
#include <sstream>
#include <mtesrl.h>
#include <string.h> // memset

namespace ad::asts {

constexpr static const char all_spaces[MTE_SQL_MAX_FIELDS] = {' '};

inline void SQLiteStorage::ExecOrThrow(std::string_view sql, std::string errormsg) {
  char *zErrMsg = nullptr;
  int error = sqlite3_exec(db_, sql.data(), NULL, 0, &zErrMsg);
  if(error)
    throw std::runtime_error(errormsg+": "+std::string(zErrMsg));
}

SQLiteStorage::SQLiteStorage() {
  int error = sqlite3_open(":memory:", &db_);
  if(error) {
    sqlite3_close(db_);
    throw std::runtime_error("Unable to initialize SQLite storage");
  }
}

SQLiteStorage::~SQLiteStorage() {
  sqlite3_close(db_);
}

void SQLiteStorage::AddInterface(std::shared_ptr<AstsInterface> iface) {
  std::string sql = "create table if not exists MTE$STRUCTURE (system_type char(2), interface_name char(12), table_name char(12), orig_table_name char(12), field_name char(20), field_type integer, field_length integer, decimals integer);";
  std::string errmsg = std::string("SQLite error while create reflection for interface ")+iface->name_;
  ExecOrThrow(sql, errmsg);
  auto add_fld = [&](std::string v) {
    sql.append("'"+v+"',");
  };
  auto add_int = [&](int v, bool last_val=false) {
    sql.append(std::to_string(v));
    if(!last_val)
      sql.append(",");
  };
  for(auto & rec : iface->tables)
    for(auto & fld : rec.second->outfields) {
      sql = "insert into MTE$STRUCTURE select ";
      add_fld(iface->GetSystemType());
      add_fld(iface->name_);
      add_fld(iface->prefix_+rec.second->name);
      add_fld(rec.second->name);
      add_fld(fld.name);
      add_int(fld.type);
      add_int(fld.size);
      add_int(fld.decimals, true);
      sql.append(";");
      ExecOrThrow(sql, errmsg);
    }
  sql.append("create index if not exists MTE$STRUCTURE_IDX on MTE$STRUCTURE (table_name, field_name);");
}

void SQLiteStorage::CreateTable(std::shared_ptr<AstsInterface> iface, const std::string& tablename) {
  // check if table exists
  std::string expr = "SELECT * FROM sqlite_master WHERE name ='"+tablename+"' and type='table' COLLATE NOCASE;";
  sqlite3_stmt *statement;
  int error = sqlite3_prepare_v2(db_, expr.c_str(), -1, &statement, 0);
  if(error)
      throw std::runtime_error("SQLite error occured while checking if table "+tablename+" already exists: "+std::string(sqlite3_errstr(error)));
  size_t rowcount = 0;
  while(true) {
      int res = sqlite3_step(statement);
      if(res == SQLITE_ROW)
          ++rowcount;
      if(res == SQLITE_DONE || res==SQLITE_ERROR)
          break;
  }
  sqlite3_finalize(statement);
  if(!rowcount) {
    // table doesn't exist. create it
    std::string create, pk, tmp;
    create = "create table "+tablename+" (";
    pk = ", PRIMARY KEY (";
    bool haskeyfields = false;
    std::vector<std::string> fields, pk_fields;
    fields.reserve(iface->tables[tablename]->outfields.size());
    for(auto & fld : iface->tables[tablename]->outfields) {
        tmp = "";
        switch(fld.type)
        {
        case AstsFieldType::kInteger:
          tmp = fld.name+" integer";
          break;
        case AstsFieldType::kFixed:
        case AstsFieldType::kFloatPoint:
          tmp = fld.name+" double";
          break;
        case AstsFieldType::kChar:
        case AstsFieldType::kDate:
        case AstsFieldType::kTime:
        case AstsFieldType::kFloat: // we will know how to format this only in runtime
        default:
          tmp = fld.name+" char("+std::to_string(fld.size)+")";
          break;
        }
        if( (fld.attr & mffKey) == mffKey) {
          haskeyfields = true;
          tmp.append(" NOT NULL");
          pk_fields.push_back(fld.name);
        }
        fields.push_back(tmp);
    }
    create.append(ad::util::join(fields,", "));
    if(haskeyfields)
      pk.append(ad::util::join(pk_fields, ", ").append(")"));
    else
      pk = "";
    create.append(pk.append(");"));
    ExecOrThrow(create, "SQLite error occured while creating table "+tablename);
  }
}

void SQLiteStorage::ReadRowFromBuffer(AstsOpenedTable* table, ad::util::PointerHelper& buffer, fld_count_t* fldnums, fld_count_t* fldnums_prev, fld_count_t fldcount) {
  // if statements are not set or cannot be reused, we need to prepare next statements
  if(!(IsStatementPrepared() && memcmp(fldnums, fldnums_prev, fldcount) == 0))
    PrepareNextStatement(table->tablename_, table->thistable_, fldnums, fldcount);

  // at this point we are sure we have prepared INSERT statement, and maybe we have prepared UPDATE statement as well
  sqlite3_clear_bindings(ins_stmt);
  if(upd_stmt != NULL)
    sqlite3_clear_bindings(upd_stmt);

  int sqlite_idx = 0;
  int64_t tmpint = 0;
  double tmpdouble = 0.0;
  AstsOutField fld;

  for(fld_count_t c=0; c<fldcount; ++c) {
    fld = table->thistable_->outfields[fldnums[c]];
    memset(tmp_buf, 0x00, sizeof(*tmp_buf)); // reset temp buffer for field data
    sqlite_idx = c+1;
    int * ptr = buffer._ptr;
    // ASTSConnectivty API Guide says NULL is a value consisting only of spaces
    if(memcmp(ptr, all_spaces, fld.size) == 0) {
      buffer.Rewind(fld.size);
      sqlite3_bind_null(ins_stmt, sqlite_idx);
      if(upd_stmt != NULL)
          sqlite3_bind_null(upd_stmt, sqlite_idx);
    }
    else switch(fld.type) { // regular value
      case AstsFieldType::kFixed:
        // copy first part (before dot) of value to temp buffer
        tmpint = fld.size - fld.decimals;
        memcpy(tmp_buf, ptr, (int)tmpint);
        // set the dot
        tmp_buf[(int)tmpint] = '.';
        // copy second part of value to temp buffer
        memcpy(tmp_buf+(int)tmpint+1, (char*)ptr+(int)tmpint, fld.decimals);
        buffer.Rewind(fld.size);
        tmpdouble = atof(tmp_buf);
        sqlite3_bind_double(ins_stmt, sqlite_idx, tmpdouble);
        if(upd_stmt != NULL)
          sqlite3_bind_double(upd_stmt, sqlite_idx, tmpdouble);
        break;
      case AstsFieldType::kFloatPoint:
        memcpy(tmp_buf, ptr, fld.size);
        buffer.Rewind(fld.size);
        tmpdouble = atof(tmp_buf);
        sqlite3_bind_double(ins_stmt, sqlite_idx, tmpdouble);
        if(upd_stmt != NULL)
          sqlite3_bind_double(upd_stmt, sqlite_idx, tmpdouble);
        break;
      case AstsFieldType::kInteger:
        memcpy(tmp_buf, ptr, fld.size);
        buffer.Rewind(fld.size);
        tmpint = strtoll(tmp_buf, nullptr, 10);
        sqlite3_bind_int64(ins_stmt, sqlite_idx, tmpint);
        if(upd_stmt != NULL)
          sqlite3_bind_int64(upd_stmt, sqlite_idx, tmpint);
        break;
      case AstsFieldType::kFloat: // we don't know how to format this
      case AstsFieldType::kChar:
      case AstsFieldType::kDate: // leave date and time as strings, let upper-level code deal with this
      case AstsFieldType::kTime:
      default:
        sqlite3_bind_text(ins_stmt, sqlite_idx, (char*)ptr, fld.size, SQLITE_TRANSIENT);
        if(upd_stmt != NULL)
          sqlite3_bind_text(upd_stmt, sqlite_idx, (char*)ptr, fld.size, SQLITE_TRANSIENT);
        buffer.Rewind(fld.size);
        break;
    }
  } // for each field in this row

  int error = 0;
  bool doInsert = false, has_keyfields = !table->thistable_->keyfields.empty();
  if(has_keyfields && upd_stmt != NULL) {
    error = sqlite3_step(upd_stmt);
    CheckRetCode(error, "EXECUTE UPDATE", SQLITE_DONE);
    // if CheckRetCode throws exception, we won't get here
    int rows_updated = sqlite3_changes(db_);
    if(rows_updated == 0)
      doInsert = true;
    sqlite3_reset(upd_stmt);
    CheckRetCode(error, "RESET UPDATE", SQLITE_DONE);
  }
  if(doInsert || !has_keyfields) {
    error = sqlite3_step(ins_stmt);
    CheckRetCode(error, "EXECUTE INSERT", SQLITE_DONE);
    // if CheckRetCode throws exception, we won't get here
    sqlite3_reset(ins_stmt);
    CheckRetCode(error, "RESET INSERT", SQLITE_DONE);
  }
}

void SQLiteStorage::CloseTable(const std::string& tablename) {
  std::string del = "delete from "+tablename+";";
  ExecOrThrow(del, "SQLite error occured while deleting table "+tablename);
}

void SQLiteStorage::StartReadingRows(AstsOpenedTable* table) {
  tmp_buf = new char[table->thistable_->max_fld_len+2];
}

void SQLiteStorage::StopReadingRows() {
  sqlite3_finalize(ins_stmt);
  sqlite3_finalize(upd_stmt);
  ins_stmt = NULL;
  upd_stmt = NULL;
  delete[] tmp_buf;
}

bool SQLiteStorage::IsStatementPrepared() {
  return (ins_stmt != NULL || upd_stmt != NULL);
}

void SQLiteStorage::PrepareNextStatement(std::string& masked_tablename, std::shared_ptr<AstsTable> table, fld_count_t* fldnums, fld_count_t fldcount) {
  std::ostringstream insert, update;
  std::ostringstream insfields, insvalues;
  std::ostringstream updfields, updkey;
  insert << "INSERT OR IGNORE INTO " << masked_tablename << " ";
  update << "UPDATE " << masked_tablename << " SET ";

  bool haskeyfields = false;
  bool hasupd = false;
  bool hasins = false;
  std::string sqlite_idx;
  AstsOutField fld;
  for(fld_count_t c = 0; c < fldcount; ++c) {
    fld = table->outfields[fldnums[c]];
    sqlite_idx = "?"+std::to_string((int)c+1); // numbers start with 1
    if(hasins) {
      insfields << ",";
      insvalues << ",";
    }
    hasins = true;;
    insfields << fld.name;
    insvalues << sqlite_idx;
    if ( (fld.attr & mffKey) == mffKey) {
      if(haskeyfields)
        updkey << " AND ";
      haskeyfields = true;
      updkey << fld.name << "=" << sqlite_idx;
    }
    else {
      if(hasupd)
        updfields << ",";
      hasupd = true;
      updfields << fld.name << "=" << sqlite_idx;
    }
  }

  // INSERT
  insert << "(" << insfields.str() << ") VALUES (" << insvalues.str() << ");";
  if(hasupd) {
    update << updfields.str();
    if(haskeyfields)
      update<< " WHERE " << updkey.str();
  }
  else
    update.str("");

  int error = sqlite3_finalize(ins_stmt);
  CheckRetCode(error, "FINALIZE INSERT");
  error = sqlite3_finalize(upd_stmt);
  CheckRetCode(error, "FINALIZE UPDATE");
  if(hasupd) {
    error = sqlite3_prepare_v2(db_, update.str().c_str(), -1, &upd_stmt, 0);
    CheckRetCode(error, "PREPARE UPDATE");
  }
  else
    upd_stmt = NULL;
  error = sqlite3_prepare_v2(db_, insert.str().c_str(), -1, &ins_stmt, 0);
  CheckRetCode(error, "PREPARE INSERT");
}

void SQLiteStorage::CheckRetCode(int e, const std::string &step, int expected) {
  if(e != expected)
    throw std::runtime_error("Error on "+step+" step: "+std::string(sqlite3_errmsg(db_)));
}

void SQLiteStorage::Query(std::string_view query, SqlResult& result, std::map<std::string, std::shared_ptr<AstsInterface> >& interfaces) {
  result.fields = {};
  result.data = {};
  sqlite3_stmt *statement;
  int error = sqlite3_prepare_v2(db_, query.data(), -1, &statement, 0);
  if (error == SQLITE_OK) {
    int ctotal = sqlite3_column_count(statement);

    std::map<std::string, int> fncounts;
    auto fncount_chk = [&](std::string fieldname) {
      if(fncounts.find(fieldname) != fncounts.end()) {
        int c = fncounts[fieldname];
        fncounts[fieldname] = ++c;
        return fieldname+std::to_string(c);
      }
      else {
        fncounts[fieldname] = 0;
        return fieldname;
      }
    };

    bool is_first_row = true;
    std::string aliased_fieldname, tbl;
    while(true) {
      int res = sqlite3_step(statement);
      if(res != SQLITE_ROW) {
        if (res != SQLITE_DONE)
          error = res;
        break;
      }
      std::vector<std::any> tmp_row;
      for(int i=0; i<ctotal; i++) {
        if(is_first_row) { // process metadata
          /*
           * Field name may be one of the following:
           *  1. field name from interface's table
           *  2. aliased field name from interface's table
           *  3. field name from custom table
           *  4. aliased field name from custom table
           *  5. aliased expression from query (function, case, etc)
           * So determining type of field should work this way:
           *  1. determine real name of field
           *  2. based on the source of this field:
           *  2.1 if it's a table from interface - take it from interface
           *  2.2 if it's an expression - ask SQLite
           *  2.3 if it's a table NOT from interface - ask SQLite as well
           * We should check field names for duplicates either way
          */
          const char * cn = sqlite3_column_origin_name(statement,i);
          aliased_fieldname = sqlite3_column_name(statement,i);
          bool sqlite_type = false;
          AstsOutField orig_fld;
          if (cn == NULL) // it's an expression
            sqlite_type = true;
          else {
            // it's a field from some table
            tbl = sqlite3_column_table_name(statement,i);
            bool found = false;
            // search all interfaces for a table with this name
            for (auto& v : interfaces)
              if(v.second->tables.find(tbl) != v.second->tables.end()) {
                auto orig_iface = v.second;
                for(auto& field : orig_iface->tables[tbl]->outfields)
                  if(field.name == std::string(cn)) {
                    sqlite_type = false;
                    orig_fld = field;
                    found = true;
                    break;
                  }
              }
            if(!found)
              sqlite_type = true;
          }
          if(sqlite_type) {
            SqlOutField cfield;
            size_t f = aliased_fieldname.find_first_of('(');
            aliased_fieldname = aliased_fieldname.substr(0, f);
            cfield.name = fncount_chk(aliased_fieldname);
            int ct = sqlite3_column_type(statement, i);
            switch(ct) {
              case SQLITE_INTEGER:
                cfield.type = AstsFieldType::kInteger;
                cfield.size = 16;
                break;
              case SQLITE_FLOAT:
                cfield.type = AstsFieldType::kFloatPoint;
                cfield.size = 16;
                break;
              case SQLITE_TEXT:
                cfield.type = AstsFieldType::kChar;
                cfield.size = strlen((char*)sqlite3_column_text(statement, i));
                break;
              case SQLITE_NULL:
                cfield.type = AstsFieldType::kNull;
                cfield.size = 0;
                break;
              case SQLITE_BLOB:
                throw std::runtime_error("Invalid field type in SQLite query results");
            }
            result.fields.push_back(cfield);
          }
          else {
            tbl = sqlite3_column_table_name(statement,i);
            SqlOutField tmp;
            tmp.name = fncount_chk(aliased_fieldname);
            tmp.type = orig_fld.type;
            tmp.size = orig_fld.size;
            result.fields.push_back(tmp);
          }
        } // if first row (metadata)
        // process actual data
        std::string s;
        char * ptr = (char*)sqlite3_column_text(statement, i);
        if(ptr)
          s = std::string(ptr);
        else
          s = "";
        tmp_row.push_back(std::make_any<std::string>(s));
      } // for value in row
      is_first_row = false;
      result.data.push_back(tmp_row);
    } // while sqlite_step
  } // if prepare yields SQLITE_OK
  // finalize statement anyway
  sqlite3_finalize(statement);
  if(error != SQLITE_OK) {
#if SQLITE_VERSION_NUMBER < 3008000
    std::string errmsg = sqlite3_errmsg(db_);
#else
    std::string errmsg = sqlite3_errstr(sqlite3_extended_errcode(db));
    errmsg += ": "+string(sqlite3_errmsg(db_));
#endif
    throw std::runtime_error(errmsg);
  }

}


}
