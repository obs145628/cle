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
  MDDocument(const std::string &name, const std::string &type,
             bool link_in_root);
  MDDocument(const MDDocument &) = delete;
  MDDocument &operator=(const MDDocument &) = delete;

  ~MDDocument();

  const std::string &name() const { return _name; }
  const std::string &id() const { return _id; }
  const std::string &type() const { return _type; }
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

  // Get file path of a local file, used on the WebApp to ref this file
  std::string get_file_path(const std::string &filename) const;

  // Add a link to another document
  void doc_link(const std::string &doc_id, const std::string &text);
  void doc_link(const MDDocument &doc, const std::string &text = "");

  template <class T> std::ostream &operator<<(const T &obj) {
    return raw_os() << obj;
  }

private:
  std::string _name;
  std::string _id;
  std::string _type;
  std::string _dir;
  std::string _md_path;
  std::ofstream _os;
  std::size_t _unique;

public:
  struct CodeHelper {
    void operator()(MDDocument &doc) { doc.code_end(); }
  };
};
