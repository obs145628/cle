#pragma once

#include "document.hh"

namespace logia {

// Common Mark with Github Flavored Markdown
// Github version of Markdown
class MdGfmDoc : public Document {

  struct CodeHelper;

public:
  static const char *repr_document_type() { return "cmark-gfm"; }

  MdGfmDoc(const std::string &name, bool link_inr_root);

  /// Generate a block of code (using ```<language> ... ```)
  void code_begin(const std::string &language = "");
  void code_end();
  OsWrapperHelper<CodeHelper> code(const std::string &language = "");

  /// Insert an image (path may be local or global url)
  void image(const std::string &label, const std::string path);

  // Add a link to another document
  void doc_link(const std::string &doc_id, const std::string &text);
  void doc_link(const Document &doc, const std::string &text = "");

private:
  struct CodeHelper {
    using doc_t = MdGfmDoc;

    void operator()(MdGfmDoc &doc) { doc.code_end(); }
  };
};

} // namespace logia
