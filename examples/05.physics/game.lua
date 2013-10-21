
--[[
   This example attempts to demonstrate the physics.

   You can copy, alter or reuse this file any way you like.
   It is available without restrictions (public domain).
]]

print("Physics example in blitwizard")

-- >> Some global vars we want to use:
-- list of all crate physics objects:
crates = {}
-- list of health numbers (0..1) for each crate object:
crateshealth = {}
-- list of crate splint particles. each particle is a sub list containing:
-- { x pos, y pos , initial rotation, rotation speed, anim progress (0..1) }
cratesplints = {}
-- list of all ball physics objects:
balls = {}
-- list of smoke particles. each particle is a sub list containing:
-- { x pos, y pos, current rotation angle, current smoke alpha }
smokeobjs = {}
-- meter (physics unit) to pixels factor:
pixelsperunit = 30 -- meter (physics unit) to pixels factor
-- size of a crate (length of each side):
cratesize = 64/pixelsperunit
-- size of a ball (diameter):
ballsize = 24/pixelsperunit

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

	-- Load image
	--[[blitwiz.graphics.loadImage("bg.png")
	blitwiz.graphics.loadImage("crate.png")
	blitwiz.graphics.loadImage("shadows.png")
	blitwiz.graphics.loadImage("ball.png")
    blitwiz.graphics.loadImage("smoke.png")
    blitwiz.graphics.loadImage("cratesplint.png")]]

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

	-- More basic level collision shape:
	-- (that black obtruding rectangle part in bg.png on the floor/center)
	levelcollision2 = blitwizard.object:new(
        blitwizard.object.o2d, "crate.png")
    levelcollision2:enableStaticCollision({
        type="rectangle",
        width=((382 - 222)/pixelsperunit)*0.5,
        height=((314 - 242)/pixelsperunit)
    })
    levelcollision2:setPosition(((222 + 382)/2 - halfwidth)/pixelsperunit, ((224 + 314)/2 - halfheight)/pixelsperunit)
	levelcollision2:setFriction(0.3)
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

        --[[-- Set a collision callback for the smoke effect
        blitwiz.physics2d.setCollisionCallback(crate, function(otherobj, x, y, nx, ny, force)
            if force > 4 then
                smokeobjs[#smokeobjs+1] = { x * pixelsperunit, y * pixelsperunit, math.random()*360, math.min(1, (force-4)/20) }
            end
            if force > 1 then
                -- substract health from the crate:
                local i = 1
                while i <= #crates do
                    if crates[i] == crate then
                        local h = crateshealth[i]
                        h = h - force/500
                        crateshealth[i] = h
                        if h <= 0 then
                            -- health is zero -> destroy crate
                            local j = 1
                            while j <= 2 + math.random() * 8 do
                                cratesplints[#cratesplints+1] = { x * pixelsperunit + 30 - 60 * math.random(), y * pixelsperunit + 30 - 60 * math.random(), math.random() * 360, math.random()*1, 0 }
                                j = j + 1 
                            end
                            table.remove(crates, i)
                            blitwiz.physics2d.destroyObject(crate)
                            table.remove(crateshealth, i)
                        end
                        break
                    end
                    i = i +1
                end
            end
            return true
        end)]]--
	else
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
            self:setMass(0.4)
            self:setAngularDamping(0.3)
            self:setLinearDamping(0.3)
            self:setRestitution(0.3)
        end
        ball:setPosition(objectposx, objectposy)
	end
end

function blitwizard.onClose()
	os.exit(0)
end

--[[function blitwiz.on_step()
    -- animate smoke:
    local i = 1
    while i <= #smokeobjs do
        -- adjust alpha:
        smokeobjs[i][4] = math.max(0, smokeobjs[i][4] - 0.004)

        -- adjust rotation angle:
        smokeobjs[i][3] = smokeobjs[i][3] + 1

        -- remove from list when invisible:
        if smokeobjs[i][4] <= 0.05 then
            table.remove(smokeobjs, i)
        else
            i = i + 1
        end
    end
    -- animate splints:
    local i = 1
    while i <= #cratesplints do
        -- do animation:
        cratesplints[i][5] = cratesplints[i][5] + 0.005
        if cratesplints[i][5] > 1 then
            -- remove splint when animation complete:
            table.remove(cratesplints, i)
        end
        i = i + 1
    end
end]]

