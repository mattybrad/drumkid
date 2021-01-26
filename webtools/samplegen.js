/*

sample generation script
takes a set of audio files as in input
outputs arduino code for drumkid

order of operations:
- choose files in HTML form
- send files and other options to express API
- move chosen files to temporary folder
- run sox to generate 8-bit raw files
- run char2mozzi on each file
- check size, alert if too large for sketch
- zip all .h files
- download zip to user's computer

*/

const { spawn } = require('child_process');

var sampleRate = '8192'; // usually 16384

function generateArduinoFile(tempFolderName, sourcePath, baseName, callback) {
  console.log("run on this path/name:",sourcePath, baseName);
  const soxProcess = spawn('sox', [
    sourcePath,
    '--bits',
    '8',
    '-r',
    sampleRate,
    '--encoding',
    'signed-integer',
    '--endian',
    'little',
    'tempfolders/'+tempFolderName + '/rawfiles/' + baseName + '.raw'
  ]);
  soxProcess.on('exit', function(code, signal) {
    // should really check code/signal here, but it's late

    const mozziProcess = spawn('python', [
      'char2mozzi.py',
      'tempfolders/'+tempFolderName + '/rawfiles/'+baseName+'.raw',
      'tempfolders/'+tempFolderName + '/arduino/'+baseName+'.h',
      baseName,
      sampleRate
    ]);
    var numCells = 0;
    mozziProcess.stdout.on('data', function(data) {
      console.log(baseName,'...received stdout:',data.toString());
      numCells = parseInt(data.toString());
    });
    mozziProcess.on('exit', function(code, signal) {
      console.log(baseName, '...exited',code,signal,numCells);
      callback(numCells);
    });
  })
}

exports.generateArduinoFiles = function(tempFolderName, filePaths, callback) {
  var filesDone = 0;
  var totalNumCells = 0;
  console.log("file paths: ",filePaths);
  console.log("number of file paths: ",filePaths.length);
  for(var i=0; i<filePaths.length; i++) {
    generateArduinoFile(tempFolderName, filePaths[i], "sample"+i, function(numCells) {
      filesDone ++;
      totalNumCells += numCells;
      if(filesDone == filePaths.length) {
        callback(totalNumCells);
      }
    });
  }
}
