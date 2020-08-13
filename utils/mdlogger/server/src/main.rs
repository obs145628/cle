#![feature(proc_macro_hygiene, decl_macro)]

extern crate chrono;
extern crate comrak;
extern crate rocket;
extern crate rocket_contrib;

mod conf;
mod data_builder;
mod doc_builder;
mod doc_config;
mod stats;


use rocket_contrib::serve::StaticFiles;
use rocket::fairing::AdHoc;

fn reload_docs() {
	data_builder::update_all();
}

fn main() {

    reload_docs();
    
    rocket::ignite()


	.attach(AdHoc::on_request("Generate docs", |req, _| {
	    let uri = req.uri().path();
	    if uri == "/" || uri == "/index.html" {
		reload_docs();
	    }
	}))

	
        .mount("/", StaticFiles::from("../www"))
        .launch();
}
