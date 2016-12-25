from wrapper import *
import re
from collections import defaultdict

class RemarkorError(LibcmarkError):
    pass

def pretty_print_extents(source_map):
    for extent in source_map:
        print ('%d-%d %s for %s' % (extent.start, extent.stop, extent.type, type(extent.node)))

ESCAPE_REGEX = re.compile('^' # Start of the string
                          '(' # The potential problematic pattern
                          '[' # Any of these characters
                          '#' # A heading
                          '|>' # Or a blockquote
                          '|*|+|-' # Or an unordered list start
                          ']' # End of single characters
                          '|[0-9]+[.|)]' # Ordered list start
                          ')' # End of the problematic pattern
                          '('
                          '[ ]+.*'
                          '|$)'
                          )

ESCAPE_THEMATIC_REGEX = re.compile('^' # Start of the string
                                   '((\*\s*){3,}|(\-\s*){3,}|(_\s*){3,})' # Either '*' or '-' or '_' 3 times or more, ws allowed
                                   '$' # Nothing else is allowed
                                   )

ESCAPE_CODE_BLOCK_REGEX = re.compile('^' # Start of the string
                                     '(`{3,}|~{3,})' # Either '`' or `~`  3 times or more
                                     '[^`]*' # Anything but '`'
                                     '$' # Nothing else is allowed
                                     )

ESCAPE_SETEXT_REGEX = re.compile('^' # Start of the string
                                 '(\-+|=+)' # Either '-' or '=' one time or more
                                 '[ ]*' # Optionally followed by 0 or more whitespace characters
                                 '$' # Nothing else is allowed
                                 )

ESCAPE_REFERENCE_DEF_REGEX = re.compile('^' # Start of the string
                                        '\[' # Opening '['
                                        '.*' # Anything
                                        '\]' # Closing ']'
                                        ':' # Literal ':'
                                        '.*' # Consume the remainder
                                        )

def build_reverse_source_map(source_map):
    rmap = defaultdict(list)
    for ex in source_map:
        rmap[ex.node].append(ex)
    return rmap

class Remarkor:
    def __init__(self, contents):
        self.dump_context = None
        if type(contents) == str:
            self.source = contents.encode('utf8')
        else:
            assert type(contents) == bytes
            self.source = contents

    def remark(self, width=80, validate=True):
        self.__reset(width)

        self.__dump(self.root_node, '')
        self.need_cr = 1
        self.__flush('', '')

        res = '\n'.join(self.result)

        if validate:
            self.__validate(res)

        return res

    @staticmethod
    def from_filename(filename):
        with open(filename, 'rb') as _:
            contents = _.read()
        return Remarkor(contents)

    def __reset(self, width):
        self.parser = Parser(options=Parser.OPT_SOURCEPOS)
        self.parser.feed(self.source)
        self.root_node = self.parser.finish()
        self.source_map = self.parser.get_source_map()
        self.rmap = build_reverse_source_map(self.source_map)

        # List of lines
        self.result = ['']
        # Number of new lines to insert before flushing new content
        self.need_cr = 0
        # Whether to insert 1 or 2 new lines before the next item
        self.in_tight_list = False
        # Workaround for indented lists, which are not reliably breakable by
        # any block (in particular indented code)
        # FIXME: Ask why this case is even part of the spec, because afaiu it's just broken
        self.break_out_of_list = False
        # Maximum number of columns
        self.width = width
        # Whether flush operations can break lines
        self.flush_can_break = True
        # The offset in the last line to check escape from
        self.last_line_content_offset = 0
        # If we break the line when rendering this node, escape the last character
        self.escape_link_if_breaking = None
        # Do not try to escape anything
        self.no_escape = False
        # Do not try to escape html blocks, <url> type link
        self.no_escape_html_block = False

    def __normalize_texts(self, node):
        if type(node) == Text:
            node.literal = ' '.join(node.literal.split())
            if not node.literal:
                node.unlink()
        for c in node:
            self.__normalize_texts(c)

    def __strip_blanks(self, node):
        if type(node) == SoftBreak:
            node.insert_after(Text(literal=' '))
            node.unlink()
            return None
        elif type(node) == HtmlBlock:
            if node.literal.strip() == "<!-- end list -->":
                node.unlink()
        for c in node:
            self.__strip_blanks(c)

    # This method compares the result with the original AST, stripping
    # all blank nodes, all html end-list workaround blocks, and
    # consolidating and normalizing text nodes.
    def __validate(self, res):
        parser = Parser()
        parser.feed(res)
        new_root_node = parser.finish()

        self.__strip_blanks(self.root_node)
        self.__strip_blanks(new_root_node)
        self.root_node.consolidate_text_nodes()
        new_root_node.consolidate_text_nodes()
        self.__normalize_texts(self.root_node)
        self.__normalize_texts(new_root_node)
        if self.root_node.to_xml() != new_root_node.to_xml():
            raise RemarkorError('Refactoring changed the AST !')

    def __utf8(self, start, stop):
        return self.source[start:stop].decode('utf8')

    def __get_extent_utf8(self, extent):
        if extent:
            return self.__utf8(extent.start, extent.stop)
        return ''

    def __get_closer_utf8(self, node):
        for ex in reversed(self.rmap[node]):
            if ex.type == ExtentType.CLOSER:
                return self.__get_extent_utf8(ex)
        return ''
        return self.__get_extent_utf8(self.get_closer(node))

    def __get_opener_utf8(self, node):
        for ex in self.rmap[node]:
            if ex.type == ExtentType.OPENER:
                return self.__get_extent_utf8(ex)
        return ''

    def __breakup_contents(self, node):
        skip_next_ws = False
        token = ''
        extents = self.rmap[node]

        is_text = type(node) is Text
        is_escaped = False

        if is_text:
            while node.next:
                node = node.next
                if type(node) is not Text:
                    break
                extents += self.rmap[node]
                self.rmap[node] = []

        def sanitize(token):
            if is_text:
                if type(node) is Link and re.match('.*\[.*\]$', token):
                    self.escape_link_if_breaking = node
            return token

        for ex in extents:
            if ex.type != ExtentType.CONTENT:
                continue
            for c in self.__utf8(ex.start, ex.stop):
                if c == ' ' and not is_escaped:
                    if token:
                        yield token
                        token = ''
                    if not skip_next_ws:
                        yield ' '
                        skip_next_ws = True
                else:
                    token += c
                    skip_next_ws = False
                if c == '\\':
                    is_escaped = not is_escaped
                else:
                    is_escaped = False
        if token:
            yield sanitize(token)

    def __blankline(self):
        self.need_cr = 2

    def __cr(self):
        self.need_cr = max(self.need_cr, 1)

    def __check_escape(self):
        if self.no_escape:
            self.no_escape = False
            return

        prefix = self.result[-1][:self.last_line_content_offset]
        unprefixed = self.result[-1][self.last_line_content_offset:]
        m = re.match(ESCAPE_REGEX, unprefixed)
        if (m):
            try:
                first_space = unprefixed.index(' ')
            except ValueError:
                first_space = len(unprefixed)
            self.result[-1] = '%s%s\\%s' % (prefix,
                    unprefixed[0:first_space - 1],
                    unprefixed[first_space - 1:])
            return

        m = re.match(ESCAPE_THEMATIC_REGEX, unprefixed)
        if (m):
            self.result[-1] = '%s\\%s' % (prefix, unprefixed)
            return

        m = re.match(ESCAPE_CODE_BLOCK_REGEX, unprefixed)
        if (m):
            self.result[-1] = '%s\\%s' % (prefix, unprefixed)
            return

        m = re.match(ESCAPE_SETEXT_REGEX, unprefixed)
        if (m):
            self.result[-1] = '%s\\%s' % (prefix, unprefixed)
            return

        m = re.match(ESCAPE_REFERENCE_DEF_REGEX, unprefixed)
        if (m):
            self.result[-1] = '%s\\%s' % (prefix, unprefixed)
            return

        # FIXME: certainly very expensive, but as we make it so
        # html inlines can never start a line, it is at least
        # safe and correct
        if not self.no_escape_html_block:
            root_node = parse_document(unprefixed)
            if type(root_node.first_child) in [HtmlBlock, Reference]:
                self.result[-1] = '%s\\%s' % (prefix, unprefixed)
        self.no_escape_html_block = False

    def __check_prefix(self, prefix):
        if not self.result[-1]:
            self.result[-1] = prefix
            self.last_line_content_offset = len(prefix)

    def __flush(self, prefix, utf8, escape_if_breaking=0):
        if self.in_tight_list:
            self.need_cr = min(self.need_cr, 1)

        while self.need_cr:
            self.__check_prefix(prefix)
            self.__check_escape()
            self.result.append('')
            self.need_cr -= 1

        self.__check_prefix(prefix)

        if (utf8 and
                self.flush_can_break and
                len(self.result[-1]) > self.last_line_content_offset and
                len(self.result[-1]) + len(utf8) >= self.width):
            self.result[-1] = self.result[-1].rstrip(' ')
            self.__check_escape()
            if escape_if_breaking:
                self.result[-1] = "%s\\%s" % (self.result[-1][:escape_if_breaking],
                                                  self.result[-1][escape_if_breaking:])
            self.result.append('')
            self.__check_prefix(prefix)
            if utf8 == ' ':
                return

        self.result[-1] += utf8

    def __dump(self, node, prefix):
        old_in_tight_list = self.in_tight_list
        old_break_out_of_list = self.break_out_of_list
        old_flush_can_break = self.flush_can_break

        opener_utf8 = self.__get_opener_utf8(node).strip()

        if type(node) is BlockQuote:
            self.__flush(prefix, opener_utf8 + ' ')
            self.last_line_content_offset = len(opener_utf8 + ' ' + prefix)
            prefix += opener_utf8 + ' '
        elif type(node) is Heading:
            self.flush_can_break = False
            if (opener_utf8):
                self.__flush(prefix, opener_utf8 + ' ')
            self.no_escape = True
        elif type(node) is Item:
            opener_utf8_with_blank = ''
            last_stop = -1
            # Very awkward, see list item indentation tests
            for ex in self.rmap[node]:
                if last_stop != -1 and ex.start != last_stop:
                    break
                last_stop = ex.stop
                opener_utf8_with_blank += self.__get_extent_utf8(ex)
            if len(opener_utf8_with_blank) > 4:
                self.break_out_of_list = True

            self.__flush(prefix, opener_utf8 + ' ')
            self.last_line_content_offset = len(opener_utf8 + ' ' + prefix)
            # Only setting here to make sure the call to flush was made with
            # the right tightness.
            self.in_tight_list = node.parent.tight
            prefix += (len(opener_utf8) + 1) * ' '
        elif type(node) in [CodeBlock, HtmlBlock]:
            self.flush_can_break = False
            for ex in self.rmap[node]:
                utf8 = self.__get_extent_utf8(ex)
                self.__flush(prefix, utf8.rstrip('\r\n'))
                self.no_escape = True
                # Make sure to prefix the next line
                if utf8.endswith('\n'):
                    self.__cr()
            self.flush_can_break = old_flush_can_break
            self.__blankline()
        elif type(node) is ThematicBreak:
            self.flush_can_break = False
            utf8 = ' '.join(self.__breakup_contents(node)).rstrip('\r\n')
            self.__flush(prefix, utf8)
            self.no_escape = True
            # Make sure to prefix the next line
            if utf8.endswith('\n'):
                self.__cr()
            self.flush_can_break = old_flush_can_break
            self.__blankline()
        elif type(node) is Reference:
            self.flush_can_break = False
            for ex in self.rmap[node]:
                utf8 = self.__get_extent_utf8(ex)
                self.__flush(prefix, utf8)
                if ex.type == ExtentType.OPENER:
                    self.no_escape = True
            self.flush_can_break = old_flush_can_break
            if type(node.next) is Reference: # Keep reference lists tight
                self.__cr()
            else:
                self.__blankline()
        elif type(node) is Text:
            for word in self.__breakup_contents(node):
                self.__flush(prefix, word)
        elif type(node) is SoftBreak:
            self.__flush(prefix, ' ')
        elif type(node) is LineBreak:
            self.flush_can_break = False
            content = ''.join([self.__get_extent_utf8(ex).rstrip('\r\n') for ex in self.rmap[node]])
            # Keep the source hardbreak style
            if '\\' in content:
                self.__flush(prefix, content)
            else:
                self.__flush(prefix, '  ')
            self.flush_can_break = old_flush_can_break
            self.__cr()
        elif type(node) in [Emph, Strong]:
            self.__flush(prefix, opener_utf8)
            if self.result[-1] == prefix + opener_utf8:
                self.no_escape = True
            self.flush_can_break = False
        elif type(node) in [Link, Image]:
            if self.escape_link_if_breaking == node:
                self.__flush(prefix, opener_utf8, escape_if_breaking=-1)
            else:
                self.__flush(prefix, opener_utf8)
            if self.result[-1] == prefix + opener_utf8:
                self.no_escape = True
        elif type(node) is HtmlInline:
            self.flush_can_break = False
            for ex in self.rmap[node]:
                utf8 = self.__get_extent_utf8(ex)
                self.__flush(prefix, utf8.rstrip('\r\n'))
                if utf8.endswith('\n'):
                    self.__cr()
            self.flush_can_break = old_flush_can_break
        elif type(node) is Code:
            for ex in self.rmap[node]:
                utf8 = self.__get_extent_utf8(ex)
                self.__flush(prefix, utf8.rstrip('\r\n'))
                if utf8.endswith('\n'):
                    self.__cr()

        for child in node:
            tmp_flush_can_break = self.flush_can_break
            tmp_node = child

            # See __breakup_contents
            while type(tmp_node) is Text and type(tmp_node.next) is Text:
                tmp_node = tmp_node.next

            if type(tmp_node.next) is HtmlInline or type(tmp_node.previous) is HtmlInline:
                self.flush_can_break = False
            self.__dump(child, prefix)
            self.flush_can_break = tmp_flush_can_break

        if type(node) in [Emph, Strong]:
            self.__flush(prefix, self.__get_closer_utf8(node).rstrip('\r\n'))
            self.flush_can_break = old_flush_can_break
        elif type(node) is List:
            self.in_tight_list = old_in_tight_list
            if self.break_out_of_list:
                self.__cr()
                self.__flush(prefix, "<!-- end list -->")
                self.no_escape = True
                self.__cr()
            self.break_out_of_list = old_break_out_of_list
        elif type(node) is Heading:
            for ex in self.rmap[node]:
                if ex.type != ExtentType.OPENER:
                    utf8 = self.__get_extent_utf8(ex)
                    self.__flush(prefix, utf8.rstrip('\r\n'))
                    self.no_escape = True
                    if utf8.endswith('\n'):
                        self.__cr()
            self.flush_can_break = old_flush_can_break
        elif type(node) in [Link, Image]:
            for ex in self.rmap[node]:
                if ex.type != ExtentType.OPENER:
                    self.flush_can_break = old_flush_can_break
                    utf8 = self.__get_extent_utf8(ex).strip(' \r\n')
                    if ex.type == ExtentType.PUNCTUATION and prev_extent.type == ExtentType.PUNCTUATION:
                        self.flush_can_break = False
                    elif ex.type == ExtentType.LINK_TITLE:
                        self.__flush(prefix, ' ')
                    if ex.type != ExtentType.BLANK:
                        self.__flush(prefix, utf8)
                    if ex.type == ExtentType.LINK_DESTINATION:
                        if self.result[-1] == prefix + utf8 and re.match('^<.*>$', utf8):
                            self.no_escape_html_block = True
                prev_extent = ex
            self.flush_can_break = old_flush_can_break

        if type(node) in [Paragraph, List, BlockQuote, Item, Heading, Document]:
            self.__blankline() 
