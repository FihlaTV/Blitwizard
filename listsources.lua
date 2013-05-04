#!/usr/bin/lua
require("lfs")

-- get the contents of the template
local f = io.open("config.ld.template", "rb")
contents = f:read("*all")
io.close(f)

-- get a list of all C files in src/ (not recursively)
iter, dir_obj = lfs.dir("src/")
dirstr = ""
filestr = iter(dir_obj)
while filestr ~= nil do
    if string.sub(filestr, -#".c") == ".c" then
        if dirstr ~= "" then
            dirstr = dirstr .. ", "
        end
        dirstr = dirstr .. "\"src/" .. filestr .. "\""
    end
    filestr = iter(dir_obj)
end

-- get a list of all Lua files in templates/ (recursively):
do
    local handle = io.popen("find templates/ -name \"*.lua\"")
    local result = handle:read("*a")
    handle:close()

    -- turn line breaks to space chars:
    result = result:gsub("\n", " ")
    result = result:gsub("\r", " ")

    -- remove leading space chars:
    while result:sub(1, 1) == " " do
        result = result:sub(2)
    end

    -- remove trailing space chars:
    while result:sub(#result, #result) == " " do
        result = result:sub(1, #result - 1)
    end

    -- turn space chars into commas:
    result = result:gsub(" ", ",")
    -- remove double commas:
    result = result:gsub(",,", ",")

    -- add quotations in a nice way:
    result = "\"" .. result:gsub(",", "\", \"") .. "\""

    -- add this to the file list:
    dirstr = dirstr .. ", " .. result
end

-- replace template items:
local f = io.open("config.ld", "wb")
contents = string.gsub(contents, "INSERT_FILES_HERE", dirstr)
f:write(contents)
io.close(f)
print("Contents: " .. contents)

