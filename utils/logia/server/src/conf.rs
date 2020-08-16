use std::path::PathBuf;

pub fn path_project_root() -> PathBuf {
    PathBuf::from("..")
}

pub fn path_data() -> PathBuf {
    path_project_root().join("data")
}

pub fn path_data_config() -> PathBuf {
    path_data().join("config.json")
}

pub fn path_web() -> PathBuf {
    path_project_root().join("www")
}

pub fn path_web_docs() -> PathBuf {
    path_web().join("docs")
}

pub fn root_id() -> &'static str {
    "__root"
}
