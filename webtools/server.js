// express server to handle API requests

const express = require('express');
const fileUpload = require('express-fileupload');
const sampleGen = require('./samplegen.js');
const { v4: uuidv4 } = require('uuid');
var zip = require('cross-zip');
const fs = require('fs');
const path = require('path');
const del = require('del');
const app = express();
const port = 3000;

app.use(fileUpload({
  limits: { fileSize: 10 * 1024 * 1024 },
}));

app.use('/downloads', express.static('downloads'));
app.use(express.static('app'));

app.get('generatesamples', function(req, res) {
  res.send("the server works, but you need to send a post request");
});

app.post('generatesamples', function(req, res) {

  var filePaths = [];
  var filesMoved = 0;
  var tempFolderName = uuidv4();
  fs.mkdirSync('./tempfolders/'+tempFolderName);
  fs.mkdirSync('./tempfolders/'+tempFolderName+'/uploads');
  fs.mkdirSync('./tempfolders/'+tempFolderName+'/rawfiles');
  fs.mkdirSync('./tempfolders/'+tempFolderName+'/arduino');
  for(var i=0; i<5; i++) {
    var thisFile = null;
    var thisPath = './tempfolders/'+tempFolderName+'/uploads/'+i+".wav";
    var uploadPath = thisPath;
    var thisSource = req.body['file'+(i+1)+'Source'];
    switch(thisSource) {
      case "default":
      thisPath = "./defaultsamples/" + i + ".wav";
      break;

      case "silent":
      thisPath = "./defaultsamples/silent.wav";
      break;
    }
    filePaths[i] = thisPath;
    if(req.files) {
      if(req.files['file'+(i+1)]) {
        thisFile = req.files['file'+(i+1)];
        console.log(i, "req.files exists, attempting to move...")
        thisFile.mv(uploadPath, function(){
          console.log("..moved");
          incrementFilesMoved();
        })
      } else {
        console.log(i, "req.files exists but no file with this name");
        incrementFilesMoved();
      }
    } else {
      console.log(i, "no req.files");
      incrementFilesMoved();
    }
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
        var folderToDelete = path.join(__dirname, 'tempfolders', tempFolderName);
        del.sync(folderToDelete);

        var msg = numCells + " cells";

        var outputHTML = "<html><body>";
        var maxCells = 15245;
        var percentage = Math.ceil(100*numCells/maxCells)+"%";
        if(numCells <= maxCells) {
          outputHTML += "These samples use " + percentage + " of the available space and should work fine";
        } else {
          outputHTML += "These samples use " + percentage + " of the available space and therefore may not compile";
        }
        outputHTML += "<br/>";
        outputHTML += "<a href='./downloads/"+outFileName+"'>Download samples</a>";
        outputHTML += "</body></html>";

        res.send(outputHTML);
      });
    }
  }


});

app.listen(port, () => console.log(`Example app listening at http://localhost:${port}`))
