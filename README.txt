xlsreader - Read XLS files from Hollywood

This plugin exposes one new function to Hollywood scripts : xlsreader.OpenXls()

/* Open an .xls file and:
** 1)If everything went well return its content as a table as described below:
** {
**   ["first tab"] = {
**    (note:first row) {"First column value", "Second column value", "Third column value", ...},
**    (note:second row) {"First column value", "Second column value", "Third column value", ...},
**    (note:third row) {"First column value", "Second column value", "Third column value", ...},
**     ...
**   },
**   ["second tab"] = {
**    (note:first row) {"First column value", "Second column value", "Third column value", ...},
**    (note:second row) {"First column value", "Second column value", "Third column value", ...},
**    (note:third row) {"First column value", "Second column value", "Third column value", ...},
**     ...
**   },
**   ...
** }
**
** 2)If something went wrong raise an error containing message
*/

