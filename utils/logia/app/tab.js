import "./doc-view";

import * as keysBinder from "./keys-binder";
import * as activeKeysBinder from "./active-keys-binder";
import * as utils from "./utils";

import { Rect } from "./rect";


Vue.component('tab', {

    props: ["state"],

    template: `
        <div clas="tab-content">
            <doc-view ref="eldocs" v-for="doc in docs" :key="doc.id" :state="doc" 
                    @doc-change="on_doc_changed" @doc-activated="on_doc_activated" @doc-close="on_doc_closed" />
        </div>
    `,

    data() {
        return {
            docs: [],
            next_id: 0,
            active_id: null,
            keys_event_handlers: {},
        }
    },

    created() {
        this.add_doc('__root', {
            left: 0,
            top: 0.,
            width: 1.,
            height: 1.,
        });

        activeKeysBinder.on_press(this, ['s', keysBinder.KEY_LEFT], () => this.split_view_x(this.active_id));
        activeKeysBinder.on_press(this, ['s', keysBinder.KEY_RIGHT], () => this.split_view_x(this.active_id));
        activeKeysBinder.on_press(this, ['s', keysBinder.KEY_UP], () => this.split_view_y(this.active_id));
        activeKeysBinder.on_press(this, ['s', keysBinder.KEY_DOWN], () => this.split_view_y(this.active_id));

        activeKeysBinder.on_press(this, [keysBinder.KEY_ALT, 'c'], () => this.del_doc(this.active_id));
    },

    methods: {

        add_doc(doc_id, rec) {
            const doc = {
                id: this.next_id++,
                init_doc: doc_id,
                active: false,
                left: rec.left,
                top: rec.top,
                width: rec.width,
                height: rec.height,
            };
            this.docs.push(doc);
            this.set_active(doc.id);
            return doc.id;
        },

        del_doc(doc_id) {
            // First poor implementation
            // Can only close a doc if it has a sidekick
            // which is a doc with: 
            // - same width, height
            // - same left, dist(a.top, b.top) == height (or opposite for top)
            // Combine both docs in one

            
            const doc = this.st_doc(doc_id);
            let match = null;
            const matches = this.docs.filter(d => d.width === doc.width && d.height === doc.height);

            const left_matches = matches.filter(d => {
                if (d.left !== doc.left)
                    return false;
                
                if (d.top > doc.top)
                    return (d.top - (doc.top + doc.height)) * (d.top - (doc.top + doc.height)) < 1e-5;
                else
                    return (doc.top - (d.top + d.height)) * (doc.top - (d.top + d.height)) < 1e-5; 
            });

            const top_matches = matches.filter(d => {
                if (d.top !== doc.top)
                    return false;
                
                if (d.left > doc.left)
                    return (d.left - (doc.left + doc.width)) * (d.left - (doc.left + doc.width)) < 1e-5;
                else
                    return (doc.left- (d.left +  d.width)) * (doc.left - (d.left + d.width)) < 1e-5; 
            });

            if (left_matches.length > 0)
                match = left_matches[0];
            if (top_matches.length > 0)
                match = top_matches[0];

            if (!match)
                return;

            // Merge 2 recs
            let new_rec = Rect.merge(Rect.from_ltwh(doc), Rect.from_ltwh(match)).to_lthw();
            
            // Change dims of second to merged rec
            utils.extend(match, new_rec);

            // Remove first doc
            this.docs = this.docs.filter(d => d.id != doc_id);
            this.set_active(match.id);
        },

        set_active(doc_id) {
            if (doc_id == this.active_id)
                return;

            const old_active = this.st_doc(this.active_id);
            const new_active = this.st_doc(doc_id);

            this.active_id = doc_id;

            if (old_active)
                old_active.active = false;
            if (new_active)
                new_active.active = true;
            this.docs = this.docs;
        },

        split_view_x(doc_id) {
            // divide view into 2 views of same height, width / 2
            const doc = this.st_doc(doc_id);
            const [_, rec2] = Rect.from_ltwh(doc).split_x();
            doc.width /= 2;
            this.docs = this.docs;
            
            const content_id = this.el_doc(doc_id).doc.id;
            this.add_doc(content_id, rec2.to_lthw());
        },

        split_view_y(doc_id) {
            // divide view into 2 views of same width, height / 2
            const doc = this.st_doc(doc_id);
            const [_, rec2] = Rect.from_ltwh(doc).split_y();
            doc.height /= 2;
            this.docs = this.docs;
            
            const content_id = this.el_doc(doc_id).doc.id;
            this.add_doc(content_id, rec2.to_lthw())
        },

        st_doc(doc_id) {
            for (let i = 0; i < this.docs.length; ++i)
                if (this.docs[i].id == doc_id)
                    return this.docs[i];
            return null;
        },

        el_doc(doc_id) {
            const docs = this.$refs.eldocs;
            if (!docs)
                return null;

            for (let i = 0; i < docs.length; ++i)
                if (docs[i].state.id == doc_id)
                    return docs[i];
            
            return null;
        },

        get_active_doc() {
            return this.el_doc(this.active_id);
        },

        on_doc_changed(doc_id, doc) {
            this.$emit('doc-change', this.state.id, doc);
        },

        on_doc_activated(doc_id) {
            this.set_active(doc_id);
        },

        on_doc_closed(doc_id) {
            this.del_doc(doc_id);
        }

    }
});