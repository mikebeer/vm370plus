# bookie: BookMaster markup for VM/370plus (milestone 9.30)

IBM's BookMaster is licensed. **bookie** is our own formatter for its GML
markup. Its default style, **vmplus** (`vmplus.tmac`), is the look of the
VM/370plus Guide:
- A4 pages with 2 cm margins, a Times body and Helvetica headings.
- A title page and an edition notice, both in the IBM manner.
- A contents page with dotted leaders and page numbers. bookie formats the
  document again until those numbers stop changing.
- Chapters titled `Chapter n. Title`, each on a new page under a heavy rule.
- A running head (the short title) and a running foot (the publication
  number and the page).
- Screens in Courier, in a light-grey framed box.
- Tables with grey rules and a Helvetica header row, and `Figure n.`
  captions.
- `Note:` and `Attention:` paragraphs.

`--style ms` gives the plain ms look of the first version. `bookie.py` translates a BookMaster source (`fn SCRIPT`) into troff
(ms macros, tbl for tables), and groff in the Debian guest formats it as
text or PostScript. Ghostscript (9.29) turns the PostScript into PDF or a
printer's format.

On CMS:

    BOOKIE fn [ft [fm]]          -> fn DOCUMENT A   (text)
    BOOKIE fn [ft [fm]] (PS      -> fn PS A          (PostScript)
    PSPRINT fn [(BRIDGE [pdf]]   -> to a printer (9.31)

BOOKIE EXEC resolves `.im` files on CMS (one level deep) and sends the
document through the spool bridge (9.25, the service `bookie`).

## Markup

| Area | Tags |
|---|---|
| Structure | `:userdoc :prolog :title :author :date :docnum :frontm :tipage :toc :body :appendix :backm`; title page (vmplus): `:prodname :subtitle :title :stitle` (running head) `:version :based :docnum :filenum`, `:edition title='...'` ... `:eedition` |
| Headings and paragraphs | `:h0-:h6` (`id=`), `:p :lp :pc :note`, `:abstract`, `:preface` |
| Highlighting | `:hp0-:hp9`/`:ehpN`, `:q`/`:eq`, `:cit`/`:ecit`, `:xph`/`:exph`, `:fn`/`:efn` |
| Lists | `:ol :ul :sl :li` (nested), `:dl tsize= :dthd :ddhd :dt :dd :edl`, `:gl :gt :gd :egl` |
| Blocks | `:xmp`/`:exmp`, `:lines`/`:elines`, `:fig :figcap :efig`, `:table cols= :thd :ethd :row :c :etable` |
| References | `:hdref refid=`, `:figref refid=` |
| Control words | `.*`, `.sp`, `.br`, `.pa`, `.ce`, `.im` |
| Symbols | `&colon. &semi. &period. &amp. &lt. &gt. &rbl. &bullet. &mdash. &copy. &tm.` and more; `:nameit symbol= text=` defines your own |

`SAMPLE.SCRIPT` exercises most of these.

## Differences from BookMaster

- (style ms only) the table of contents comes at the end.
- Running heads, figure lists, the index (`:i1.`), revision codes and
  profiles are not done; their tags are ignored.
- Highlighting levels 4-9 repeat 1-3.

## Verified (pd19, 13 October)

- `BOOKIE SAMPLE` gives 198 lines of text.
- `BOOKIE SAMPLE (PS` gives 2 pages of PostScript.
- `PSPRINT SAMPLE` then `tools/psprint.py --pdf` gives a PDF on the PC.

## Manuals in BookMaster (14 October)

`manual/bookmaster/STATUS.SCRIPT` (VMP-0003, *Status of the System*) and
`manual/bookmaster/BOOKIE.SCRIPT` (VMP-0004, *BOOKIE User's Guide and
Reference*, including how bookie works) are formatted by bookie; both go
on the language disk (next kit). Fixed while making them: the paragraph
after a figure caption stayed in Helvetica bold (`BK-FIGCAP` restored
the wrong font), a word could be printed below the running foot (the
foot's `bp` broke the pending line: now `'bp`), and table text was grey
like the rules (cells are now `\m[black]`).
