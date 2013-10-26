
--[[
   This example attempts to demonstrate a movable character
   based on the physics.

   You can copy, alter or reuse this file any way you like.
   It is available without restrictions (public domain).
]]

print("Moving character example in blitwizard")
leftright = 0 -- keyboard left/right input
jump = false -- keyboard jump input

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
    level:setZIndex(0) -- put nicely in the background
    local pixelsperunit = blitwizard.graphics.gameUnitToPixels()
    local halfwidth = 320  -- half the width of bg.png
    local halfheight = 240  -- half the height of bg.png
    -- set up the level's collision shape:
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
	level:setFriction(0.5)

	-- Add animated character.
	-- The animation code we use here is just a suggestion,
	-- you could achieve the same result differently.
    char = blitwizard.object:new(
        blitwizard.object.o2d, "char1.png")
    function char:onGeometryLoaded()
        -- character's texture dimensions were loaded!
        -- set up the character:

        -- set ourselves invisible:
        self:setVisible(false)
        assert(self:getVisible() == false)

        -- load all animation frames as sub objects
        -- and then show those instead:
        self.animationFrames = {}
        local parent = self
        local i = 1
        while i <= 3 do
            -- create new object as visible animation frame:
            local frame = blitwizard.object:new(
                blitwizard.object.o2d, "char" .. i .. ".png")
            function frame:doAlways()
                -- move sub objects to follow parent (us)
                local x,y = parent:getPosition()
                frame:setPosition(x, y)
                frame:setZIndex(parent:getZIndex())
            end
            -- store frame in a list:
            self.animationFrames[i] = frame
            i = i + 1
        end

        -- this function will allow to specify the frame to show:
        function self:setFrame(frame)
            self.animationFrames[frame]:setVisible(true)
            -- hide all other frames:
            local i = 1
            while i <= #self.animationFrames do
                if i ~= frame then
                    self.animationFrames[i]:setVisible(false)
                end
                i = i + 1
            end
        end

        -- start with showing the first frame
        self:setFrame(1)

        -- put ourselves on top of the background:
        self:setZIndex(2)

        -- enable collision and make this movable:
        local w,h = self:getDimensions()
        self:enableMovableCollision({
            type = "oval",
            width = w * 0.75,
            height = h
        })
        self:setMass(60)
        self:setFriction(0.3)
        self:setLinearDamping(7)
        self:restrictRotation(true)

        -- move left and right:
        function self:doAlways()
            -- animation frame we want to use:
            local frame = -1

            -- Various variables:
            local onthefloor = false
            local charx, chary = self:getPosition(char)

            -- Cast a ray to check for the floor
            local obj, posx, posy, normalx, normaly =
            blitwizard.physics.ray2d(charx, chary, charx,
            chary+350/pixelsperunit)

            -- Check if we reach the floor:
            local charsizex, charsizey = self:getDimensions()
            charsizex = charsizex / pixelsperunit
            charsizey = charsizey / pixelsperunit
            if obj ~= nil and posy < chary + charsizey/2 + 1/pixelsperunit then
                onthefloor = true
            end

            local walkanim = false
            local flipped = false
            -- Enable walking if on the floor
            if onthefloor == true or true then
                -- walk
                local upforce = -0.6
                if onthefloor then upforce = -3 end
                if leftright < 0 then
                    flipped = true
                    walkanim = true
                    self:impulse(-6, upforce, charx + 5, chary - 3)
                end
                if leftright > 0 then
                    flipped = false
                    walkanim = true
                    self:impulse(6, upforce, charx - 5, chary - 3)
                end
                -- jump
                if jump == true and
                (self.lastjump or 0) + 500 < blitwizard.time.getTime() then
                    self.lastjump = blitwizard.time.getTime()
                    self:impulse(0, -200, charx, chary - 1)
                end
            end

            -- Check out how to animate
            if walkanim == true then
                -- We walk, animate:
                frame = 2

                -- our animation state keeps track of switching
                -- the frames to visualize walking:
                self.animationstate = (self.animationstate or 0) + 1
                if self.animationstate >= 13 then
                    -- half of the animation time is reached,
                    -- switch to second walk frame
                    frame = 3
                end
                if self.animationstate >= 26 then
                    -- wrap over at the end of the animation:
                    self.animationstate = 0
                    frame = 2
                end
            else
                frame = 1
            end

            -- set animation frame if changed:
            if frame > 0 then
                self:setFrame(frame)
            end
        end
    end
end

function blitwizard.onKeyDown(key)
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

function blitwizard.onKeyUp(key)
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

function blitwizard.onClose()
	os.exit(0)
end

