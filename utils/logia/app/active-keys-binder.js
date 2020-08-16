// Wrapper around keysBinder, ensures events are only triggered to active items

import * as keysBinder from "./keys-binder";

const patterns_list = [];
let dispatcher = null;

function pattern_eq(a, b) {
    if (a == b)
        return true;
    if (a.length != b.length)
        return false;

    for (let i = 0; i < a.length; ++i)
        if (a[i] != b[i])
            return false;
    return true;
}

function pattern_find(arr, x) {
    for (let i = 0; i < arr.length; ++i)
    if (pattern_eq(arr[i], x))
        return i;
    return -1;
}

function on_press_cb(pat) {
    const id = pattern_find(patterns_list, pat);
    if (id === -1 || !dispatcher)
        return;

    dispatcher(id);
}

export function add_patern(pat) {
    let id = pattern_find(patterns_list, pat);
    if (id === -1) {
        id = patterns_list.length;
        patterns_list.push(pat);
        keysBinder.on_press(pat, on_press_cb)
        return id;
    }

    return id;
}

export function set_dispatcher(cb) {
    dispatcher = cb;
}

export function emit_event(component, id) {
    const handler = component.keys_event_handlers[id];
    if (handler)
        handler[0](handler[1]);
}

export function on_press(component, keys, cb) {
    const id = this.add_patern(keys);
    component.keys_event_handlers[id] = [cb, keys];
}