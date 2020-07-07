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

  var filePaths = [];
  var filesMoved = 0;
  for(var i=0; i<5; i++) {
    var thisFile = null;
    var thisPath = "./uploadtest/"+i+".wav";
    var thisSource = req.body['file'+(i+1)+'Source'];
    switch(thisSource) {
      case "default":
      thisPath = "./defaultsamples/" + i + ".wav";
      break;

      case "silent":
      thisPath = "./defaultsamples/silent.wav";
      break;
    }
    if(req.files) {
      if(req.files['file'+(i+1)]) {
        thisFile = req.files['file'+(i+1)];
        thisFile.mv(thisPath, function(){
          incrementFilesMoved();
        })
      } else {
        incrementFilesMoved();
      }
    } else {
      incrementFilesMoved();
    }
    filePaths[i] = thisPath;
  }

  function incrementFilesMoved() {
    filesMoved ++;
    if(filesMoved == 5) {
      sampleGen.generateArduinoFiles(filePaths, function(returnData){
        res.send(returnData);
      });
    }
  }


});

app.listen(port, () => console.log(`Example app listening at http://localhost:${port}`))
