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

function generateArduinoFile(path, callback) {
  console.log("run on this path:",path);
  setTimeout(callback, 5000);
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
