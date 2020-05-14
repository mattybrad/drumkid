var markdownpdf = require("markdown-pdf")
  , fs = require("fs")

markdownpdf().from("docs/v6/manual.md").to("manual.pdf", function () {
  console.log("Manual PDF generated");
  markdownpdf().from("docs/v6/hackers_manual.md").to("hackers_manual.pdf", function () {
    console.log("Hackers' manual PDF generated");
  })
})
