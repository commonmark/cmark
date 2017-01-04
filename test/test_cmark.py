# -*- coding: utf8 -*-

from __future__ import unicode_literals

import sys
import os
import unittest
import argparse

here = os.path.abspath(os.path.dirname(__file__))
sys.path.append(os.path.join(here, os.pardir, 'wrappers'))
from wrapper import *

class TestHighLevel(unittest.TestCase):
    def test_markdown_to_html(self):
        self.assertEqual(markdown_to_html('foo'), '<p>foo</p>\n')

    def test_parse_document(self):
        doc = parse_document('foo')
        self.assertEqual(type(doc), Document)

class TestParser(unittest.TestCase):
    def test_lifecycle(self):
        parser = Parser()
        del parser

    def test_feed(self):
        parser = Parser()
        parser.feed('‘')

    def test_finish(self):
        parser = Parser()
        parser.feed('‘')
        doc = parser.finish()

    def test_source_map(self):
        parser = Parser(options=Parser.OPT_SOURCEPOS)
        parser.feed('‘')
        doc = parser.finish()
        source_map = parser.get_source_map()
        extents = [e for e in source_map]
        self.assertEqual(len(extents), 1)
        self.assertEqual(extents[0].type, ExtentType.CONTENT)
        self.assertEqual(extents[0].start, 0)
        self.assertEqual(extents[0].stop, 3)

    def test_render_html(self):
        parser = Parser()
        parser.feed('‘')
        doc = parser.finish()
        res = doc.to_html()
        self.assertEqual(res, '<p>‘</p>\n')

    def test_render_xml(self):
        parser = Parser()
        parser.feed('‘')
        doc = parser.finish()
        res = doc.to_xml()
        self.assertEqual(
            res,
            '<?xml version="1.0" encoding="UTF-8"?>\n'
			'<!DOCTYPE document SYSTEM "CommonMark.dtd">\n'
			'<document xmlns="http://commonmark.org/xml/1.0">\n'
			'  <paragraph>\n'
			'    <text>‘</text>\n'
			'  </paragraph>\n'
			'</document>\n')

    def test_render_commonmark(self):
        parser = Parser()
        parser.feed('‘')
        doc = parser.finish()
        res = doc.to_commonmark()
        self.assertEqual(res, '‘\n')

    def test_render_man(self):
        parser = Parser()
        parser.feed('‘')
        doc = parser.finish()
        res = doc.to_man()
        self.assertEqual(
            res,
            '.PP\n'
            '\[oq]\n')

    def test_render_latex(self):
        parser = Parser()
        parser.feed('‘')
        doc = parser.finish()
        res = doc.to_latex()
        self.assertEqual(res, '`\n')

class TestNode(unittest.TestCase):
    def test_type(self):
        parser = Parser()
        parser.feed('foo')
        doc = parser.finish()
        self.assertEqual(type(doc), Document)

    def test_equal(self):
        parser = Parser()
        parser.feed('foo\n\nbar')
        doc = parser.finish()
        para_one = doc.first_child
        para_two = doc.last_child
        self.assertEqual(doc.last_child, para_one.next)
        self.assertEqual(para_one != para_two, True)

    def test_first_child(self):
        parser = Parser()
        parser.feed('foo')
        doc = parser.finish()
        child1 = doc.first_child
        child2 = doc.first_child
        self.assertEqual(child1, child2)
        self.assertEqual((child1 != child2), False)

    def test_last_child(self):
        parser = Parser()
        parser.feed('foo')
        doc = parser.finish()
        child1 = doc.first_child
        child2 = doc.last_child
        self.assertEqual(child1, child2)
        self.assertEqual((child1 != child2), False)

    def test_next(self):
        parser = Parser()
        parser.feed('foo *bar*')
        doc = parser.finish()
        para = doc.first_child
        self.assertEqual(type(para), Paragraph)
        text = para.first_child
        self.assertEqual(type(text), Text)
        emph = text.next
        self.assertEqual(type(emph), Emph)
        self.assertEqual(para.next, None)

    def test_previous(self):
        parser = Parser()
        parser.feed('foo *bar*')
        doc = parser.finish()
        para = doc.first_child
        text = para.first_child
        emph = text.next
        self.assertEqual(emph.previous, text)
        self.assertEqual(para.previous, None)

    def test_children(self):
        parser = Parser()
        parser.feed('foo *bar*')
        doc = parser.finish()
        para = doc.first_child
        children = [c for c in para]
        self.assertEqual(len(children), 2)
        self.assertEqual(type(children[0]), Text)
        self.assertEqual(type(children[1]), Emph)

        # Test unlinking while iterating

        children = []
        for c in para:
            children.append(c)
            c.unlink()

        self.assertEqual(len(children), 2)
        self.assertEqual(type(children[0]), Text)
        self.assertEqual(type(children[1]), Emph)

    def test_parent(self):
        parser = Parser()
        parser.feed('foo')
        doc = parser.finish()
        para = doc.first_child
        self.assertEqual(para.parent, doc)

    def test_new(self):
        with self.assertRaises(NotImplementedError):
            n = Node()

    def test_unlink(self):
        parser = Parser()
        parser.feed('foo *bar*')
        doc = parser.finish()
        para = doc.first_child
        para.unlink()
        self.assertEqual(doc.to_html(), '')

    def test_append_child(self):
        parser = Parser()
        parser.feed('')
        doc = parser.finish()
        doc.append_child(Paragraph())
        self.assertEqual(doc.to_html(), '<p></p>\n')
        with self.assertRaises(LibcmarkError):
            doc.append_child(Text(literal='foo'))

    def test_prepend_child(self):
        parser = Parser()
        parser.feed('foo')
        doc = parser.finish()
        doc.prepend_child(Paragraph())
        self.assertEqual(doc.to_html(), '<p></p>\n<p>foo</p>\n')
        with self.assertRaises(LibcmarkError):
            doc.prepend_child(Text(literal='foo'))

    def test_insert_before(self):
        parser = Parser()
        parser.feed('foo')
        doc = parser.finish()
        para = doc.first_child
        para.insert_before(Paragraph())
        self.assertEqual(doc.to_html(), '<p></p>\n<p>foo</p>\n')
        with self.assertRaises(LibcmarkError):
            para.insert_before(Text(literal='foo'))

    def test_insert_after(self):
        parser = Parser()
        parser.feed('foo')
        doc = parser.finish()
        para = doc.first_child
        para.insert_after(Paragraph())
        self.assertEqual(doc.to_html(), '<p>foo</p>\n<p></p>\n')
        with self.assertRaises(LibcmarkError):
            para.insert_after(Text(literal='foo'))

    def test_consolidate_text_nodes(self):
        parser = Parser()
        parser.feed('foo **bar*')
        doc = parser.finish()
        self.assertEqual(len([c for c in doc.first_child]), 3)
        doc.consolidate_text_nodes()
        self.assertEqual(len([c for c in doc.first_child]), 2)

class TestLiteral(unittest.TestCase):
    def test_text(self):
        parser = Parser()
        parser.feed('foo')
        doc = parser.finish()
        para = doc.first_child
        self.assertEqual(type(para), Paragraph)
        text = para.first_child
        self.assertEqual(type(text), Text)
        self.assertEqual(text.literal, 'foo')
        text.literal = 'bar'
        self.assertEqual(text.to_html(), 'bar')

class TestDocument(unittest.TestCase):
    def test_new(self):
        doc = Document()
        self.assertEqual(doc.to_html(),
                         '')

class TestBlockQuote(unittest.TestCase):
    def test_new(self):
        bq = BlockQuote()
        self.assertEqual(bq.to_html(),
                         '<blockquote>\n</blockquote>\n')

class TestList(unittest.TestCase):
    def test_new(self):
        list_ = List()
        self.assertEqual(list_.to_html(),
                         '<ul>\n</ul>\n')

    def test_type(self):
        parser = Parser()
        parser.feed('* foo')
        doc = parser.finish()
        list_ = doc.first_child
        self.assertEqual(type(list_), List)
        self.assertEqual(list_.type, ListType.BULLET)
        list_.type = ListType.ORDERED
        self.assertEqual(doc.to_html(),
                         '<ol>\n'
                         '<li>foo</li>\n'
                         '</ol>\n')

    def test_start(self):
        parser = Parser()
        parser.feed('2. foo')
        doc = parser.finish()
        list_ = doc.first_child
        self.assertEqual(type(list_), List)
        self.assertEqual(list_.start, 2)
        list_.start = 1
        self.assertEqual(doc.to_commonmark(),
                         '1.  foo\n')
        with self.assertRaises(LibcmarkError):
            list_.start = -1
        list_.type = ListType.BULLET

    def test_delim(self):
        parser = Parser()
        parser.feed('1. foo')
        doc = parser.finish()
        list_ = doc.first_child
        self.assertEqual(type(list_), List)
        self.assertEqual(list_.delim, '.')
        list_.delim = ')'
        self.assertEqual(doc.to_commonmark(),
                         '1)  foo\n')

    def test_tight(self):
        parser = Parser()
        parser.feed('* foo\n'
                    '\n'
                    '* bar\n')
        doc = parser.finish()
        list_ = doc.first_child
        self.assertEqual(type(list_), List)
        self.assertEqual(list_.tight, False)
        self.assertEqual(doc.to_commonmark(),
                         '  - foo\n'
                         '\n'
                         '  - bar\n')

        list_.tight = True
        self.assertEqual(doc.to_commonmark(),
                         '  - foo\n'
                         '  - bar\n')

        with self.assertRaises(LibcmarkError):
            list_.tight = 42

class TestItem(unittest.TestCase):
    def test_new(self):
        item = Item()
        self.assertEqual(item.to_html(),
                         '<li></li>\n')

class TestCodeBlock(unittest.TestCase):
    def test_new(self):
        cb = CodeBlock(literal='foo', fence_info='python')
        self.assertEqual(cb.to_html(),
                         '<pre><code class="language-python">foo</code></pre>\n')

    def test_fence_info(self):
        parser = Parser()
        parser.feed('``` markdown\n'
                    'hello\n'
                    '```\n')
        doc = parser.finish()
        code_block = doc.first_child
        self.assertEqual(type(code_block), CodeBlock) 
        self.assertEqual(code_block.fence_info, 'markdown')
        code_block.fence_info = 'python'
        self.assertEqual(doc.to_commonmark(),
                         '``` python\n'
                         'hello\n'
                         '```\n')

class TestHtmlBlock(unittest.TestCase):
    def test_new(self):
        hb = HtmlBlock(literal='<p>foo</p>')
        self.assertEqual(hb.to_html(),
                         '<p>foo</p>\n')

class TestCustomBlock(unittest.TestCase):
    def test_new(self):
        cb = CustomBlock()
        self.assertEqual(cb.to_html(),
                         '')

class TestParagraph(unittest.TestCase):
    def test_new(self):
        para = Paragraph()
        self.assertEqual(para.to_html(),
                '<p></p>\n')

class TestHeading(unittest.TestCase):
    def test_new(self):
        heading = Heading(level=3)
        self.assertEqual(heading.to_html(),
                '<h3></h3>\n')

    def test_level(self):
        parser = Parser()
        parser.feed('# foo')
        doc = parser.finish()
        heading = doc.first_child
        self.assertEqual(type(heading), Heading)
        self.assertEqual(heading.level, 1)
        heading.level = 3
        self.assertEqual(heading.level, 3)

        self.assertEqual(doc.to_html(),
                         '<h3>foo</h3>\n')

        with self.assertRaises(LibcmarkError):
            heading.level = 10

class TestThematicBreak(unittest.TestCase):
    def test_new(self):
        tb = ThematicBreak()
        self.assertEqual(tb.to_html(),
                '<hr />\n')

class TestText(unittest.TestCase):
    def test_new(self):
        text = Text(literal='foo')
        self.assertEqual(text.to_html(),
                         'foo')

class TestSoftBreak(unittest.TestCase):
    def test_new(self):
        sb = SoftBreak()
        self.assertEqual(sb.to_html(), '\n')
        self.assertEqual(sb.to_html(options=Parser.OPT_HARDBREAKS),
                         '<br />\n')
        self.assertEqual(sb.to_html(options=Parser.OPT_NOBREAKS),
                         ' ')

class TestLineBreak(unittest.TestCase):
    def test_new(self):
        lb = LineBreak()
        self.assertEqual(lb.to_html(), '<br />\n')

class TestCode(unittest.TestCase):
    def test_new(self):
        code = Code(literal='bar')
        self.assertEqual(code.to_html(), '<code>bar</code>')

class TestHtmlInline(unittest.TestCase):
    def test_new(self):
        hi = HtmlInline(literal='<b>baz</b>')
        self.assertEqual(hi.to_html(), '<b>baz</b>')

class TestCustomInline(unittest.TestCase):
    def test_new(self):
        ci = CustomInline()
        self.assertEqual(ci.to_html(),
                         '')

class TestEmph(unittest.TestCase):
    def test_new(self):
        emph = Emph()
        self.assertEqual(emph.to_html(),
                         '<em></em>')

class TestStrong(unittest.TestCase):
    def test_new(self):
        strong = Strong()
        self.assertEqual(strong.to_html(),
                         '<strong></strong>')

class TestLink(unittest.TestCase):
    def test_new(self):
        link = Link(url='http://foo.com', title='foo')
        self.assertEqual(link.to_html(),
                         '<a href="http://foo.com" title="foo"></a>')

    def test_url(self):
        parser = Parser()
        parser.feed('<http://foo.com>\n')
        doc = parser.finish()
        para = doc.first_child
        self.assertEqual(type(para), Paragraph)
        link = para.first_child
        self.assertEqual(type(link), Link)
        self.assertEqual(link.url, 'http://foo.com')
        link.url = 'http://bar.net'
        # Yeah that's crappy behaviour but not our problem here
        self.assertEqual(doc.to_commonmark(),
                         '[http://foo.com](http://bar.net)\n')

    def test_title(self):
        parser = Parser()
        parser.feed('<http://foo.com>\n')
        doc = parser.finish()
        para = doc.first_child
        self.assertEqual(type(para), Paragraph)
        link = para.first_child
        self.assertEqual(type(link), Link)
        self.assertEqual(link.title, '')
        link.title = 'foo'
        self.assertEqual(doc.to_html(),
                         '<p><a href="http://foo.com" title="foo">http://foo.com</a></p>\n')

class TestImage(unittest.TestCase):
    def test_new(self):
        image = Image(url='http://foo.com', title='foo')
        self.assertEqual(image.to_html(),
                         '<img src="http://foo.com" alt="" title="foo" />')

    def test_url(self):
        parser = Parser()
        parser.feed('![image](image.com)\n')
        doc = parser.finish()
        para = doc.first_child
        self.assertEqual(type(para), Paragraph)
        link = para.first_child
        self.assertEqual(type(link), Image)
        self.assertEqual(link.url, 'image.com')
        link.url = 'http://bar.net'
        self.assertEqual(doc.to_commonmark(),
                         '![image](http://bar.net)\n')

    def test_title(self):
        parser = Parser()
        parser.feed('![image](image.com "ze image")\n')
        doc = parser.finish()
        para = doc.first_child
        self.assertEqual(type(para), Paragraph)
        image = para.first_child
        self.assertEqual(type(image), Image)
        self.assertEqual(image.title, 'ze image')
        image.title = 'foo'
        self.assertEqual(doc.to_html(),
                         '<p><img src="image.com" alt="image" title="foo" /></p>\n')

if __name__=='__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('libdir')
    args = parser.parse_known_args()
    conf.set_library_path(args[0].libdir)
    unittest.main(argv=[sys.argv[0]] + args[1])
