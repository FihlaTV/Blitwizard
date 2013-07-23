
--[[

blitwizard engine - source code file

  Copyright (C) 2011-2013 Jonas Thiem

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

]]

--[[

 This is the sample browser of blitwizard.
 It might be itself an interesting example to study for a more real-life
 appplication than the examples it is supposed to show, so feel free
 to read the code.

]]

examples = { "01.helloworld", "02.simplecar", "03.sound", "04.simplecar.async", "05.scalerotate", "06.physics", "07.movingcharacter" }

yoffset = 150
yspacing = 1
menufocus = 1

if browser == nil then
    browser = {}
end

dofile("startscreen.lua")

function browser.launchSelection()
    blitwizard.graphics.setMode(640, 480, "blitwizard game engine", false)
    local camera = blitwizard.graphics.getCameras()[1]
    camera:set2dZoomFactor(1)
    local white = blitwizard.object:new(blitwizard.object.o2d, "white.png")
    white:setPosition(0, 0)
    white:setScale(50, 50)
    local title = blitwizard.object:new(blitwizard.object.o2d, "title.png")
    title:setPosition(0, -2.5)

    -- create menu buttons:
    local y = -0.8
    local i = 1
    while i <= #examples do
        local button = blitwizard.object:new(blitwizard.object.o2d,
        "menu" .. i .. ".png")
        button:setPosition(-1, y)
        button:setZIndex(10)
        local exnumber = i
        function button:onMouseClick()
            -- run the given example:
            print("Launching example " .. exnumber)
        end
        y = y + 0.8
        i = i + 1
    end
end

-- Find and delete all physics objects found recursively in _G
function find_and_delete_physics_objects(t)
    local globaltable = false
    if t == nil then
        t = _G
        globaltable = true
    end
    local function processvalue(v)
        if (type(v) == "table") then
            find_and_delete_physics_objects(v)
        end
        if (type(v) == "userdata") then
            pcall(function()
                -- This should only do anything if v is a physics object:
                blitwiz.physics2d.destroyObject(v)
            end)
        end
    end
    if not globaltable then
        for _,v in ipairs(t) do
            processvalue(v)
        end
    end
    for k,v in pairs(t) do
        if not globaltable or k ~= "_G" then
            if k ~= "blitwiz" and k ~= "table" and k ~= "package" then
                processvalue(v)
            end
        end
    end
end

function browser.runExample(number)
    os.chdir("../../examples/" .. examples[menufocus] .. "/")

    -- Remember and delete our previous event functions
    browser_onClose = blitwiz.on_close
    blitwiz.on_close = nil
    browser_onMouseDown = blitwiz.on_mousedown
    blitwiz.on_mousedown = nil
    browser_onMouseMove = blitwiz.on_mousemove
    blitwiz.on_mousemove = nil
    browser_onInit = blitwiz.on_init
    blitwiz.on_init = nil

    -- Load example
    dofile("game.lua")

    -- Wrap blitwiz.graphics.loadImage to load an image only if not present:
    if browser_loadImage_wrapped ~= true then
        browser_loadImage_wrapped = true

        -- wrap loadImage:
        local f = blitwiz.graphics.loadImage
        function blitwiz.graphics.loadImage(imgname)
            -- don't do anything if already being loaded or present
            if blitwiz.graphics.isImageLoaded(imgname) ~= nil then
                return
            end

            -- otherwise, load:
            f(imgname)
        end

        -- wrap loadImageAsync:
        local f = blitwiz.graphics.loadImageAsync
        function blitwiz.graphics.loadImageAsync(imgname)
            -- don't do anything if already being loaded or present
            if blitwiz.graphics.isImageLoaded(imgname) ~= nil then
                -- fire the callback if fully loaded:
                if blitwiz.graphics.isImageLoaded(imgname) == true then
                    blitwiz.on_image(imgname, true)
                end
                return
            end

            -- otherwise, load:
            f(imgname)
        end
    end

    -- Wrap drawing to show return button:
    local f = blitwiz.on_draw
    example_start_time = blitwiz.time.getTime()
    blitwiz.on_draw = function()
        f()
        blitwiz.graphics.drawImage("return.png", {x=blitwiz.graphics.getWindowSize()-blitwiz.graphics.getImageSize("return.png"), y= -150 + math.min(150, (blitwiz.time.getTime() - example_start_time) * 0.2)})
    end

    -- Wrap on_mousedown to enable clicking the return button:
    local f = blitwiz.on_mousedown
    blitwiz.on_mousedown = function(button, x, y)
        local imgw,imgh = blitwiz.graphics.getImageSize("return.png")
        if (x > blitwiz.graphics.getWindowSize()-imgw and y < imgh) then
            -- Wipe physics objects:
            find_and_delete_physics_objects()

            -- Unload some often-conflicting images:
            blitwiz.graphics.unloadImage("bg.png")
            blitwiz.graphics.unloadImage("background.png")

            -- Restore event functions of the sample browser:
            blitwiz.on_close = browser_on_close
            blitwiz.on_mousedown = browser_on_mousedown
            blitwiz.on_mousemove = browser_on_mousemove
            blitwiz.on_init = browser_on_init
            blitwiz.on_draw = browser_on_draw
            blitwiz.on_step = browser_on_step
            return
        else
            if f ~= nil then
                f(button, x, y)
            end
        end
    end
    -- Run example
    blitwiz.on_init()
end

function blitwizard.onClose()
    os.exit(0)
end

