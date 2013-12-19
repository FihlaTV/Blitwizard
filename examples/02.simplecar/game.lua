
--[[
   This is an example that simply shows a moving car to
   demonstrate how to do a simple movement animation.

   You can copy, alter or reuse this file any way you like.
   It is available without restrictions (public domain).
]]

print("Simple car example in blitwizard")


function blitwizard.onInit()
    -- This function is called right after blitwizard has
    -- started. You would normally want to open up a
    -- window here with blitwizard.graphics.setMode().
    
    -- Open a window
    blitwizard.graphics.setMode(640, 480, "Simple Car")
    
    -- create background object:
    local bg = blitwizard.object:new(
    blitwizard.object.o2d, "background.png")
    bg:setZIndex(1)

    -- create car object:    
    local car = blitwizard.object:new(
    blitwizard.object.o2d, "car.png")
    car:setZIndex(2)
    function car:onGeometryLoaded() -- this will be called by blitwizard.
        -- now the texture size of the car is known
        -- (texture is at least partially loaded).

        -- We will now set a doAlways() function on the car,
        -- which will be called repeatedly as long as the game runs.
        -- (we don't want to do this earlier because now we know
        -- how large the car is, based on the texture size.)

        function self:doAlways()  -- this will be called repeatedly.
            -- get current position of us (= the car) and the window size:
            local carx,cary = self:getPosition()
            local windowwidth,windowheight = blitwizard.graphics.
                getCameras()[1]:getDimensions()

            -- get size of ourselves (= the car)
            local carwidth, carheight = self:getDimensions()

            -- calculate a position slightly right to our current position.
            -- If we reach the right screen border, make the position
            -- wrap to the left border again:
            carx = carx + 0.05
            if carx >= windowwidth/2 + carwidth/2 then
                -- whoops, right screen border...
                carx = -(windowwidth/2) - carwidth/2  -- warp to the left!
            end

            -- set the calculated position to us:
            self:setPosition(carx, windowheight/2-carheight/2)
        end
    end    

    -- create in-front night mask for a nice vignette effect:
    local mask = blitwizard.object:new(
    blitwizard.object.o2d, "nightmask.png")
    mask:setZIndex(3)
end


function blitwizard.onClose()
    -- The user has attempted to close the window,
    -- so we want to respect his wishes and quit :-)
    os.exit(0)
end



