#pragma once

#include <fstream>
#include <string>

class MDDocument {

public:
  template <class Helper> class OsWrapperHelper {
  public:
    OsWrapperHelper(MDDocument &doc) : _doc(&doc) {}

    OsWrapperHelper(const OsWrapperHelper &) = delete;
    OsWrapperHelper(OsWrapperHelper &&h) : _doc(h._doc) { h._doc = nullptr; }
    OsWrapperHelper &operator=(const OsWrapperHelper &) = delete;

    ~OsWrapperHelper() {
      if (_doc) {
        Helper h;
        h(*_doc);
      }
    }

    std::ostream &os() { return _doc->raw_os(); }

    template <class T> std::ostream &operator<<(const T &obj) {
      return os() << obj;
    }

  private:
    MDDocument *_doc;
  };

  struct CodeHelper;

public:
  MDDocument(const std::string &out_dir);
  MDDocument(const MDDocument &) = delete;
  MDDocument &operator=(const MDDocument &) = delete;

  ~MDDocument();

  const std::string &out_dir() const { return _dir; }
  std::ostream &raw_os() { return _os; }

  /// Generate a block of code (using ```<language> ... ```)
  void code_begin(const std::string &language = "");
  void code_end();
  OsWrapperHelper<CodeHelper> code(const std::string &language = "");

  void image(const std::string &label, const std::string path);

  // Gen filepath of the form <out_dir>/UID<post>
  std::string gen_file_name(const std::string &post = "");
  std::string gen_file_path(const std::string &post = "");

  template <class T> std::ostream &operator<<(const T &obj) {
    return raw_os() << obj;
  }

private:
  std::string _dir;
  std::ofstream _os;
  std::size_t _unique;

public:
  struct CodeHelper {
    void operator()(MDDocument &doc) { doc.code_end(); }
  };
};
