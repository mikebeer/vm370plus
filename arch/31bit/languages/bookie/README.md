# bookie: BookMaster markup for VM/370plus (milestone 9.30)

IBM's BookMaster is licensed. **bookie** is our own formatter for its GML
markup. `bookie.py` translates a BookMaster source (`fn SCRIPT`) into troff
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
| Structure | `:userdoc :prolog :title :author :date :docnum :frontm :tipage :toc :body :appendix :backm` |
| Headings and paragraphs | `:h0-:h6` (`id=`), `:p :lp :pc :note`, `:abstract`, `:preface` |
| Highlighting | `:hp0-:hp9`/`:ehpN`, `:q`/`:eq`, `:cit`/`:ecit`, `:xph`/`:exph`, `:fn`/`:efn` |
| Lists | `:ol :ul :sl :li` (nested), `:dl tsize= :dthd :ddhd :dt :dd :edl`, `:gl :gt :gd :egl` |
| Blocks | `:xmp`/`:exmp`, `:lines`/`:elines`, `:fig :figcap :efig`, `:table cols= :thd :ethd :row :c :etable` |
| References | `:hdref refid=`, `:figref refid=` |
| Control words | `.*`, `.sp`, `.br`, `.pa`, `.ce`, `.im` |
| Symbols | `&colon. &semi. &period. &amp. &lt. &gt. &rbl. &bullet. &mdash. &copy. &tm.` and more; `:nameit symbol= text=` defines your own |

`SAMPLE.SCRIPT` exercises most of these.

## Differences from BookMaster

- The table of contents comes at the end of the document (`.TC`, roman
  page numbers), not the front.
- Running heads, figure lists, the index (`:i1.`), revision codes and
  profiles are not done; their tags are ignored.
- Highlighting levels 4-9 repeat 1-3.

## Verified (pd19, 13 October)

- `BOOKIE SAMPLE` gives 198 lines of text.
- `BOOKIE SAMPLE (PS` gives 2 pages of PostScript.
- `PSPRINT SAMPLE` then `tools/psprint.py --pdf` gives a PDF on the PC.
