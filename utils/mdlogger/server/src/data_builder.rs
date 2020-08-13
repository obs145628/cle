use crate::conf;
use crate::stats;
use crate::doc_builder;
use crate::doc_config;

use std::io::Write;
use chrono::naive::NaiveDateTime;

fn list_data_docs() -> Vec<String> {

    std::fs::read_dir(conf::path_data()).expect("Failed to read data dir")
        .map(|e| e.expect("Failed to read data dir"))
        .filter(|e| e.path().is_dir())
        .map(|e| String::from(e.path().file_name().unwrap().to_str().unwrap()))
        .filter(|s| s.starts_with("doc_"))
        .map(|s| String::from(&s[4..]))
        .collect()
}

fn build_root(st: &stats::Stats) {
    let root_path = conf::path_data().join(format!("doc_{}", conf::root_id()));
    let md_path = root_path.join("main.md");
    if  !root_path.is_dir() {
        std::fs::create_dir(&root_path).expect("Failed to create data dir for root doc");
    }

    // Get all docs with link in root, sorted by descending date
    let mut docs : Vec<_> = st.docs().values().filter(|d| d.link_in_root()).collect();
    docs.sort_by(|a, b| b.time().partial_cmp(&a.time()).unwrap());

    // Write all links to docs
    let mut f = std::fs::File::create(md_path).expect("Failed to open output root markdown file");
    for doc in docs {
        let t = NaiveDateTime::from_timestamp(doc.time() / 1000, 0);
        writeln!(f, "- [{}](#!doc-{}) ({})", doc.name(), doc.id(), t.format("%Y-%m-%d %H:%M")).unwrap();
    }

    // Save config file
    let mut conf = doc_config::DocConfig::new();
    conf.set_id(conf::root_id());
    conf.set_name("Home");
    conf.set_type("markdown");
    let conf_path = root_path.join("config.txt");
    conf.write_file(&conf_path);
}

pub fn update_all()
{
    let mut st = stats::Stats::load_main();
    let data_docs = list_data_docs();

    let mut upd_count = 0;
    for d in data_docs {
        if !st.has_doc(&d) && d != conf::root_id() {
            println!("Found new document {}", d);
            upd_count += 1;
            let conf = doc_builder::build_doc(&d);
            st.add_doc(conf);
        }
    }

    println!("Updated {} documents", upd_count);

    if upd_count != 0 {
        build_root(&st);
        doc_builder::build_doc(conf::root_id());
        st.save_main();
    }
}