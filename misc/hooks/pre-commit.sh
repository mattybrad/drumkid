git diff --cached --name-only | if grep --quiet "docs"
then
  node misc/gendocs/index.js
  git add manual.pdf
fi
