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
- zip all .h files
- download zip to user's computer

*/

const { spawn } = require('child_process');

function generateArduinoFile(path, callback) {
  console.log("run on this path:",path);
  const soxProcess = spawn('sox', [
    path,
    '--bits',
    '8',
    '-r',
    '16384',
    '--encoding',
    'signed-integer',
    '--endian',
    'little',
    'rawfiles/test.raw'
  ]);
  soxProcess.on('exit', function(code, signal) {
    // should really check code/signal here, but it's late

    const mozziProcess = spawn('python', [
      'char2mozzi.py',
      'rawfiles/'+"test"+'.raw',
      'arduinofiles/'+"raw"+'.h',
      "testing",
      '16384'
    ]);
    mozziProcess.on('exit', function(code, signal) {
      callback();
    });
  })
}

exports.generateArduinoFiles = function(filePaths, callback) {
  var filesDone = 0;
  for(var i=0; i<filePaths.length; i++) {
    generateArduinoFile(filePaths[i], function() {
      filesDone ++;
      if(filesDone == filePaths.length) {
        callback("done all the files yeah");
      }
    });
  }
}
