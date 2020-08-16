import "./env";

Vue.component('app', {
    template: `
        <env />
    `,

    data() {
        return {}
    },

    methods: {}
});

const app = new Vue({
    el: '#app'
});
