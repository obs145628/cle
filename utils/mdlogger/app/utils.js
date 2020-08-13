
export function extend(dst, src) {
    for (let key in src)
        if (src.hasOwnProperty(key))
            dst[key] = src[key];
    return dst;
}