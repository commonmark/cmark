#include <stdlib.h>
#include "ext_scanners.h"

bufsize_t _ext_scan_at(bufsize_t (*scanner)(const unsigned char *), unsigned char *ptr, int len, bufsize_t offset)
{
	bufsize_t res;

        if (ptr == NULL || offset > len) {
          return 0;
        } else {
	  unsigned char lim = ptr[len];

	  ptr[len] = '\0';
	  res = scanner(ptr + offset);
	  ptr[len] = lim;
        }

	return res;
}

/*!re2c
  re2c:define:YYCTYPE  = "unsigned char";
  re2c:define:YYCURSOR = p;
  re2c:define:YYMARKER = marker;
  re2c:define:YYCTXMARKER = marker;
  re2c:yyfill:enable = 0;

  spacechar = [ \t\v\f];
  newline = [\r]?[\n];

  table_marker = (spacechar*[:]?[-]+[:]?spacechar*);
*/

bufsize_t _scan_table_start(const unsigned char *p)
{
  const unsigned char *marker = NULL;
  const unsigned char *start = p;
/*!re2c
  [|]? table_marker ([|] table_marker)* [|]? spacechar* newline { return (bufsize_t)(p - start); }
  .? { return 0; }
*/
}
