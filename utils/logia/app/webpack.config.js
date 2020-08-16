const path = require('path');
const copyPlugin = require('copy-webpack-plugin');

module.exports = {
  entry: "./bootstrap.js",
  output: {
    path: path.resolve(__dirname, "../www"),
    filename: "bootstrap.js",
  },
  mode: "development",

  plugins: [
    new copyPlugin({
      patterns: [
        { from: './static' }
      ]
    })
  ],
};
