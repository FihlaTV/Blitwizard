--[[
blitwizard.debug (lua extensions)
Under the zlib license:

Copyright (c) 2014 Jonas Thiem

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required. 

2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.

]]

if type(blitwizard.debug) == "table" then
    -- debug functions are present. extend them:
    function blitwizard.debug.printGlobalTextureInfo()
        texlist = blitwizard.debug.getAllTextures()
        local texturecolwidth = 50
        local loadedcolwidth = 10

        local function fitlen(texname, length)
            if #texname <= length then
                -- already short enough
                while #texname < length do
                    texname = texname .. " "
                end
                return texname
            end
            -- skip directories from start
            local dirskipped = false
            local dirskipaddedlen = 0
            while #texname + dirskipaddedlen > length
                    and (texname:find("/", 1, true) ~= nil
                    or texname:find("\\", 1, true)) do
                local pos = math.min(texname:find("/", 1, true) or 9999,
                    texname:find("\\", 1, true) or 9999)
                if pos >= #texname then
                    break
                end
                texname = texname:sub(pos + 1, #texname)
                dirskipped = true
                dirskipaddedlen = #".../"
            end
            -- still too long? take out the middle:
            if #texname + dirskipaddedlen > length then
                local firsthalf = length/2
                local secondhalf = length/2
                local dots = "..."
                if length < 5 then
                    dots = ""
                end
                while firsthalf + #dots + secondhalf + dirskipaddedlen
                        > length do
                    firsthalf = firsthalf - 1
                    secondhalf = secondhalf - 1
                end
                texname = texname:sub(1, firsthalf - 1) .. dots ..
                    texname:sub(-secondhalf)
            end
            while #texname + dirskipaddedlen < length do
                texname = texname .. " "
            end
            if dirskipped then
                texname = ".../" .. texname
            end
            return texname
        end

        local t = "Texture"
        if texturecolwidth < 20 then
            texturecolwidth = 20
        end
        while #t < texturecolwidth do
            t = t .. " "
        end
        t = t .. " Size"
        if loadedcolwidth < #"Size" then
            loadedcolwidth = #"Size"
        end
        while #t < loadedcolwidth + 1 + texturecolwidth do
            t = t .. " "
        end
        print(t)
        t = ""
        while #t < loadedcolwidth + 1 + texturecolwidth do
            t = t .. "-"
        end
        print(t)
        for _,texture in ipairs(texlist) do
            local t = fitlen(texture, texturecolwidth) .. " "
                .. fitlen(tostring(
                    blitwizard.debug.getTextureGpuSizeInfo(
                    texture)), loadedcolwidth)
            print(t)
        end
    end
end

