use crate::doc_config::DocConfig;
use crate::conf;
use std::path::Path;

fn md2html(in_path: &Path, out_path: &Path) {
    println!("MD2HTML: {:?} -> {:?}", in_path, out_path);
    let md_text = std::fs::read_to_string(in_path).expect("Failed to read md file");
    let html_text = comrak::markdown_to_html(&md_text, &comrak::ComrakOptions::default());
    std::fs::write(out_path, html_text).expect("Failed to html file");
}

fn dot2png(in_path: &Path, out_path: &Path) {
    // dot -Tpng <in-path> -o <out-path>
    println!("dot -Tpng: {:?} -> {:?}", in_path, out_path);

    let st = std::process::Command::new("dot")
        .arg("-Tpng")
        .arg(in_path)
        .arg("-o")
        .arg(out_path)
        .status().expect("Failed to run dot command");
    if !st.success() {
        println!("Dot command returned an error");
    }
}


pub fn build_doc(id: &str) -> DocConfig {

    // Read and check config
    let data_path = conf::path_data().join(format!("doc_{}", id));
    let conf_path = data_path.join("config.txt");
    let conf = DocConfig::from_text_file(&conf_path);
    assert!(conf.id() == id);

    // Create output dir
    let out_path = conf::path_web_docs().join(format!("doc_{}", id));
    if !out_path.is_dir() { //condition necessary for root
        std::fs::create_dir(&out_path).expect("Failed to create doc output dir");
    }

    //Find and convert custom input files
    for entry in std::fs::read_dir(&data_path).expect("Failed to read data dir") {
        let entry = entry.expect("Failed to read input dir");
        match entry.path().extension().and_then(std::ffi::OsStr::to_str) {
            Some("dot") => {
                // Convert dot to png file
                let png_path = out_path.join(format!("{}.{}", entry.path().file_stem().unwrap().to_str().unwrap(), "png"));
                dot2png(&entry.path(), &png_path);
            },
            _ => {}
        }

    }

    // Convert main input file
    match conf.doc_type() {

        "markdown" => {
            // Convert markdown file
            let md_path = data_path.join("main.md");
            let out_html_path = out_path.join("main.html");
            md2html(&md_path, &out_html_path);
        },
    
        unk_type => {
            panic!("Unknown input file type {}", unk_type);
        }
    };

    // Copy extra files
    let out_conf_path = out_path.join("config.txt");
    std::fs::copy(conf_path, out_conf_path).expect("Failed to copy doc config file");

    conf
}

