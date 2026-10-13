// mkbookie.js -- the VM/370plus Guide as a BookMaster document for bookie
// (9.30): content.js is run against helpers that write GML instead of
// docx, so the Guide can be formatted on VM/370plus itself:
//    node mkbookie.js > GUIDE.SCRIPT       then on CMS:  BOOKIE GUIDE (PS
// Lines are cut at 72 columns (CMS files of 80), between words.
const out = [];
const W = 72;
// CMS files are EBCDIC: everything outside Latin-1 becomes ASCII here
function asc(t) {
  return t.replace(/[\u2018\u2019]/g, "'").replace(/[\u201c\u201d]/g, '"')
          .replace(/\u2013/g, '-').replace(/\u2192/g, '->').replace(/\u00a0/g, ' ')
          .replace(/\u2026/g, '...').replace(/[^\x00-\xff]/g, '?');
}
function gml(t) {
  // text: & and : would start symbols and tags; `code`, **bold**, *italic*
  t = t.replace(/&/g, '&amp.').replace(/:/g, '&colon.');
  t = t.replace(/`([^`]+)`/g, ':xph.$1:exph.')
       .replace(/\*\*([^*]+)\*\*/g, ':hp2.$1:ehp2.')
       .replace(/\*([^*]+)\*/g, ':hp1.$1:ehp1.');
  t = t.replace(/’/g, "'").replace(/‘/g, "'").replace(/[“”]/g, '"')
       .replace(/–/g, '-').replace(/—/g, '&mdash.').replace(/•/g, '&bullet.')
       .replace(/→/g, '->').replace(/ /g, ' ');
  return asc(t);
}
function emit(tag, text) {
  let line = tag;
  for (const w of (text || '').split(/ +/)) {
    if (!w) continue;
    if (line.length + 1 + w.length > W && line.length > 0) { out.push(line); line = w; }
    else line = line ? (line.endsWith('.') && line.startsWith(':') && line === tag ? line + w : line + ' ' + w) : w;
  }
  if (line) out.push(line);
}
const P = (t) => emit(':p.', gml(t));
const H1 = (t) => { body(); out.push(':h1.' + gml(t)); };
const H1plain = (t) => out.push(':h1.' + gml(t));
const H2 = (t) => out.push(':h2.' + gml(t));
const H3 = (t) => out.push(':h3.' + gml(t));
const B = (items) => { out.push(':ul.'); items.forEach(t => emit(':li.', gml(t))); out.push(':eul.'); };
const N = (items) => { out.push(':ol.'); items.forEach(t => emit(':li.', gml(t))); out.push(':eol.'); };
const NOTE = (t) => emit(':note.', gml(t));
const ATTN = (t) => emit(':attention.', gml(t));
function SCREEN(lines, caption) {
  out.push(':fig.', ':xmp.');
  lines.forEach(l => out.push(asc(l).replace(/&/g, '&amp.').replace(/:exmp\./gi, '&colon.exmp.')));
  out.push(':exmp.');
  if (caption) emit(':figcap.', gml(caption));
  out.push(':efig.');
}
function TABLE(head, rows, widths, caption) {
  out.push(':fig.', ":table cols='" + (widths && widths.length === head.length ? widths.join(' ') : head.map(() => '*').join(' ')) + "'.", ':thd.', ':row.');
  head.forEach(h => emit(':c.', gml(h)));
  out.push(':ethd.');
  rows.forEach(r => { out.push(':row.'); r.forEach(c => emit(':c.', gml(String(c)))); });
  out.push(':etable.');
  if (caption) emit(':figcap.', gml(caption));
  out.push(':efig.');
}
let inbody = false;
function body() { if (!inbody) { out.push(':body.'); inbody = true; } }
const setAppendix = () => { out.push(':appendix.'); };

out.push('.* GUIDE SCRIPT -- the VM/370plus Guide in BookMaster markup,',
         '.* made by manual/mkbookie.js from manual/content.js: format with',
         '.* BOOKIE GUIDE (PS on CMS, or bookie.py --groff ps GUIDE.SCRIPT',
         ':userdoc.', ':prolog.',
         ':prodname.VM/370plus',
         ':subtitle.Virtual Machine Facility/370, ESA/390 Edition',
         ":title.Introduction, Installation and User's Guide",
         ":stitle.VM/370plus Introduction, Installation and User's Guide",
         ':version.Release 1 &bullet. Kit Build 8, updated 13 October 2026',
         ':based.based on VM/370 Community Edition V1 R1.2',
         ':docnum.VMP-0001-0', ':filenum.S370-34',
         ':eprolog.', ':frontm.', ':tipage.',
         ":edition title='First Edition, revised (October 2026)'.");
['This edition applies to VM/370plus Release 1 (kit build 5, 9 October 2026), an ESA/390 conversion of the VM/370 Community Edition V1 R1.2, and to all subsequent builds until otherwise indicated in new editions.',
 'VM/370plus is a hobbyist project. It is not a product of, and this manual is not a publication of, International Business Machines Corporation. IBM, System/370, ESA/390, z/Architecture and VM/370 are names of IBM products and architectures, used here only to identify the systems this software runs on and descends from.',
 'VM/370 itself is in the public domain. The VM/370 Community Edition is maintained by its community. Hercules is an open-source emulator. Linux and Debian are trademarks of their respective owners. cREXX is an open-source REXX implementation.',
 'The project repository holds the update decks, the build tools, the issue register and the test runs from which every statement in this manual can be checked. Comments on this publication may be addressed to the project.'
].forEach(P);
out.push(':eedition.', ':toc.');
require('./content.js')({ P, H1, H1plain, H2, H3, B, N, NOTE, ATTN, SCREEN, TABLE, setAppendix });
out.push(':euserdoc.');
process.stdout.write(Buffer.from(out.join('\n') + '\n', 'latin1'));
