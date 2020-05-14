var markdownpdf = require("markdown-pdf")
  , fs = require("fs")

var mdOptions = {
  cssPath: "misc/gendocs/style.css"
}

markdownpdf(mdOptions).from("docs/v6/manual.md").to("manual.pdf", function () {
  console.log("Manual PDF generated");
  markdownpdf(mdOptions).from("docs/v6/hackers_manual.md").to("hackers_manual.pdf", function () {
    console.log("Hackers' manual PDF generated");
  })
})
