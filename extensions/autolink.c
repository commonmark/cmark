#include "autolink.h"
#include <parser.h>
#include <string.h>

#if defined(_WIN32)
#define strncasecmp	_strnicmp
#endif

static int sd_autolink_issafe(const uint8_t *link, size_t link_len) {
	static const size_t valid_uris_count = 5;
	static const char *valid_uris[] = {
		"/", "http://", "https://", "ftp://", "mailto:"
	};

	size_t i;

	for (i = 0; i < valid_uris_count; ++i) {
		size_t len = strlen(valid_uris[i]);

		if (link_len > len &&
        strncasecmp((char *)link, valid_uris[i], len) == 0 &&
        cmark_isalnum(link[len]))
			return 1;
	}

	return 0;
}

static size_t
autolink_delim(uint8_t *data, size_t link_end)
{
	uint8_t cclose, copen;
	size_t i;

	for (i = 0; i < link_end; ++i)
		if (data[i] == '<') {
			link_end = i;
			break;
		}

	while (link_end > 0) {
		if (strchr("?!.,:*_", data[link_end - 1]) != NULL)
			link_end--;

		else if (data[link_end - 1] == ';') {
			size_t new_end = link_end - 2;

			while (new_end > 0 && cmark_isalpha(data[new_end]))
				new_end--;

			if (new_end < link_end - 2 && data[new_end] == '&')
				link_end = new_end;
			else
				link_end--;
		}
		else break;
	}

	while (link_end > 0) {
		cclose = data[link_end - 1];

		switch (cclose) {
		case '"':	copen = '"'; break;
		case '\'':	copen = '\''; break;
		case ')':	copen = '('; break;
		case ']':	copen = '['; break;
		case '}':	copen = '{'; break;
		default:	copen = 0;
		}

		if (copen == 0)
			break;
		
		size_t closing = 0;
		size_t opening = 0;
		size_t i = 0;

		/* Try to close the final punctuation sign in this same line;
		 * if we managed to close it outside of the URL, that means that it's
		 * not part of the URL. If it closes inside the URL, that means it
		 * is part of the URL.
		 *
		 * Examples:
		 *
		 *	foo http://www.pokemon.com/Pikachu_(Electric) bar
		 *		=> http://www.pokemon.com/Pikachu_(Electric)
		 *
		 *	foo (http://www.pokemon.com/Pikachu_(Electric)) bar
		 *		=> http://www.pokemon.com/Pikachu_(Electric)
		 *
		 *	foo http://www.pokemon.com/Pikachu_(Electric)) bar
		 *		=> http://www.pokemon.com/Pikachu_(Electric)
		 *
		 *	(foo http://www.pokemon.com/Pikachu_(Electric)) bar
		 *		=> foo http://www.pokemon.com/Pikachu_(Electric)
		 */

		while (i < link_end) {
			if (data[i] == copen)
				opening++;
			else if (data[i] == cclose)
				closing++;

			i++;
		}

		if (closing == opening)
			break;

		link_end--;
	}

	return link_end;
}

static size_t
check_domain(uint8_t *data, size_t size, int allow_short)
{
	size_t i, np = 0, uscore1 = 0, uscore2 = 0;

	for (i = 1; i < size - 1; i++) {
		if (data[i] == '_') uscore2++;
		else if (data[i] == '.') {
			uscore1 = uscore2;
			uscore2 = 0;
			np++;
		}
		else if (!cmark_isalnum(data[i]) && data[i] != '-') break;
	}

	if (uscore1 > 0 || uscore2 > 0) return 0;

	if (allow_short) {
		/* We don't need a valid domain in the strict sense (with
		 * least one dot; so just make sure it's composed of valid
		 * domain characters and return the length of the the valid
		 * sequence. */
		return i;
	} else {
		/* a valid domain needs to have at least a dot.
		 * that's as far as we get */
		return np ? i : 0;
	}
}

static cmark_node *www_match(cmark_parser *parser, cmark_node *parent, cmark_inline_parser *inline_parser) {
  cmark_chunk *chunk = cmark_inline_parser_get_chunk(inline_parser);
  size_t max_rewind = cmark_inline_parser_get_offset(inline_parser);
  uint8_t *data = chunk->data + max_rewind;
  size_t size = chunk->len - max_rewind;

  size_t link_end;

  if (max_rewind > 0 && data[-1] != '(' && data[-1] != '[' && !cmark_isspace(data[-1]))
    return 0;

  if (size < 4 || memcmp(data, "www.", strlen("www.")) != 0)
    return 0;

  link_end = check_domain(data, size, 0);

  if (link_end == 0)
    return NULL;

  while (link_end < size && !cmark_isspace(data[link_end]))
    link_end++;

  link_end = autolink_delim(data, link_end);

  if (link_end == 0)
		return NULL;

  cmark_inline_parser_set_offset(inline_parser, max_rewind + link_end);

  cmark_node *node = cmark_node_new_with_mem(CMARK_NODE_LINK, parser->mem);

  cmark_strbuf buf;
  cmark_strbuf_init(parser->mem, &buf, 10);
  cmark_strbuf_puts(&buf, "http://");
  cmark_strbuf_put(&buf, data, link_end);
  node->as.link.url = cmark_chunk_buf_detach(&buf);

  cmark_node *text = cmark_node_new_with_mem(CMARK_NODE_TEXT, parser->mem);
  text->as.literal = cmark_chunk_dup(chunk, max_rewind, link_end);
  cmark_node_append_child(node, text);

	return node;
}

static cmark_node *email_match(cmark_parser *parser, cmark_node *parent, cmark_inline_parser *inline_parser) {
  size_t link_end, rewind;
	int nb = 0, np = 0, ns = 0;

  cmark_chunk *chunk = cmark_inline_parser_get_chunk(inline_parser);
  size_t max_rewind = cmark_inline_parser_get_offset(inline_parser);
  uint8_t *data = chunk->data + max_rewind;
  size_t size = chunk->len - max_rewind;

  for (rewind = 0; rewind < max_rewind; ++rewind) {
		uint8_t c = data[-rewind - 1];

		if (cmark_isalnum(c))
			continue;

		if (strchr(".+-_", c) != NULL)
			continue;

		if (c == '/')
			ns++;

		break;
	}

	if (rewind == 0 || ns > 0)
		return 0;

	for (link_end = 0; link_end < size; ++link_end) {
		uint8_t c = data[link_end];

		if (cmark_isalnum(c))
			continue;

		if (c == '@')
			nb++;
		else if (c == '.' && link_end < size - 1)
			np++;
		else if (c != '-' && c != '_')
			break;
	}

	if (link_end < 2 || nb != 1 || np == 0 ||
      (!cmark_isalpha(data[link_end - 1]) && data[link_end - 1] != '.'))
		return 0;

	link_end = autolink_delim(data, link_end);

  if (link_end == 0)
		return NULL;

  cmark_inline_parser_set_offset(inline_parser, max_rewind + link_end);
  cmark_node_unput(parent, rewind);

  cmark_node *node = cmark_node_new_with_mem(CMARK_NODE_LINK, parser->mem);

  cmark_strbuf buf;
  cmark_strbuf_init(parser->mem, &buf, 10);
  cmark_strbuf_puts(&buf, "mailto:");
  cmark_strbuf_put(&buf, data - rewind, link_end + rewind);
  node->as.link.url = cmark_chunk_buf_detach(&buf);

  cmark_node *text = cmark_node_new_with_mem(CMARK_NODE_TEXT, parser->mem);
  text->as.literal = cmark_chunk_dup(chunk, max_rewind - rewind, link_end + rewind);
  cmark_node_append_child(node, text);

	return node;
}

static cmark_node *url_match(cmark_parser *parser, cmark_node *parent, cmark_inline_parser *inline_parser) {
  size_t link_end, rewind = 0, domain_len;

  cmark_chunk *chunk = cmark_inline_parser_get_chunk(inline_parser);
  size_t max_rewind = cmark_inline_parser_get_offset(inline_parser);
  uint8_t *data = chunk->data + max_rewind;
  size_t size = chunk->len - max_rewind;

  if (size < 4 || data[1] != '/' || data[2] != '/')
		return 0;

	while (rewind < max_rewind && cmark_isalpha(data[-rewind - 1]))
		rewind++;

	if (!sd_autolink_issafe(data - rewind, size + rewind))
		return 0;

	link_end = strlen("://");

	domain_len = check_domain(data + link_end, size - link_end, 1);

	if (domain_len == 0)
		return 0;

	link_end += domain_len;
	while (link_end < size && !cmark_isspace(data[link_end]))
		link_end++;

	link_end = autolink_delim(data, link_end);

	if (link_end == 0)
		return NULL;

  cmark_inline_parser_set_offset(inline_parser, max_rewind + link_end);
  cmark_node_unput(parent, rewind);

  cmark_node *node = cmark_node_new_with_mem(CMARK_NODE_LINK, parser->mem);

  cmark_chunk url = cmark_chunk_dup(chunk, max_rewind - rewind, link_end + rewind);
  node->as.link.url = url;

  cmark_node *text = cmark_node_new_with_mem(CMARK_NODE_TEXT, parser->mem);
  text->as.literal = url;
  cmark_node_append_child(node, text);

	return node;
}

static cmark_node *match(cmark_syntax_extension *ext, cmark_parser *parser, cmark_node *parent, unsigned char c, cmark_inline_parser *inline_parser) {
  if (cmark_inline_parser_in_bracket(inline_parser, false) ||
      cmark_inline_parser_in_bracket(inline_parser, true))
    return NULL;

  if (c == ':')
    return url_match(parser, parent, inline_parser);

  if (c == '@')
    return email_match(parser, parent, inline_parser);

  if (c == 'w')
    return www_match(parser, parent, inline_parser);

  return NULL;

  // note that we could end up re-consuming something already a
  // part of an inline, because we don't track when the last
  // inline was finished in inlines.c.
}

cmark_syntax_extension *create_autolink_extension(void) {
  cmark_syntax_extension *ext = cmark_syntax_extension_new("autolink");
  cmark_llist *special_chars = NULL;

  cmark_syntax_extension_set_match_inline_func(ext, match);

  special_chars = cmark_llist_append(special_chars, (void *) ':');
  special_chars = cmark_llist_append(special_chars, (void *) '@');
  special_chars = cmark_llist_append(special_chars, (void *) 'w');
  cmark_syntax_extension_set_special_inline_chars(ext, special_chars);

  return ext;
}
