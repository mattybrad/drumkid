// express server to handle API requests

const express = require('express');
const fileUpload = require('express-fileupload');
const sampleGen = require('./samplegen.js');
const app = express();
const port = 3000;

app.use(fileUpload());

app.get('/samplegen', function(req, res) {
  res.send("this works");
});

app.post('/samplegen', function(req, res) {

  var fileData = [];
  for(var i=0; i<5; i++) {
    var thisFile = null;
    if(req.files) {
      if(req.files['file'+(i+1)]) {
        thisFile = req.files['file'+(i+1)];
      }
    }
    fileData[i] = {
      source: req.body['file'+(i+1)+'Source'],
      file: thisFile
    };
  }
  sampleGen.generateArduinoFiles(fileData, function(returnData){
    res.send(returnData);
  });

});

app.listen(port, () => console.log(`Example app listening at http://localhost:${port}`))
