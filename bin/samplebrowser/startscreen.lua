
--[[

blitwizard engine - source code file

  Copyright (C) 2013 Jonas Thiem

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

-- This is the new blitwizard startup screen.
--
-- It procudes an endless rain of blue orbs!
-- (if the image orb.png is present)
--
-- It also displays a top bar (topbar.png)
-- and an introduction text.
--
-- Can be used as a stress test. :-)
-- (increase amount of orbs)

function blitwizard.onInit()
    -- this will open up a graphics window:
    blitwizard.graphics.setMode(800 * 1, 600 * 1, "blitwizard", false)
    blitwizard.graphics.getCameras()[1]:set2dZoomFactor(2) -- zoom into things

    -- Introduction text:
    local text = blitwizard.font.text:new("Welcome to " .. _VERSION .. "!" ..
    "\n\n  Press F9 for developer console." ..
    "\n\n  Visit http://www.blitwizard.de/doc-files/api-stable " ..
    "for documentation." ..
    "\n\n  Need help? Check out the forums: http://www.blitwizard.de/forum/"
    , "default", 1)
    text:setPosition(1.5, 3.5) -- move a bit away from top/left corner
    text:setZIndex(2)

    -- Create top bar:
    local bar = blitwizard.object:new(
        blitwizard.object.o2d, "topbar.png")
    bar:setRotationAngle(0)

    -- Pin top bar to camera:
    -- (it stays at the same screen position regardless of
    -- zoom and camera movement - useful for interface
    -- graphics like health bars or similar things)
    local cameras = blitwizard.graphics.getCameras()
    bar:pinToCamera(cameras[1])
    bar:setZIndex(1)

    -- scale top bar to the width of the visible area
    -- (camera:getDimensions(false) returns
    -- the visible area for pinned objects, so without
    -- zoom as for normal objects):
    function bar:onGeometryLoaded()
        local w,h = cameras[1]:getDimensions()
        bar:scaleToDimensions(w, nil)
    end

    -- button for showing sample browser selection
    local button = blitwizard.object:new(
        blitwizard.object.o2d, "browse.png")
    button:pinToCamera()

    -- move to a nice position:
    function button:onGeometryLoaded()
        local w,h = cameras[1]:getDimensions()
        self:setScale(0.7, 0.7)
        local tx,th = self:getDimensions()
        self:setPosition(w/2 - tx/2, h/2 + h/6)
    end

    -- when clicking, open sample browser:
    function button:onMouseClick()
        -- destroy all orbs:
        noOrbs = true

        -- destroy bar, button and text:
        bar:destroy()
        button:destroy()
        text:destroy()

        -- launch sample browser list:
        browser.launchSelection()
    end

    -- weaken gravity for nicely floating orbs:
    blitwizard.physics.set2dGravity(0, 0.2)

    -- spawn a few orbs (see spawnOrb code below):
    local i = 0
    while i < 10 do
        spawnOrb()
        i = i + 1
    end
end

-- blue orb spawn function:
function spawnOrb()
    -- when orbs are disabled, don't spawn any new ones:
    if noOrbs then
        return
    end

    -- spawn blue orb at random position:
    local obj = blitwizard.object:new(
        blitwizard.object.o2d, "orb.png")

    -- random position, scale and alpha:
    obj:setPosition(math.random() * 6 - 3, math.random() * 6 - 3)
    local v = 0.2 + 6*math.random()
    obj:setScale(v, v)
    obj:setTransparency(math.random() * 0.5 + 0.5)
    obj:setZIndex(-9999)

    -- initialise physics:
    function obj:onGeometryLoaded()
        -- some information:
        local w,h = self:getDimensions()

        -- enable collision, apply impulse and set mass:
        self:enableMovableCollision({type="circle", diameter=w})
        self:setMass(50)
        self:impulse(math.random() * 100 - 50, -math.random() * 1)
    end
    function obj:onCollision(obj)
        -- ignore all collisions with other orbs:
        return false
    end

    function obj:doOften()
        -- if too far from visible area, delete and spawn new:
        local x, y = self:getPosition()
        if y > 4 or x > 5 or x < -5 then
            self:destroy()
            spawnOrb()
        end
    end
end



