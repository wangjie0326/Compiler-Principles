const fs = require('fs');
let src = fs.readFileSync('gen_report.js', 'utf8');
const lq = String.fromCodePoint(0x201C); // left double quotation mark
const rq = String.fromCodePoint(0x201D); // right double quotation mark
let count = 0;
while (src.includes(lq) || src.includes(rq)) {
  src = src.split(lq).join('"').split(rq).join('"');
  count++;
  if (count > 100) break;
}
fs.writeFileSync('gen_report.js', src, 'utf8');
console.log('replaced, remaining lq:', src.includes(lq), 'rq:', src.includes(rq));
