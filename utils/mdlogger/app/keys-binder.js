const keys_state = {};
const press_listeners = [];

export const KEY_CTRL = "control";
export const KEY_ALT = "alt";
export const KEY_SHIFT = "shift";
export const KEY_LEFT = "arrowleft";
export const KEY_RIGHT = "arrowright";
export const KEY_UP = "arrowup";
export const KEY_DOWN = "arrowdown";

function key_code(key) {
    return key.toLowerCase();
}

function press_matched(key, l) {
    const pat = l[0];
    let key_in_pat = false;

    for (let i = 0; i < pat.length; ++i) {
        if (!keys_state[pat[i]]) {
            // Check if all keys in the pattern are down
            //console.log('failed', l, pat[i]);
            return false;
        }

        // Check if the key press triggered the pattern
        key_in_pat = key_in_pat || pat[i] == key;
    }

    return key_in_pat;
}

function on_keydown(evt) {
    const key = key_code(evt.key);
    if (keys_state[key])
        return; //already down
        
    keys_state[key] = 1;

    let match = press_listeners.filter(l => press_matched(key, l));
    match.forEach(l => l[1](l[0]));
    if (match.length > 0)
        evt.preventDefault();

    //console.log("key_down", key);
}

function on_keyup(evt) {
    const key = key_code(evt.key);
    if (!keys_state[key])
        return; //already up

    keys_state[key] = 0;

    //console.log("key_up", key);
}

export function on_press(keys, cb) {
    press_listeners.push([keys, cb]);
}



document.addEventListener("keydown", on_keydown);
document.addEventListener("keyup", on_keyup);