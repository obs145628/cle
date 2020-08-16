#include <logia/md-gfm-doc.hh>

namespace logia {

MdGfmDoc::MdGfmDoc(const std::string &name, bool link_inr_root)
    : Document(name, repr_document_type(), link_inr_root) {}

void MdGfmDoc::code_begin(const std::string &language) {
  raw_os().flush();
  raw_os() << "```" << language << "\n";
}

void MdGfmDoc::code_end() {
  raw_os() << "```\n";
  raw_os().flush();
}

MdGfmDoc::OsWrapperHelper<MdGfmDoc::CodeHelper>
MdGfmDoc::code(const std::string &language) {
  code_begin(language);
  return OsWrapperHelper<CodeHelper>(*this);
}

void MdGfmDoc::image(const std::string &label, const std::string path) {
  raw_os() << "![" << label << "](" << path << ")\n";
  raw_os().flush();
}

void MdGfmDoc::doc_link(const std::string &doc_id, const std::string &text) {
  raw_os() << "[" << text << "](#!doc-" << doc_id << ")";
  raw_os().flush();
}

void MdGfmDoc::doc_link(const Document &doc, const std::string &text) {
  doc_link(doc.id(), text.empty() ? doc.name() : text);
  raw_os().flush();
}

} // namespace logia
