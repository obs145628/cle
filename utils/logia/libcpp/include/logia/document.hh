#pragma once

#include <fstream>
#include <string>

#include "doc-config.hh"

namespace logia {

// Fromatted text document
// Regroups functions common to all documents types
// Most of the time this class shouldn't be used directly
// Instead a subclass for a specific document type should be used
class Document {

public:
  template <class Helper> class OsWrapperHelper {
  public:
    using doc_t = typename Helper::doc_t;

    OsWrapperHelper(doc_t &doc) : _doc(&doc) {}

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
    doc_t *_doc;
  };

public:
  Document(const std::string &name, const std::string &type, bool link_in_root);
  Document(const Document &) = delete;
  Document &operator=(const Document &) = delete;

  ~Document();

  const DocConfig &conf() const { return _conf; }
  const std::string &name() const { return _conf.name; }
  const std::string &id() const { return _conf.id; }
  const std::string &type() const { return _conf.type; }
  const std::string &out_dir() const { return _dir; }
  std::ostream &raw_os() { return _os; }

  // Gen filepath of the form <out_dir>/UID<post>
  std::string gen_file_name(const std::string &post = "");
  std::string gen_file_path(const std::string &post = "");

  // Get file path of a local file, used on the WebApp to ref this file
  std::string get_file_path(const std::string &filename) const;

  template <class T> std::ostream &operator<<(const T &obj) {
    return raw_os() << obj;
  }

protected:
  DocConfig _conf;

private:
  std::string _dir;
  std::string _md_path;
  std::ofstream _os;
  std::size_t _unique;
};

} // namespace logia
