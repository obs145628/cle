export class Url {

    constructor(str) {
        this.str = str;

        const parts = this.str.split('#');
        this.page = parts[0];
        this.tag = parts.length > 1 ? '#' + parts[1] : '';

        this.route = this.tag.startsWith('#!') ? this.tag.substr(2) : null;
    }

    is_route() {
        return this.route !== null;
    }

}