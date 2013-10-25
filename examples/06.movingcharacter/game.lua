
--[[
   This example attempts to demonstrate a movable character
   based on the physics.

   You can copy, alter or reuse this file any way you like.
   It is available without restrictions (public domain).
]]

print("Moving character example in blitwizard")
pixelsperunit = 30
cratesize = 64/pixelsperunit
frame = 1 -- animation frame to be displayed (can be 1, 2, 3)
leftright = 0 -- keyboard left/right input
animationstate = 0 -- used for timing the animation
flipped = false -- whether to draw the character flipped (=facing left)
jump = false -- keyboard jump input
lastjump = 0 -- remember our last jump to enforce a short no-jump time after each jump

function blitwizard.onInit()
	-- Open a window
	blitwizard.graphics.setMode(640, 480, "Moving character", false)

	-- Load image
	--[[blitwiz.graphics.loadImage("bg.png")
	blitwiz.graphics.loadImage("char1.png")
	blitwiz.graphics.loadImage("char2.png")
	blitwiz.graphics.loadImage("char3.png")]]

	-- Add base level collision
	level = blitwizard.object:new(blitwizard.object.o2d,
        "bg.png")
    local pixelsperunit = blitwizard.graphics.gameUnitToPixels()
    local halfwidth = 320  -- half the width of bg.png
    local halfheight = 240  -- half the height of bg.png
    level:enableStaticCollision({
        type = "edge list",
        edges = {
	        {
		        {(0-halfwidth)/pixelsperunit, (245-halfheight)/pixelsperunit},
                {(0-halfwidth)/pixelsperunit, (0-halfheight)/pixelsperunit},
            },
            {
		        {(0-halfwidth)/pixelsperunit, (245-halfheight)/pixelsperunit},
		        {(93-halfwidth)/pixelsperunit, (297-halfheight)/pixelsperunit},
            },
            {
		        {(93-halfwidth)/pixelsperunit, (297-halfheight)/pixelsperunit},
		        {(151-halfwidth)/pixelsperunit, (293-halfheight)/pixelsperunit},
            },
            {
		        {(151-halfwidth)/pixelsperunit, (293-halfheight)/pixelsperunit},
		        {(198-halfwidth)/pixelsperunit, (306-halfheight)/pixelsperunit},
            },
            {
		        {(198-halfwidth)/pixelsperunit, (306-halfheight)/pixelsperunit},
		        {(268-halfwidth)/pixelsperunit, (375-halfheight)/pixelsperunit},
	        },
            {
		        {(268-halfwidth)/pixelsperunit, (375-halfheight)/pixelsperunit},
		        {(309-halfwidth)/pixelsperunit, (350-halfheight)/pixelsperunit},
            },
            {
		        {(309-halfwidth)/pixelsperunit, (350-halfheight)/pixelsperunit},
		        {(357-halfwidth)/pixelsperunit, (355-halfheight)/pixelsperunit},
            },
            {
		        {(357-halfwidth)/pixelsperunit, (355-halfheight)/pixelsperunit},
		        {(416-halfwidth)/pixelsperunit, (427-halfheight)/pixelsperunit},
            },
            {
		        {(416-halfwidth)/pixelsperunit, (427-halfheight)/pixelsperunit},
		        {(470-halfwidth)/pixelsperunit, (431-halfheight)/pixelsperunit},
            },
            {
		        {(470-halfwidth)/pixelsperunit, (431-halfheight)/pixelsperunit},
                {(512-halfwidth)/pixelsperunit, (407-halfheight)/pixelsperunit},
            },
            {
		        {(512-halfwidth)/pixelsperunit, (407-halfheight)/pixelsperunit},
                {(558-halfwidth)/pixelsperunit, (372-halfheight)/pixelsperunit},
            },
            {
		        {(558-halfwidth)/pixelsperunit, (372-halfheight)/pixelsperunit},
                {(640-halfwidth)/pixelsperunit, (364-halfheight)/pixelsperunit},
            },
            {
		        {(640-halfwidth)/pixelsperunit, (364-halfheight)/pixelsperunit},
                {(640-halfwidth)/pixelsperunit, (0-halfheight)/pixelsperunit}
            }
        }
	})
	level:setFriction(levelcollision, 0.5)

	-- Add character
	--[[char = blitwiz.physics2d.createMovableObject()
	blitwiz.physics2d.setShapeOval(char, (50-halfwidth)/pixelsperunit, (140-halfheight)/pixelsperunit)
	blitwiz.physics2d.setMass(char, 60)
	blitwiz.physics2d.setFriction(char, 0.3)
	--blitwiz.physics2d.setLinearDamping(char, 10)
	blitwiz.physics2d.warp(char, (456-halfwidth)/pixelsperunit, (188-halfheight)/pixelsperunit)
	blitwiz.physics2d.restrictRotation(char, true)]]
end

blitwiz = {}

function bgimagepos()
	-- Return the position of the background image (normally just 0,0):
	local w,h = blitwiz.graphics.getImageSize("bg.png")
    local mw,mh = blitwiz.graphics.getWindowSize()
	return mw/2 - w/2, mh/2 - h/2
end

function blitwiz.on_keydown(key)
	-- Process keyboard walk/jump input
	if key == "a" then
		leftright = -1
	end
	if key == "d" then
		leftright = 1
	end
	if key == "w" then
		jump = true
	end
end

function blitwiz.on_keyup(key)
	if key == "a" and leftright < 0 then
		leftright = 0
	end
	if key == "d" and leftright > 0 then
		leftright = 0
	end
	if key == "w" then
		jump = false
	end
end

function blitwiz.on_draw()
	if flipped == nil then
		flipped = false
	end

	-- Draw the background image centered:
	local bgx,bgy = bgimagepos()
	blitwiz.graphics.drawImage("bg.png", {x=bgx, y=bgy})

	-- Draw the character
	local x,y = blitwiz.physics2d.getPosition(char)
	local w,h = blitwiz.graphics.getImageSize("char1.png")
	blitwiz.graphics.drawImage("char" .. frame .. ".png", {x=x*pixelsperunit - w/2 + bgx, y=y*pixelsperunit - h/2 + bgy, flipped=flipped})
end

function blitwiz.on_close()
	os.exit(0)
end

function blitwiz.on_step()
	local onthefloor = false
	local charx,chary = blitwiz.physics2d.getPosition(char)

	-- Cast a ray to check for the floor
	local obj,posx,posy,normalx,normaly = blitwiz.physics2d.ray(charx,chary,charx,chary+350/pixelsperunit)

	-- Check if we reach the floor:
	local charsizex,charsizey = blitwiz.graphics.getImageSize("char1.png")
	charsizex = charsizex / pixelsperunit
	charsizey = charsizey / pixelsperunit
	if obj ~= nil and posy < chary + charsizey/2 + 1/pixelsperunit then
		onthefloor = true
	end

	local walkanim = false
	-- Enable walking if on the floor
	if onthefloor == true then
		-- walk
		if leftright < 0 then
			flipped = true
			walkanim = true
			blitwiz.physics2d.impulse(char, charx + 5, chary - 3, -0.4, -0.6)
		end
		if leftright > 0 then
			flipped = false
			walkanim = true
        	blitwiz.physics2d.impulse(char, charx - 5, chary - 3, 0.4, -0.6)
		end
		-- jump
		if jump == true and lastjump + 500 < blitwiz.time.getTime() then
			lastjump = blitwiz.time.getTime()
			blitwiz.physics2d.impulse(char, charx, chary - 1, 0, -20)
		end
	end

	-- Check out how to animate
	if walkanim == true then
		-- We walk, animate:
		frame = 2
		animationstate = animationstate + 1
		if animationstate >= 13 then -- half of the animation time is reached, switch to second walk frame
			frame = 3
		end
		if animationstate >= 26 then -- wrap over at the end of the animation
			animationstate = 0
			frame = 2
		end
	else
		frame = 1
	end
end

