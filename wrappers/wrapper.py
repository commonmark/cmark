from __future__ import unicode_literals

from ctypes import *
import sys
import platform

c_object_p = POINTER(c_void_p)

if sys.version_info[0] > 2:
    def bytes_and_length(text):
        if type(text) == str:
            text = text.encode("utf8")
        return text, len(text)
else:
    def bytes_and_length(text):
        if type(text) == unicode:
            text = text.encode("utf8")
        return text, len(text)

def unicode_from_char_p(res, fn, args):
    ret = res.decode("utf8")
    return ret

class owned_char_p(c_void_p):
    def __del__(self):
        conf.lib.cmark_default_mem_free(self.value)

def unicode_from_owned_char_p(res, fn, args):
    ret = cast(res, c_char_p).value.decode("utf8")
    return ret

def boolean_from_result(res, fn, args):
    return bool(res)

def delim_from_int(res, fn, args):
    if res == 0:
        return ''
    elif res == 1:
        return '.'
    elif res == 2:
        return ')'

class BaseEnumeration(object):
    def __init__(self, value):
        if value >= len(self.__class__._kinds):
            self.__class__._kinds += [None] * (value - len(self.__class__._kinds) + 1)
        if self.__class__._kinds[value] is not None:
            raise ValueError('{0} value {1} already loaded'.format(
                str(self.__class__), value))
        self.value = value
        self.__class__._kinds[value] = self
        self.__class__._name_map = None

    def from_param(self):
        return self.value

    @classmethod
    def from_id(cls, id, fn, args):
        if id >= len(cls._kinds) or cls._kinds[id] is None:
            raise ValueError('Unknown template argument kind %d' % id)
        return cls._kinds[id]

    @property
    def name(self):
        """Get the enumeration name of this cursor kind."""
        if self._name_map is None:
            self._name_map = {}
            for key, value in self.__class__.__dict__.items():
                if isinstance(value, self.__class__):
                    self._name_map[value] = key
        return str(self._name_map[self])

    def __repr__(self):
        return '%s.%s' % (self.__class__.__name__, self.name,)

class Parser(object):
    OPT_DEFAULT = 0
    OPT_SOURCEPOS = 1 << 1
    OPT_HARDBREAKS = 1 << 2
    OPT_SAFE = 1 << 3
    OPT_NOBREAKS = 1 << 4
    OPT_NORMALIZE = 1 << 8
    OPT_VALIDATE_UTF8 = 1 << 9
    OPT_SMART = 1 << 10

    def __init__(self, options=0):
        self._parser = conf.lib.cmark_parser_new(options)

    def __del__(self):
        conf.lib.cmark_parser_free(self._parser)

    def feed(self, text):
        conf.lib.cmark_parser_feed(self._parser, *bytes_and_length(text))

    def finish(self):
        return conf.lib.cmark_parser_finish(self._parser)

    def get_source_map(self):
        return conf.lib.cmark_parser_get_first_source_extent(self._parser)

class LibcmarkError(Exception):
    def __init__(self, message):
        self.m = message

    def __str__(self):
        return self.m

class NodeType(BaseEnumeration):
    _kinds = []
    _name_map = None

# FIXME: a bit awkward to update, not sure what the best practice is
NodeType.NONE = NodeType(0)
NodeType.DOCUMENT = NodeType(1)
NodeType.BLOCK_QUOTE = NodeType(2)
NodeType.LIST = NodeType(3)
NodeType.ITEM = NodeType(4)
NodeType.CODE_BLOCK = NodeType(5)
NodeType.HTML_BLOCK = NodeType(6)
NodeType.CUSTOM_BLOCK = NodeType(7)
NodeType.PARAGRAPH = NodeType(8)
NodeType.HEADING = NodeType(9)
NodeType.THEMATIC_BREAK = NodeType(10)
NodeType.REFERENCE = NodeType(11)
NodeType.TEXT = NodeType(12)
NodeType.SOFTBREAK = NodeType(13)
NodeType.LINEBREAK = NodeType(14)
NodeType.CODE = NodeType(15)
NodeType.HTML_INLINE = NodeType(16)
NodeType.CUSTOM_INLINE = NodeType(17)
NodeType.EMPH = NodeType(18)
NodeType.STRONG = NodeType(19)
NodeType.LINK = NodeType(20)
NodeType.IMAGE = NodeType(21)

class ListType(BaseEnumeration):
    _kinds = []
    _name_map = None

ListType.BULLET = ListType(1)
ListType.ORDERED = ListType(2)

class Node(object):
    __subclass_map = {}

    def __init__(self):
        self._owned = False
        raise NotImplementedError

    @staticmethod
    def from_result(res, fn=None, args=None):
        try:
            res.contents
        except ValueError:
            return None
        cls = Node.get_subclass_map()[conf.lib.cmark_node_get_type(res)]

        ret = cls.__new__(cls)
        ret._node = res
        ret._owned = False
        return ret

    @classmethod
    def get_subclass_map(cls):
        if cls.__subclass_map:
            return cls.__subclass_map

        res = {c._node_type: c for c in cls.__subclasses__()}

        for c in cls.__subclasses__():
            res.update(c.get_subclass_map())

        return res

    def unlink(self):
        conf.lib.cmark_node_unlink(self._node)
        self._owned = True

    def append_child(self, child):
        res = conf.lib.cmark_node_append_child(self._node, child._node)
        if not res:
            raise LibcmarkError("Can't append child %s to node %s" % (str(child), str(self)))
        child._owned = False

    def prepend_child(self, child):
        res = conf.lib.cmark_node_prepend_child(self._node, child._node)
        if not res:
            raise LibcmarkError("Can't prepend child %s to node %s" % (str(child), str(self)))
        child._owned = False

    def insert_before(self, sibling):
        res = conf.lib.cmark_node_insert_before(self._node, sibling._node)
        if not res:
            raise LibcmarkError("Can't insert sibling %s before node %s" % (str(sibling), str(self)))
        sibling._owned = False

    def insert_after(self, sibling):
        res = conf.lib.cmark_node_insert_after(self._node, sibling._node)
        if not res:
            raise LibcmarkError("Can't insert sibling %s after node %s" % (str(sibling), str(self)))
        sibling._owned = False

    def consolidate_text_nodes(self):
        conf.lib.cmark_consolidate_text_nodes(self._node)

    def to_html(self, options=Parser.OPT_DEFAULT):
        return conf.lib.cmark_render_html(self._node, options)

    def to_xml(self, options=Parser.OPT_DEFAULT):
        return conf.lib.cmark_render_xml(self._node, options)

    def to_commonmark(self, options=Parser.OPT_DEFAULT, width=0):
        return conf.lib.cmark_render_commonmark(self._node, options, width)

    def to_man(self, options=Parser.OPT_DEFAULT, width=0):
        return conf.lib.cmark_render_man(self._node, options, width)

    def to_latex(self, options=Parser.OPT_DEFAULT, width=0):
        return conf.lib.cmark_render_latex(self._node, options, width)

    @property
    def parent(self):
        return conf.lib.cmark_node_parent(self._node)

    @property
    def first_child(self):
        return conf.lib.cmark_node_first_child(self._node)

    @property
    def last_child(self):
        return conf.lib.cmark_node_last_child(self._node)

    @property
    def next(self):
        return conf.lib.cmark_node_next(self._node)

    @property
    def previous(self):
        return conf.lib.cmark_node_previous(self._node)

    def __eq__(self, other):
        if other is None:
            return False
        return addressof(self._node.contents) == addressof(other._node.contents)

    def __ne__(self, other):
        if other is None:
            return True
        return addressof(self._node.contents) != addressof(other._node.contents)

    def __hash__(self):
        return hash(addressof(self._node.contents))

    def __del__(self):
        if self._owned:
            conf.lib.cmark_node_free(self._node)

    def __iter__(self):
        cur = self.first_child
        while (cur):
            next_ = cur.next
            yield cur
            cur = next_

class Literal(Node):
    _node_type = NodeType.NONE

    @property
    def literal(self):
        return conf.lib.cmark_node_get_literal(self._node)

    @literal.setter
    def literal(self, value):
        bytes_, _ = bytes_and_length(value)
        if not conf.lib.cmark_node_set_literal(self._node, bytes_):
            raise LibcmarkError("Invalid literal %s\n" % str(value))

class Document(Node):
    _node_type = NodeType.DOCUMENT

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True

class BlockQuote(Node):
    _node_type = NodeType.BLOCK_QUOTE

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True

class List(Node):
    _node_type = NodeType.LIST

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True

    @property
    def type(self):
        return conf.lib.cmark_node_get_list_type(self._node)

    @type.setter
    def type(self, type_):
        if not conf.lib.cmark_node_set_list_type(self._node, type_.value):
            raise LibcmarkError("Invalid type %s" % str(type_))

    @property
    def delim(self):
        return conf.lib.cmark_node_get_list_delim(self._node)

    @delim.setter
    def delim(self, value):
        if value == '.':
            delim_type = 1
        elif value == ')':
            delim_type = 2
        else:
            raise LibcmarkError('Invalid delim type %s' % str(value))

        conf.lib.cmark_node_set_list_delim(self._node, delim_type)

    @property
    def start(self):
        return conf.lib.cmark_node_get_list_start(self._node)

    @start.setter
    def start(self, value):
        if not conf.lib.cmark_node_set_list_start(self._node, value):
            raise LibcmarkError("Invalid list start %s\n" % str(value))

    @property
    def tight(self):
        return conf.lib.cmark_node_get_list_tight(self._node)

    @tight.setter
    def tight(self, value):
        if value is True:
            tightness = 1
        elif value is False:
            tightness = 0
        else:
            raise LibcmarkError("Invalid list tightness %s\n" % str(value))
        if not conf.lib.cmark_node_set_list_tight(self._node, tightness):
            raise LibcmarkError("Invalid list tightness %s\n" % str(value))

class Item(Node):
    _node_type = NodeType.ITEM

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True

class CodeBlock(Literal):
    _node_type = NodeType.CODE_BLOCK

    def __init__(self, literal='', fence_info=''):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True
        self.literal = literal
        self.fence_info = fence_info

    @property
    def fence_info(self):
        return conf.lib.cmark_node_get_fence_info(self._node)

    @fence_info.setter
    def fence_info(self, value):
        bytes_, _ = bytes_and_length(value)
        if not conf.lib.cmark_node_set_fence_info(self._node, bytes_):
            raise LibcmarkError("Invalid fence info %s\n" % str(value))

class HtmlBlock(Literal):
    _node_type = NodeType.HTML_BLOCK

    def __init__(self, literal=''):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True
        self.literal = literal


class CustomBlock(Node):
    _node_type = NodeType.CUSTOM_BLOCK

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True


class Paragraph(Node):
    _node_type = NodeType.PARAGRAPH

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True

class Heading(Node):
    _node_type = NodeType.HEADING

    def __init__(self, level=1):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self.level = level
        self._owned = True

    @property
    def level(self):
        return int(conf.lib.cmark_node_get_heading_level(self._node))

    @level.setter
    def level(self, value):
        res = conf.lib.cmark_node_set_heading_level(self._node, value)
        if (res == 0):
            raise LibcmarkError("Invalid heading level %s" % str(value))

class ThematicBreak(Node):
    _node_type = NodeType.THEMATIC_BREAK

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True


class Reference(Node):
    _node_type = NodeType.REFERENCE

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True

class Text(Literal):
    _node_type = NodeType.TEXT

    def __init__(self, literal=''):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True
        self.literal = literal


class SoftBreak(Node):
    _node_type = NodeType.SOFTBREAK

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True


class LineBreak(Node):
    _node_type = NodeType.LINEBREAK

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True


class Code(Literal):
    _node_type = NodeType.CODE

    def __init__(self, literal=''):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True
        self.literal = literal


class HtmlInline(Literal):
    _node_type = NodeType.HTML_INLINE

    def __init__(self, literal=''):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True
        self.literal = literal


class CustomInline(Node):
    _node_type = NodeType.CUSTOM_INLINE

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True

class Emph(Node):
    _node_type = NodeType.EMPH

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True

class Strong(Node):
    _node_type = NodeType.STRONG

    def __init__(self):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True


class Link(Node):
    _node_type = NodeType.LINK

    def __init__(self, url='', title=''):
        self._node = conf.lib.cmark_node_new(self.__class__._node_type.value)
        self._owned = True
        self.url = url
        self.title = title

    @property
    def url(self):
        return conf.lib.cmark_node_get_url(self._node)

    @url.setter
    def url(self, value):
        bytes_, _ = bytes_and_length(value)
        if not conf.lib.cmark_node_set_url(self._node, bytes_):
            raise LibcmarkError("Invalid url %s\n" % str(value))

    @property
    def title(self):
        return conf.lib.cmark_node_get_title(self._node)

    @title.setter
    def title(self, value):
        bytes_, _ = bytes_and_length(value)
        if not conf.lib.cmark_node_set_title(self._node, bytes_):
            raise LibcmarkError("Invalid title %s\n" % str(value))

class Image(Link):
    _node_type = NodeType.IMAGE

class ExtentType(BaseEnumeration):
    _kinds = []
    _name_map = None

ExtentType.NONE = ExtentType(0)
ExtentType.OPENER = ExtentType(1)
ExtentType.CLOSER = ExtentType(2)
ExtentType.BLANK = ExtentType(3)
ExtentType.CONTENT = ExtentType(4)
ExtentType.PUNCTUATION = ExtentType(5)
ExtentType.LINK_DESTINATION = ExtentType(6)
ExtentType.LINK_TITLE = ExtentType(7)
ExtentType.LINK_LABEL = ExtentType(8)
ExtentType.REFERENCE_DESTINATION = ExtentType(9)
ExtentType.REFERENCE_LABEL = ExtentType(10)
ExtentType.REFERENCE_TITLE = ExtentType(11)

class Extent(object):
    @staticmethod
    def from_result(res, fn=None, args=None):
       ret = Extent()
       ret._extent = res
       return ret

    @property
    def start(self):
        return conf.lib.cmark_source_extent_get_start(self._extent)

    @property
    def stop(self):
        return conf.lib.cmark_source_extent_get_stop(self._extent)

    @property
    def type(self):
        return conf.lib.cmark_source_extent_get_type(self._extent)

    @property
    def node(self):
        return conf.lib.cmark_source_extent_get_node(self._extent)

class SourceMap(object):
    @staticmethod
    def from_result(res, fn, args):
        ret = SourceMap()
        ret._root = res
        return ret

    def __iter__(self):
        cur = self._root
        while (cur):
            yield Extent.from_result(cur)
            cur = conf.lib.cmark_source_extent_get_next(cur)

def markdown_to_html(text, options=Parser.OPT_DEFAULT):
    bytes_, length = bytes_and_length(text)
    return conf.lib.cmark_markdown_to_html(bytes_, length, options)

def parse_document(text, options=Parser.OPT_DEFAULT):
    bytes_, length = bytes_and_length(text)
    return conf.lib.cmark_parse_document(bytes_, length, options)

functionList = [
        ("cmark_default_mem_free",
         [c_void_p]),
        ("cmark_markdown_to_html",
         [c_char_p, c_long, c_int],
         owned_char_p,
         unicode_from_owned_char_p),
        ("cmark_parse_document",
         [c_char_p, c_long, c_int],
         c_object_p,
         Node.from_result),
        ("cmark_parser_new",
         [c_int],
         c_object_p),
        ("cmark_parser_free",
         [c_object_p]),
        ("cmark_parser_feed",
         [c_object_p, c_char_p, c_long]),
        ("cmark_parser_finish",
         [c_object_p],
         c_object_p,
         Node.from_result),
        ("cmark_parser_get_first_source_extent",
         [c_object_p],
         c_object_p,
         SourceMap.from_result),
        ("cmark_source_extent_get_next",
         [c_object_p],
         c_object_p),
        ("cmark_source_extent_get_start",
         [c_object_p],
         c_ulonglong),
        ("cmark_source_extent_get_stop",
         [c_object_p],
         c_ulonglong),
        ("cmark_source_extent_get_type",
         [c_object_p],
         c_int,
         ExtentType.from_id),
        ("cmark_source_extent_get_node",
         [c_object_p],
         c_object_p,
         Node.from_result),
        ("cmark_render_html",
         [c_object_p, c_int],
         owned_char_p,
         unicode_from_owned_char_p),
        ("cmark_render_xml",
         [c_object_p, c_int],
         owned_char_p,
         unicode_from_owned_char_p),
        ("cmark_render_commonmark",
         [c_object_p, c_int, c_int],
         owned_char_p,
         unicode_from_owned_char_p),
        ("cmark_render_man",
         [c_object_p, c_int, c_int],
         owned_char_p,
         unicode_from_owned_char_p),
        ("cmark_render_latex",
         [c_object_p, c_int, c_int],
         owned_char_p,
         unicode_from_owned_char_p),
        ("cmark_node_new",
         [c_int],
         c_object_p),
        ("cmark_node_free",
         [c_object_p]),
        ("cmark_node_get_type",
         [c_object_p],
         c_int,
         NodeType.from_id),
        ("cmark_node_parent",
         [c_object_p],
         c_object_p,
         Node.from_result),
        ("cmark_node_first_child",
         [c_object_p],
         c_object_p,
         Node.from_result),
        ("cmark_node_last_child",
         [c_object_p],
         c_object_p,
         Node.from_result),
        ("cmark_node_next",
         [c_object_p],
         c_object_p,
         Node.from_result),
        ("cmark_node_previous",
         [c_object_p],
         c_object_p,
         Node.from_result),
        ("cmark_node_unlink",
         [c_object_p]),
        ("cmark_node_append_child",
         [c_object_p, c_object_p],
         c_int,
         boolean_from_result),
        ("cmark_node_prepend_child",
         [c_object_p, c_object_p],
         c_int,
         boolean_from_result),
        ("cmark_node_insert_before",
         [c_object_p, c_object_p],
         c_int,
         boolean_from_result),
        ("cmark_node_insert_after",
         [c_object_p, c_object_p],
         c_int,
         boolean_from_result),
        ("cmark_consolidate_text_nodes",
         [c_object_p]),
        ("cmark_node_get_literal",
         [c_object_p],
         c_char_p,
         unicode_from_char_p),
        ("cmark_node_set_literal",
         [c_object_p, c_char_p],
         c_int,
         boolean_from_result),
        ("cmark_node_get_heading_level",
         [c_object_p],
         c_int),
        ("cmark_node_set_heading_level",
         [c_object_p, c_int],
         c_int,
         boolean_from_result),
        ("cmark_node_get_list_type",
         [c_object_p],
         c_int,
         ListType.from_id),
        ("cmark_node_set_list_type",
         [c_object_p],
         c_int,
         boolean_from_result),
        ("cmark_node_get_list_delim",
         [c_object_p],
         c_int,
         delim_from_int),
        ("cmark_node_set_list_delim",
         [c_object_p, c_int],
         c_int),
        ("cmark_node_get_list_start",
         [c_object_p],
         c_int),
        ("cmark_node_set_list_start",
         [c_object_p, c_int],
         c_int,
         boolean_from_result),
        ("cmark_node_get_list_tight",
         [c_object_p],
         c_int,
         boolean_from_result),
        ("cmark_node_set_list_tight",
         [c_object_p, c_int],
         c_int,
         boolean_from_result),
        ("cmark_node_get_fence_info",
         [c_object_p],
         c_char_p,
         unicode_from_char_p),
        ("cmark_node_set_fence_info",
         [c_object_p, c_char_p],
         c_int,
         boolean_from_result),
        ("cmark_node_get_url",
         [c_object_p],
         c_char_p,
         unicode_from_char_p),
        ("cmark_node_set_url",
         [c_object_p, c_char_p],
         c_int,
         boolean_from_result),
        ("cmark_node_get_title",
         [c_object_p],
         c_char_p,
         unicode_from_char_p),
        ("cmark_node_set_title",
         [c_object_p, c_char_p],
         c_int,
         boolean_from_result),
]

# Taken from clang.cindex
def register_function(lib, item, ignore_errors):
    # A function may not exist, if these bindings are used with an older or
    # incompatible version of libcmark.so.
    try:
        func = getattr(lib, item[0])
    except AttributeError as e:
        msg = str(e) + ". Please ensure that your python bindings are "\
                       "compatible with your libcmark version."
        if ignore_errors:
            return
        raise LibcmarkError(msg)

    if len(item) >= 2:
        func.argtypes = item[1]

    if len(item) >= 3:
        func.restype = item[2]

    if len(item) == 4:
        func.errcheck = item[3]

def register_functions(lib, ignore_errors):
    """Register function prototypes with a libccmark library instance.

    This must be called as part of library instantiation so Python knows how
    to call out to the shared library.
    """

    def register(item):
        return register_function(lib, item, ignore_errors)

    for f in functionList:
        register(f)

class Config:
    library_path = None
    library_file = None
    compatibility_check = True
    loaded = False
    lib_ = None

    @staticmethod
    def set_library_path(path):
        """Set the path in which to search for libcmark"""
        if Config.loaded:
            raise Exception("library path must be set before before using " \
                            "any other functionalities in libcmark.")

        Config.library_path = path

    @staticmethod
    def set_library_file(filename):
        """Set the exact location of libcmark"""
        if Config.loaded:
            raise Exception("library file must be set before before using " \
                            "any other functionalities in libcmark.")

        Config.library_file = filename

    @staticmethod
    def set_compatibility_check(check_status):
        """ Perform compatibility check when loading libcmark

        The python bindings are only tested and evaluated with the version of
        libcmark they are provided with. To ensure correct behavior a (limited)
        compatibility check is performed when loading the bindings. This check
        will throw an exception, as soon as it fails.

        In case these bindings are used with an older version of libcmark, parts
        that have been stable between releases may still work. Users of the
        python bindings can disable the compatibility check. This will cause
        the python bindings to load, even though they are written for a newer
        version of libcmark. Failures now arise if unsupported or incompatible
        features are accessed. The user is required to test themselves if the
        features they are using are available and compatible between different
        libcmark versions.
        """
        if Config.loaded:
            raise Exception("compatibility_check must be set before before " \
                            "using any other functionalities in libcmark.")

        Config.compatibility_check = check_status

    @property
    def lib(self):
        if self.lib_:
            return self.lib_
        lib = self.get_cmark_library()
        register_functions(lib, not Config.compatibility_check)
        Config.loaded = True
        self.lib_ = lib
        return lib

    def get_filename(self):
        if Config.library_file:
            return Config.library_file

        import platform
        name = platform.system()

        if name == 'Darwin':
            file = 'libcmark.dylib'
        elif name == 'Windows':
            file = 'cmark.dll'
        else:
            file = 'libcmark.so'

        if Config.library_path:
            file = Config.library_path + '/' + file

        return file

    def get_cmark_library(self):
        try:
            library = cdll.LoadLibrary(self.get_filename())
        except OSError as e:
            msg = str(e) + "(%s). To provide a path to libcmark use " \
                           "Config.set_library_path() or " \
                           "Config.set_library_file()." % self.get_filename()
            raise LibcmarkError(msg)

        return library

    def function_exists(self, name):
        try:
            getattr(self.lib, name)
        except AttributeError:
            return False

        return True

conf = Config()

__alla__ = [
        'Parser',
        'LibcmarkError',
        'NodeType',
        'ListType',
        'Node',
        'Document',
        'BlockQuote',
        'List',
        'Item',
        'CodeBlock',
        'HtmlBlock',
        'CustomBlock',
        'Paragraph',
        'Heading',
        'ThematicBreak',
        'Text',
        'SoftBreak',
        'LineBreak',
        'Code',
        'HtmlInline',
        'CustomInline',
        'Emph',
        'Strong',
        'Link',
        'Image',
        'ExtentType',
        'Extent',
        'SourceMap',
        'markdown_to_html',
        'parse_document',
        'Config',
        'conf'
]
