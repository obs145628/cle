import "./workspace";

import * as activeKeysBinder from "./active-keys-binder";
import * as keysBinder from "./keys-binder";

Vue.component('env', {

    props: [],

    template: `
        <div class="env">
            <workspace ref="elworkspaces" v-for="ws in workspaces" :key="ws.id" v-show="ws.id == active_id" :state="ws" />
        </div>
    `,

    data() {
        return {
            workspaces: [],
            active_id: null,
            next_id: 0,
            keys_event_handlers: {},
        }
    },

    created() {
        activeKeysBinder.set_dispatcher(id => this.dispatch_event(id));
        this.add_workspace();
    },


    methods: {

        add_workspace() {
            const new_ws = {
                id: this.next_id++,
            };
            this.workspaces = this.workspaces.concat([new_ws]);
            this.set_active(new_ws.id);
        },

        set_active(ws_id) {
            this.active_id = ws_id;
        },

        el_workspace(ws_id) {
            const ws = this.$refs.elworkspaces;
            if (!ws)
                return null;

            for (let i = 0; i < ws.length; ++i)
                if (ws[i].state.id == ws_id)
                    return ws[i];
            
            return null;
        },

        get_active_workspace() {
            return this.el_workspace(this.active_id);
        },

        dispatch_event(id) {
            const ws = this.get_active_workspace();
            const tab = ws ? ws.get_active_tab() : null;
            const doc = tab ? tab.get_active_doc() : null;

            // Emit event to all active components, from doc to itself

            if (doc)
                activeKeysBinder.emit_event(doc, id);
            if (tab)
                activeKeysBinder.emit_event(tab, id);
            if (ws)
                activeKeysBinder.emit_event(ws, id);
            activeKeysBinder.emit_event(this, id);
        },

    }
});