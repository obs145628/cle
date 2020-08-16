use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fs::File;
use crate::doc_config::DocConfig;
use crate::conf;


#[derive(Serialize, Deserialize)]
pub struct Stats {
    docs: HashMap<String, DocConfig>,
}

impl Stats {

    pub fn docs(&self) -> &HashMap<String, DocConfig> {
        &self.docs
    }

    pub fn new() -> Self {
        Stats {
            docs: HashMap::new(),
        }
    }

    pub fn load_main() -> Self {
        let path = conf::path_data_config();
        if !path.is_file() {
            return Stats::new();
        }

        let f = File::open(path).expect("Cannot open main stat file");
        serde_json::from_reader(f).expect("Failed to read main stat file")
    }

    pub fn save_main(&self) {
        let path = conf::path_data_config();
        let f = File::create(path).expect("Cannot open main stat file");
        serde_json::to_writer(f, &self).expect("Cannot write main stat file");
    }

    pub fn has_doc(&self, id: &str) -> bool {
        self.docs.contains_key(id)
    }

    pub fn add_doc(&mut self, doc: DocConfig) {
        self.docs.insert(String::from(doc.id()), doc);
    }

}