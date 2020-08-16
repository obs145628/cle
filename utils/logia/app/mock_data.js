import { Document } from "./document";

const MOCK_DOCS = [
    {
        id: "__root",
        title: "Home",
        content: `
            <div>
            <h1>Home</h1>
            <ul>
                <li><a href="#!doc-d0">My First Compiler</a></li>
                <li><a href="#!doc-d1">It's all about money</a></li>
                <li><a href="#!doc-d2">I shall be level 200</a></li>
            </ul>
            </div>
        `
    },

    {
        id: "d0",
        title: "My First Compiler",
        content: `
            <div>
            <h1>My First Compiler</h1>
            <p>This is a first compiler test</p>
            <p>I don't even have a name yet</p>
            </div>
        `
    },

    {
        id: "d1",
        title: "It's all about money",
        content: `
            <div>
            <h1>It's all about $$$</h1>
            <p>I just want a Bugatti</p>
            <p>And to live in a mension obviosuly</p>
            </div>
        `
    },

    {
        id: "d2",
        title: "I shall be level 200",
        content: `
            <div>
            <h1>The ultimate level</h1>
            <p>I jusst need to farm the Ougah</p>
            <p>And the Chene Mou</p>
            </div>
        `
    },
];

export function load_all(cb) {
    setTimeout(() => {
        const docs = MOCK_DOCS.map(d => new Document(d.id, null, null));
        cb(null, docs);
    }, 10);
}

export function load_doc(doc, cb) {
    setTimeout(() => {

        if (doc.title !== null)
            return cb(null, doc);

        const match = MOCK_DOCS.filter(d => d.id == doc.id);
        if (match.length != 1)
            return cb(new Error("Invalid document id"), null);
    
        doc.title = match[0].title;
        doc.content = match[0].content;
        cb(null, doc);
    }, 10);
}