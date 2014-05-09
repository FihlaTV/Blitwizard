--[[
blitwizard.tmxmap
Under the zlib license:

Copyright (c) 2012-2014 Jonas Thiem

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required. 

2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.

]]

--[[--
 The TMX map module allows you to load maps created with tiled
 (mapeditor.org).
 @author Jonas Thiem (jonasthiem@googlemail.com)
 @copyright 2014
 @license zlib
 @module blitwizard.tmxmap
]]

blitwizard.tmxmap = {}
blitwizard.tmxmap.map = {}

--[[--
  The map object represents a loaded map and its visual representation.
  @type map
  @usage
    -- Load up a map:
    mymap = blitwizard.tmxmap.map("mymap.tmx")
    -- The map center is now at 0,0 which is in the middle of the screen
    -- if you haven't moved the camera.
]]

-- helper debugging function
function _blitwizard_print_r(t, indent, visited_list)
    if indent == nil then
        indent = 0
    end
    if indent > 20 then
        return
    end
    if visited_list == nil then
        visited_list = {}
    end
    local prespace = string.rep("  ", indent)
    if type(t) == "table" then
        print(prespace .. tostring(t))
        for _, tvisited in ipairs(visited_list) do
            if tvisited == t then
                return
            end
        end
        visited_list[#visited_list + 1] = t
        print(prespace .. "{")
        for k, v in pairs(t) do
            print(prespace .. "\"" .. k .. "\" : ")
            _blitwizard_print_r(v, indent + 1, visited_list)
        end
        print(prespace .. "}")
    elseif type(t) == "string" then
        print(prespace .. "\"" .. t .. "\"")
    elseif type(t) == "number" or type(t) == "boolean" then
        print(prespace .. tostring(t))
    elseif type(t) == "function" then
        print(prespace .. "<function>")
    elseif t == nil then
        print(prespace .. "nil")
    else
        print(prespace .. "<unhandled type>")
    end
end

--[[--
  Load a map from a .tmx file.
  Once the map dimensions are known, callback_dimensions will be called (if
  you provide a function for that parameter).
  Once the map is fully loaded with all textures (this happens later),
  callback_loaded will be called.
  @function new
]]
function blitwizard.tmxmap.map:new(path, callback_dimensions, callback_loaded)
    self = {}
    setmetatable(self, { __index = blitwizard.tmxmap.map })

    -- open up the file:
    local file = io.open(path, "rb")
    if file == nil then
        error("failed to load TMX map: cannot open file for reading: " ..
            tostring(path))
    end

    -- helper function to read next XML tag:
    local readnexttag = function(f, char)
        -- Pass a file and the first character from the current position to
        -- read an XML tag from here.
        --
        -- (this first character you pass should be "<" - so you just found
        --  the beginning of a tag.)
        --
        -- Returns a table if a tag is found:
        --   { name = "name_of_tag",
        --     closing = false,  -- true if closing, false if closing, nil if
        --                       -- self-closing XML tag
        --     attributes = { ... }, -- key value table of all attributes
        --   }
        -- Returns nil if there is no more tag to read at the end of file.
        -- Returns a string if tag contents and not a tag were found.
        --
        -- Additionally, the last read char is always returned as a second
        -- return value which is the first character right after the tag from
        -- where you may want to continue processing in some way.
        --
        -- Throws an error for invalid syntax or excessively long tag.

        local b = { bytesread = 1 }
        local maxtagbytes = 512
        local maxcontentbytes = (1024 * 100)
        local function checkabort()
            if b.bytesread > maxtagbytes then
                error("excessively long tag or spacing before tag content " ..
                    "(exceeds " .. maxtagbytes .. " bytes)")
            end
        end
        local function checkcontentabort()
            if b.bytesread > maxcontentbytes then
                error("excessively long tag content (exceeds " ..
                    maxcontentbytes .. " bytes)")
            end
        end
        -- Go reading:
        if char == nil then
            -- end of file.
            return nil, char
        end
        -- Skip all stuff before the tag:
        while char == " " or char == "\t" or char == "\n" or char == "\r"
                do
            char = f:read(1)
            if char == nil then
                -- end of file
                return nil, char
            end
            b.bytesread = b.bytesread + 1
            checkabort()
        end
        -- See if this is a tag beginning:
        if char ~= "<" then
            local function trim(str)
                if #str == 0 then
                    return ""
                end
                while str:sub(#str) == " " or str:sub(#str) == "\r" or
                        str:sub(#str) == "\n" or str:sub(#str) == "\t" do
                    str = str:sub(1, #str - 1)
                end
                return str
            end
            if char == ">" then
                error("unexpected \">\", \"<\" expected")
            end
            assert(b.bytesread < 10)
            local content = ""
            -- ok this is tag contents. read them all and return:
            while char ~= "<" do
                content = content .. char
                char = f:read(1)
                if char == nil then
                    -- end of file
                    return trim(content), char
                end
                b.bytesread = b.bytesread + 1
                checkcontentabort()
            end
            return trim(content), char
        end

        -- read tag name:
        local tag_name = ""
        local tag_closing = false
        char = f:read(1)
        if char == nil then
            error("unexpected end of file in tag")
        end
        if char == "/" then
            char = f:read(1)
            if char == nil then
                error("unexpected end of file in tag")
            end
            tag_closing = true
        end
        while char ~= " " and char ~= "\t" and char ~= "\n" and char ~= "\r"
                and char ~= ">" and char ~= "/" do
            if char == "<" then
                error("unexpected \"<\" in tag name")
            end
            tag_name = tag_name .. char
            b.bytesread = b.bytesread + 1
            char = f:read(1)
            if char == nil then
                error("unexpected end of file in tag")
            end
            checkabort()
        end
        if #tag_name == 0 then
            error("invalid tag with no tag name")
        end
        local tag_attributes = {}

        local readattribute = function(f, char)
            -- Pass a file and the first character from the current position
            -- to read an XML attribute from here.
            --
            -- (the first character should be the first one of the attribute
            --  or space char stuff right before it - so you just encountered
            --  a position where you expect an attribute.)
            --
            -- Read from the given file up to the end of the next XML
            -- attribute.
            --
            -- Returns key, value, last read char in case of success.
            -- Returns nil, nil, last read char in case of no attribute found.
            --
            -- (last read char is the first one no longer considered to be a
            --  part of the attribute, which you may now want to use for
            --  further reading and processing.)
            --
            -- For invalid XML syntax, it will throw an error.
            
            while char == " " or char == "\t" or char == "\n" or char == "\r"
                    do
                char = f:read(1)
                if char == nil then
                    error("unexpected end of file in tag or attribute")
                end
                b.bytesread = b.bytesread + 1
                checkabort()
            end
            if char == ">" then
                return nil, nil, nil
            end
            -- read attribute name:
            local attribute_name = ""
            while char ~= " " and char ~= "=" and char ~= ">" and char ~= "\t"
                    and char ~= "\n" and char ~= "\r" and char ~= "/" do
                attribute_name = attribute_name .. char
                b.bytesread = b.bytesread + 1
                char = f:read(1)
                if char == nil then
                    error("unexpected end of file in tag or attribute")
                end
                checkabort()
            end

            if char == ">" or char == " " or char == "\t" or char == "\n"
                    or char == "\r" or char == "/" then
                -- atribute is complete.
                return attribute_name, "", char
            end
            assert(char == "=")
            char = f:read(1)
            if char == nil then
                error("unexpected end of file in tag or attribute")
            end

            -- read attribute value:
            local attribute_value = ""
            local escaped_with = ""
            local breakonnext = false
            if char == "'" or char == "\"" then
                escaped_with = char
            end
            while char ~= " " and (char ~= ">" or #escaped_width > 0)
                    and char ~= "\t" and
                    char ~= "\n" and char ~= "\n" and (char ~= "/" or
                    #escaped_with > 0) do
                attribute_value = attribute_value .. char
                b.bytesread = b.bytesread + 1
                char = f:read(1)
                if char == nil then
                    error("unexpected end of file in tag or attribute")
                end
                checkabort()
                if breakonnext then
                    break
                end
                if char == escaped_with then
                    escaped_with = nil
                    breakonnext = true
                end
            end

            -- strip "" or '' from value
            if #attribute_value >= 2 then
                if (attribute_value:sub(1, 1) == "\"" and attribute_value:sub(
                        #attribute_value) == "\"") or
                        (attribute_value(1, 1) == "'" and attribute_value:sub(
                        #attribute_value) == "'") then
                    attribute_value = attribute_value:sub(2,
                        #attribute_value - 1)
                end
            end
            return attribute_name, attribute_value, char
        end

        while 1 do
            if char == nil then
                error("unexpected end of file in tag")
            end
            if char == "/" then
                -- self closing tag:
                char = f:read(1)
                if tostring(char) ~= ">" then
                    if char == nil then
                        error("unexpected end of file in tag")
                    else
                        error("unexpected \"" .. char .. "\", \">\" expected")
                    end
                end
                char = f:read(1)
                return { name = tag_name, attributes = tag_attributes,
                    closing = nil }, char
            elseif char == ">" then
                char = f:read(1)
                return { name = tag_name, attributes = tag_attributes,
                    closing = tag_closing }, char
            end
            -- try read an attribute:
            local attribute_name, attribute_value
            attribute_name, attribute_value, char = readattribute(f, char)
            if attribute_name ~= nil then
                tag_attributes[attribute_name] = attribute_value
            end
        end
    end

    local function simple_xml_to_table(file, char)
        local result_dict = {}
        local current_parent = result_dict
        -- read all tags:
        char = file:read(1)
        while 1 do
            local tag
            tag, char = readnexttag(file, char)
            if tag == nil then
                print("done.")
                file:close()
                if current_parent ~= result_dict then
                    error("missing closing tag")
                end
                return result_dict
            end
            if type(tag) == "table" then
                local skip = false
                if tag.name == "?xml" or tag.name == "!DOCTYPE" then
                    skip = true
                end
                if not skip then
                    if tag.name == "attributes" or tag.name == "content" then
                        error("invalid tag name: reserved: " .. tag.name)
                    end
                    if tag.closing == nil or tag.closing == false then
                        if current_parent[tag.name] == nil then
                            current_parent[tag.name] = {}
                        end
                        table.insert(current_parent[tag.name], tag)
                        tag.content = { parent_tag = tag }
                        tag.parent = current_parent
                        if tag.closing == false then
                            current_parent = tag.content
                        end
                    else
                        local invalidclose = false
                        if current_parent.parent_tag == nil then
                            invalidclose = true
                        end
                        if invalidclose then
                            _blitwizard_print_r(result_dict)
                            error("unexpected closing tag: " .. tag.name)
                        end
                        current_parent = current_parent.parent_tag.parent
                    end
                end
            else
                if current_parent.parent_tag == nil then
                    error("unexpected text content outside of tag")
                end
                current_parent.content = tag 
            end
        end
    end

    -- load .tmx to table:
    self.tmxdata = simple_xml_to_table(file, char)
    --_blitwizard_print_r(self.tmxdata)

    -- see whether this is a valid map:
    if self.tmxdata["map"] == nil then
        error("not a valid TMX map")
    end
    if self.tmxdata["map"][1] == nil then
        error("not a valid TMX map")
    end
    if self.tmxdata["map"][1].content == nil then
        error("not a valid TMX map")
    end
    if self.tmxdata["map"][1].content["layer"] == nil then
        error("not a valid TMX map")
    end
    

    -- create graphical map:
    

    return self
end
