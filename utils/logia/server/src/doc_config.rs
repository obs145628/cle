use serde::{Deserialize, Serialize};
use std::fs::File;
use std::io::{self, BufRead, Write};


#[derive(Clone, Serialize, Deserialize)]
pub struct DocConfig {
    id: String,
    link_in_root: bool,
    name: String,
    time: i64,
    _type: String,
}

impl DocConfig {

    pub fn id(&self) -> &str {
        &self.id
    }

    pub fn link_in_root(&self) -> bool {
        self.link_in_root
    }

    pub fn name(&self) -> &str {
        &self.name
    }

    pub fn time(&self) -> i64 {
        self.time
    }

    pub fn doc_type(&self) -> &str {
        &self._type
    }

    pub fn set_id(&mut self, id: &str) {
        self.id = String::from(id);
    }

    pub fn set_name(&mut self, name: &str) {
        self.name = String::from(name);
    }

    pub fn set_type(&mut self, new_type: &str) {
        self._type = String::from(new_type);
    }

    pub fn new() -> Self {
        DocConfig {
            id: String::new(),
            link_in_root: false,
            name: String::new(),
            time: 0,
            _type: String::from("default"),
        }
    }

    pub fn from_text_file(path: &std::path::Path) -> DocConfig {
        let f = File::open(path).expect("Failed to open input docconfig file");
        let is = io::BufReader::new(f);
        let mut res = DocConfig::new();

        for line in is.lines() {
            let line = line.expect("Failed to read input docconfig file");
            let line = line.trim();
            if line.len() == 0 {
                continue;
            }

            let line : Vec<_> = line.splitn(2, "=").collect();
            let key = line[0].trim();
            let val = line[1].trim();

            match key {
                "id" => res.id = String::from(val),
                "link-in-root" => res.link_in_root = val.parse().expect("invalid link-in-root option val"),
                "name" => res.name = String::from(val),
                "time" => res.time = val.parse().expect("invalid time option val"),
                "type" => res._type = String::from(val),
                _ => panic!("Unknown option '{}'", key),
            }
        }

        res
    }

    pub fn write_file(&self, path: &std::path::Path) {
        let mut f = std::fs::File::create(path).expect("Failed to open output config file");
        writeln!(f, "id = {}", self.id).unwrap();
        writeln!(f, "link-in-root = {}", self.link_in_root).unwrap();
        writeln!(f, "name = {}", self.name).unwrap();
        writeln!(f, "time = {}", self.time).unwrap();
        writeln!(f, "type = {}", self._type).unwrap();
    }

}