var markdownpdf = require("markdown-pdf")
  , fs = require("fs")

var mdOptions = {
  cssPath: "misc/gendocs/style.css"
}

markdownpdf(mdOptions).from("docs/v6/manual.md").to("manual.pdf", function () {
  console.log("Manual PDF generated");
})
