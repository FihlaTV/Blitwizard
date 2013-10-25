
--[[
   This example attempts to demonstrate the physics.

   You can copy, alter or reuse this file any way you like.
   It is available without restrictions (public domain).
]]

print("Physics example in blitwizard")

-- >> Some global vars we want to use:
-- meter (physics unit) to pixels factor:
pixelsperunit = 30 -- meter (physics unit) to pixels factor

-- Warn if we run without templates
if blitwizard.templatesinitialised ~= true then
	error "The templates/ sub folder with the templates is apparently missing. Please copy it into the same folder as your game.lua before you start up."
end

function blitwizard.onInit()
	-- Open a window
	if string.lower(os.sysname()) == "android" then
        -- Fullscreen at screen resolution for Android
        local w,h = blitiwzard.graphics.getDesktopDisplayMode()
        blitwizard.graphics.setWindow(w, h, "Physics", true)
    else
        -- Windowed at 640x480 for desktop
        blitwizard.graphics.setMode(640, 480, "Physics", false)
    end

    -- set gravity to default in case it was changed:
    blitwizard.physics.set2dGravity(
        0, 9.81 * 0.75) -- assume 1 unit is roughly 1.33 meters

	-- Add base level with collision (bg.png)
	level = blitwizard.object:new(blitwizard.object.o2d,
        "bg.png")
    -- this is how much a game unit takes up in pixels:
    pixelsperunit = blitwizard.graphics.gameUnitToPixels()
    -- this is half the size of our image in pixels:
    local halfwidth = 320
    local halfheight = 240
    level:enableStaticCollision({
        type = "edge list",
        edges = {
            -- We want to specify nice edges which represent the geometry
            -- drawn in bg.png (approximately).
            --
            -- In an image manipulation program, we looked at bg.png and
            -- determined the pixel coordinates where edges should run through.
            --
            -- Now we specify the pixel coordinates in pairs per point,
            -- and two pairs for a line:  { {119, 0}, {119, 360} }
            -- Because this is easiest, we assume so far that 0, 0 is top left
            -- and 640, 480 is bottom right (the image has the size 640x480).
            --
            -- Then we substract half of the image size of all the coordinates
            -- to make 0,0 the center of the image (instead of top left).
            --
            -- Finally, we divide all coordinates by pixelsperunit to get game
            -- units. Now we have game coordinates of all edge positions!
            --
            -- Here is the result (5 edges):
            { {(119-halfwidth)/pixelsperunit, (0-halfheight)/pixelsperunit},
              {(119-halfwidth)/pixelsperunit, (360-halfheight)/pixelsperunit} },
            
            { {(119-halfwidth)/pixelsperunit, (360-halfheight)/pixelsperunit},
              {(397-halfwidth)/pixelsperunit, (234-halfheight)/pixelsperunit} },

            { {(397-halfwidth)/pixelsperunit, (234-halfheight)/pixelsperunit},
              {(545-halfwidth)/pixelsperunit, (371-halfheight)/pixelsperunit} },

            { {(545-halfwidth)/pixelsperunit, (371-halfheight)/pixelsperunit},
              {(593-halfwidth)/pixelsperunit, (122-halfheight)/pixelsperunit} },

            { {(593-halfwidth)/pixelsperunit, (122-halfheight)/pixelsperunit},
              {(564-halfwidth)/pixelsperunit, (0-halfheight)/pixelsperunit} }
	    }
    })
    level:setFriction(0.3)

	-- More basic level collision shape:
	-- (that black obtruding rectangle part in bg.png on the floor/center)
	levelcollision2 = blitwizard.object:new(
        blitwizard.object.o2d, nil)
    levelcollision2:enableStaticCollision({
        type="rectangle",
        width=((382 - 222)/pixelsperunit),
        height=((314 - 242)/pixelsperunit)
    })
    levelcollision2:setPosition(((222 + 382)/2 - halfwidth)/pixelsperunit,
        ((242 + 314)/2 - halfheight)/pixelsperunit)
	levelcollision2:setFriction(0.3)

    -- on top of everything, we're putting a neat shadow image
    -- which makes everything look a bit cooler:
    local shadows = blitwizard.object:new(
        blitwizard.object.o2d, "shadows.png")
    shadows:setZIndex(5)
end

function bgimagepos()
	local w,h = blitwiz.graphics.getImageSize("bg.png")
    local mw,mh = blitwiz.graphics.getWindowSize()
	return mw/2 - w/2, mh/2 - h/2
end

function limitspawnposition(size, x, y)
    -- This simply limits the object's spawn position to a
    -- more reasonable area.
    -- You could leave this away if you want, it is somewhat
    -- optional.
    local w,h = level:getDimensions()
	if x - size/2 < (125/pixelsperunit)-(w/2) then
		x = (125/pixelsperunit)-(w/2) + size/2
	end
	if x + size/2 > (555/pixelsperunit)-(w/2) then
		x = (555/pixelsperunit)-(w/2) - size/2
	end
	if y + size/2 > (230/pixelsperunit)-(h/2) then
		-- If we are too low, avoid getting it stuck in the floor
		y = (230/pixelsperunit)-(h/2) - size/2
	end
	return x,y
end

function blitwizard.onMouseDown(x, y)
	-- See where we can add the object
	local objectposx,objectposy =
        blitwizard.graphics.getCameras()[1]:screenPosTo2dWorldPos(x, y)

	if math.random() > 0.5 then
        -- the crate has a side length of 64 pixels.
        -- limit its position so it spawns inside the level:
		objectposx,objectposy = limitspawnposition(
		    64/pixelsperunit, objectposx, objectposy)

		-- Add a crate
		local crate = blitwizard.object:new(
            blitwizard.object.o2d, "crate.png")
        crate:setZIndex(1)
        function crate:onLoaded()
            local w,h = self:getDimensions()
            self:enableMovableCollision({
                type="rectangle",
                width=w,
                height=h,
            })
            self:setFriction(0.4)
            self:setMass(30)
            self:setAngularDamping(0.5)
            self:setLinearDamping(0.3)
        end
        crate:setPosition(objectposx, objectposy)
	else
        -- the ball has a diameter of 24 pixels,
        -- use our neat placement limiter :
        objectposx,objectposy = limitspawnposition(
            24/pixelsperunit, objectposx, objectposy)

		-- Add a ball
        local ball = blitwizard.object:new(
            blitwizard.object.o2d, "ball.png")
        ball:setZIndex(1)
        function ball:onLoaded()
            local w,h = self:getDimensions()
            self:enableMovableCollision({
                type="circle",
                diameter=w,
            })
            self:setFriction(0.1)
            self:setMass(10)
            self:setAngularDamping(0.3)
            self:setLinearDamping(0.9)
            self:setRestitution(0.3)
        end
        ball:setPosition(objectposx, objectposy)
	end
end

function blitwizard.onClose()
	os.exit(0)
end


