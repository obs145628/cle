import "./tab";

import * as activeKeysBinder from "./active-keys-binder";
import * as keysBinder from "./keys-binder";

Vue.component('workspace', {

    props: ['state'],

    template: `
        <div class="workspace">
            <div class="tabs-menu">
                <button v-for="tab in tabs" class="tab-menu-item" :class="{active: tab.id === active_id}" 
                    :key="tab.id" v-on:click="set_active(tab.id)">{{ tab.title }}</button>

                <button class="tab-menu-item" v-on:click="add_tab">+</button>
            </div>

            <div class="workspace-content">
                <tab ref="eltabs" v-for="tab in tabs" :key="tab.id" 
                    v-show="tab.id === active_id" :state="tab" @doc-change="on_doc_changed" />
            </div>
        </div>
    `,

    data() {
        return {
            tabs: [],
            active_id: null,
            next_id: 0,
            keys_event_handlers: {},
        }
    },

    created() {
        this.add_tab();

        activeKeysBinder.on_press(this, [keysBinder.KEY_ALT, 't'], () => {
            this.add_tab();
        });

        activeKeysBinder.on_press(this, [keysBinder.KEY_ALT, 'w'], () => {
            this.del_tab(this.active_id);
        });
    },


    methods: {

        add_tab() {
            const new_tab = {
                title: 'New Tab',
                id: this.next_id++,
                renamed: false,
            };
            this.tabs = this.tabs.concat([new_tab]);
            this.set_active(new_tab.id);
        },

        del_tab(tab_id) {
            let next = this.get_left_tab_id(this.active_id);
            if (next === null)
                next = this.get_right_tab_id(this.active_id);

            this.tabs = this.tabs.filter(t => t.id != tab_id);

            if (next === null)
                this.add_tab();
            else
                this.set_active(next);
        },

        set_active(tab_id) {
            this.active_id = tab_id;
        },

        get_left_tab_id(tab_id) {
            for (let i = 1; i < this.tabs.length; ++i) {
                if (this.tabs[i].id == tab_id)
                    return this.tabs[i-1].id;
            }
            return null;
        },

        get_right_tab_id(tab_id) {
            for (let i = 0; i < this.tabs.length - 1; ++i) {
                if (this.tabs[i].id == tab_id)
                    return this.tabs[i+1].id;
            }
            return null;
        },

        el_tab(tab_id) {
            const tabs = this.$refs.eltabs;
            if (!tabs)
                return null;

            for (let i = 0; i < tabs.length; ++i)
                if (tabs[i].state.id == tab_id)
                    return tabs[i];
            
            return null;
        },

        get_active_tab() {
            return this.el_tab(this.active_id);
        },

        on_doc_changed(tab_id, doc) {
            let tab = this.tabs.find(t => t.id == tab_id);
            if (!tab.renamed) {
                tab.title = doc.title;
                this.tabs = this.tabs;
            }
        },
    }
});