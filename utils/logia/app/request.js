export function get(url, cb) {
    $.ajax({
        url: url,
        dataType: 'text',
        method: 'GET',
        success: (data) => { cb(null, data); },
        error: (err) => { cb(err, null); },
    });
}