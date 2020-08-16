
function parse_conf(data) {
    let res = {};
    data.split('\n').forEach(l => {
        l = l.trim();
        if (l.length == 0)
            return;

        let p = l.indexOf('=');
        let key = l.substr(0, p).trim();
        let val = l.substr(p+1).trim();
        res[key] = val;
    });
    return res;
}


export class Document {

    constructor(id, title, content) {
        this.id = id;
        this.title = title;
        this.content = content;
        this.time = null;
    }

    set_conf_raw(data) {
        let conf = parse_conf(data);
        if (conf.id != this.id)
            throw Error('Invalid document id in conf');
        this.title = conf.name;
        this.time = conf.timel
    }

    set_content_raw(data) {
        this.content = data;
    }

}

