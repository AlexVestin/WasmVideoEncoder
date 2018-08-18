var path = require('path');

module.exports = {
  entry: './src/index.js',

  module: {
    rules: [
      {
        test: /\.js$/,
        exclude: /(node_modules|bower_components)/,
        use: {
          loader: 'babel-loader',
          options: {
            presets: ['@babel/preset-env']
          }
        }
      }
    ]
  },
  devServer: {
    contentBase: "./public",
    hot: true
},
  
  output: {
    path: path.resolve(__dirname, "public"),
    filename: "bundle.js"
  },
};
