// express server to handle API requests

const express = require('express');
const fileUpload = require('express-fileupload');
const sampleGen = require('./samplegen.js');
const { v4: uuidv4 } = require('uuid');
var zip = require('cross-zip');
const fs = require('fs');
const path = require('path');
const app = express();
const port = 3000;

app.use(fileUpload({
  limits: { fileSize: 10 * 1024 * 1024 },
}));

app.use('downloads', express.static('downloads'));
app.use(express.static('app'));

app.get('/samplegen', function(req, res) {
  res.send("the server works");
});

app.post('/samplegen', function(req, res) {

  var filePaths = [];
  var filesMoved = 0;
  var tempFolderName = uuidv4();
  fs.mkdirSync('./tempFolders/'+tempFolderName);
  fs.mkdirSync('./tempFolders/'+tempFolderName+'/uploads');
  fs.mkdirSync('./tempFolders/'+tempFolderName+'/rawfiles');
  fs.mkdirSync('./tempFolders/'+tempFolderName+'/arduino');
  for(var i=0; i<5; i++) {
    var thisFile = null;
    var thisPath = './tempFolders/'+tempFolderName+'/uploads/'+i+".wav";
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
      sampleGen.generateArduinoFiles(tempFolderName, filePaths, function(numCells){
        // make zip file
        var inPath = path.join(__dirname, 'tempfolders', tempFolderName, 'arduino');
        var outFileName = 'drumkid_samples_'+tempFolderName+'.zip';
        var outPath = path.join(__dirname, 'downloads', outFileName);
        zip.zipSync(
          inPath,
          outPath
        )

        // delete temp folder


        var msg = numCells + " cells";
        res.send(JSON.stringify({
          numCells: numCells,
          fileName: outFileName
        }));
      });
    }
  }


});

app.listen(port, () => console.log(`Example app listening at http://localhost:${port}`))
