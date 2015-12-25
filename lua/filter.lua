-- This is a sample filter using the cmark API in lua.
-- It adds a parenthetical message after each link,
-- numbering the link.
-- It also adds a paragraph to the end of the document that
-- states how many links the document contains.

-- A filter is a lua program that returns a function
-- whose arguments are a cmark node and a string giving
-- the target format.
return function(doc, format)
   local cur, node_type, entering
   local links = 0

   -- cmark-lua has a built-in iterator to walk over
   -- all the node of the document.
   for cur, entering, node_type in walk(doc) do
      -- Increment links if we're entering a link node:
      if node_type == NODE_LINK and not entering then
          links = links + 1
          -- insert " (link #n)" after the link:
          local t = node_new(NODE_TEXT)
          node_set_literal(t, string.format(" (link #%d)", links))
          node_insert_after(cur, t)
      end
   end

   -- Now we need to add a paragraph at the end of the
   -- document with a message about the number of links
   -- found.  We'll need to create a paragraph node,
   -- and a text node to go in it, and we'll add the
   -- text as the literal content of the text node.
   local p = node_new(NODE_PARAGRAPH)
   local t = node_new(NODE_TEXT)
   node_set_literal(t, string.format("%d links found in this %s document.", links, format))
   node_append_child(p, t)
   node_append_child(doc, p)

end

