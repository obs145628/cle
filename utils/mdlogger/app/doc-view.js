import { Url} from "./url";
import * as dims from "./dimensions";
import * as docsLoader from "./docs-loader";
import * as keysBinder from "./keys-binder";
import * as activeKeysBinder from "./active-keys-binder";

Vue.component('doc-view', {

    props: ['state'],

    template: `
        <div class="doc-view" :style="pos_style" v-on:click="on_doc_activated">
            <div class="doc-view-header" :class="{active: state.active}" >
                <a href="#" class="enabled" v-on:click.prevent="close_doc">X</a>
                <a href="#" :class="{enabled: hist_idx>1}" v-on:click.prevent="hist_prev">P</a>
                <a href="#" :class="{enabled: hist_idx<hist.length}" v-on:click.prevent="hist_next">N</a>
                <span> {{ doc.title }} </span>
            </div>
            <div class="doc-view-content" v-html="doc.content"></div>
        </div>
    `,

    data() {
        return {
            doc: null,
            hist: [],
            hist_idx: 0,
            keys_event_handlers: {},
        }
    },

    created() {
        this.change_doc(this.state.init_doc);
        
        activeKeysBinder.on_press(this, [keysBinder.KEY_ALT, keysBinder.KEY_LEFT], () => this.hist_prev());
        activeKeysBinder.on_press(this, [keysBinder.KEY_ALT, keysBinder.KEY_RIGHT], () => this.hist_next());
    },

    mounted() {
        this.on_ready();
    },

    updated() {
        this.on_ready();
    },

    methods: {
        on_ready() {
            let content_el = this.$el.getElementsByClassName("doc-view-content")[0];
            if (content_el.innerHTML.length > 0)
                this.sync_content(content_el);
        },

        change_doc(doc_id, keep_hist) {
            // Update history
            if (!keep_hist) {
                this.hist = this.hist.slice(0, this.hist_idx).concat([doc_id]);
                this.hist_idx = this.hist.length;
            }

            this.doc = docsLoader.get_doc(doc_id);
            docsLoader.get_ready_doc(doc_id, (err, doc) => {
                if (err)
                    throw err;
                this.doc = doc;
                this.$emit('doc-change', this.state.id, this.doc);
            });
        },

        sync_content(data) {
            let doc_links = data.getElementsByTagName('a');
            doc_links = Array.prototype.map.call(doc_links, el => [el, new Url(el.href)]);
            doc_links = doc_links.filter(el => el[1].is_route() && el[1].route.startsWith('doc-'));
            
            doc_links.forEach((el) => {
                let doc_id = el[1].route.substr(4);
                el[0].addEventListener('click', (evt) => {
                    evt.preventDefault();
                    this.on_click_doc_link(doc_id);
                });
            });

            // Need to manualy run highligh js for new block of code
            data.querySelectorAll('pre code').forEach((block) => {
                if (block.className === "language-asm") //trick to set armasm as default for asm
                    block.className = "language-armasm";
                hljs.highlightBlock(block);
            });

        },

        on_click_doc_link(doc_id) {
            this.change_doc(doc_id);
        },

        on_doc_activated() {
            this.$emit('doc-activated', this.state.id);
        },

        hist_prev() {
            if (this.hist_idx <= 1) //already at first item
                return;

            const doc_id = this.hist[this.hist_idx - 2];
            this.change_doc(doc_id, true);
            --this.hist_idx;
        },

        hist_next() {
            if (this.hist_idx >= this.hist.length) //already at last item
                return;
    
            const doc_id = this.hist[this.hist_idx];
            this.change_doc(doc_id, true);
            ++this.hist_idx;
        },

        get_page_rect() {
            const [win_w, win_h] = dims.getPageDims();
            const menu_el = document.getElementsByClassName('tabs-menu')[0];
            let off_w = 0;
            let off_h = 42; //default value
            if (menu_el) {
                off_h = menu_el.getBoundingClientRect().bottom;
            }
            
            const [tab_w, tab_h] = [win_w, win_h - off_h];

            return {
                left: off_w + tab_w * this.state.left,
                top: off_h + tab_h * this.state.top,
                width: tab_w * this.state.width,
                height: tab_h * this.state.height,
            };
        },

        close_doc() {
            this.$emit('doc-close', this.state.id);
        },
    },

    computed: {
        pos_style() {
            const res = this.get_page_rect();
            return {
                left: res.left + 'px',
                top: res.top + 'px',
                width: res.width + 'px',
                height: res.height + 'px',
            };
        }
    }
});