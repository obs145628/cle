import * as mock_data from "./mock_data";
import * as request from "./request";
import { Document } from "./document";

let ids_list = null;

const docs_dict = {};

export function get_doc(doc_id) {
    if (!docs_dict[doc_id])
        docs_dict[doc_id] = new Document(doc_id, null, null);
    return docs_dict[doc_id];
}

export function get_ready_doc(doc_id, cb) {
    let doc = get_doc(doc_id);
    if (doc.title !== null && doc.content !== null)
        return cb(null, doc);

    console.log('Loading', doc_id);

    let conf_url = '/docs/doc_' + doc_id + '/config.txt';
    let content_url = '/docs/doc_' + doc_id + '/main.html';
    request.get(conf_url, (err, data) => {
        if (err)
            return cb(err, null);

        doc.set_conf_raw(data);
        request.get(content_url, (err, data) => {
            if (err)
                return cb(err, null);

            doc.set_content_raw(data);
            cb(null, doc);
        });
    })
}
