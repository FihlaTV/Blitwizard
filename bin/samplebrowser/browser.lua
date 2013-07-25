
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
    white = blitwizard.object:new(blitwizard.object.o2d, "white.png")
    white:setPosition(0, 0)
    white:setScale(50, 50)
    title = blitwizard.object:new(blitwizard.object.o2d, "title.png")
    title:setPosition(0, -2.5)

    -- create menu buttons:
    buttons = {}
    local y = -0.8
    local i = 1
    while i <= #examples do
        buttons[i] = blitwizard.object:new(blitwizard.object.o2d,
        "menu" .. i .. ".png")
        buttons[i]:setPosition(-1, y)
        buttons[i]:setZIndex(10)
        local exnumber = i
        local b = buttons[i]
        function b:onMouseClick()
            -- run the given example:
            print("Launching example " .. exnumber)
            browser.runExample(exnumber)
        end
        y = y + 0.8
        i = i + 1
    end
end

function browser.cleanUpAfterExample()
    -- delete all blitwizard objects:
    local o = (blitwizard.getAllObjects())()
    while o ~= nil do
        o:destroy()
        o = (blitwizard.getAllObjects())()
    end
end

function browser.destroyBrowser()
    -- delete all sample browser objects:
    white:destroy()
    white = nil
    title:destroy()
    title = nil
    for i,v in ipairs(buttons) do
        buttons[i]:destroy()
        buttons[i] = nil
    end
    buttons = nil
end

function browser.runExample(number)
    os.chdir("../../examples/" .. examples[menufocus] .. "/")

    -- create button which shall lead us back:
    local backbutton = blitwizard.object:new(blitwizard.object.o2d,
    "return.png")
    backbutton:pinToCamera()
    function backbutton:onGeometryLoaded()
        -- place button into top right corner:
        local w,h = blitwizard.graphics.getCameras()[1]:
        getVisible2dAreaDimensions()
        self:setPosition(w - self:getDimensions(), 0)
    end
    function backbutton:onMouseClick()
        -- return to sample browser:
        browser:cleanUpAfterExample() -- this also destroys us
        browser:launchSelection()
    end
    backbutton:setZIndex(9998)

    -- destroy all sample browser objects:
    browser.destroyBrowser()

    -- load example
    dofile("game.lua")

    -- run init function again:
    if type(blitwizard.onInit) == "function" then
        blitwizard.onInit()
    end
end

function blitwizard.onClose()
    os.exit(0)
end

