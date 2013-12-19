
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
    -- window here with blitwiz.graphics.setWindow().
    
    -- Open a window
    blitwizard.graphics.setMode(640, 480, "Simple Car")
    
    -- Think of a car position at the very left of the screen:
    carx = (blitwizard.graphics.getCameras()[1]:getDimensions()) / 2 - 0.2
    -- We will use this later in the step function and
    -- increase it steadily for a moving car.

    -- create background object:
    local bg = blitwizard.object:new(
    blitwizard.object.o2d, "background.png")
    bg:setZIndex(1)

    -- create car object:    
    local car = blitwizard.object:new(
    blitwizard.object.o2d, "car.png")
    car:setZIndex(2)
    function car:onGeometryLoaded()
        -- now the textur size of the car is known
        -- (texture is at least partially loaded).
        -- make it slowly move along the bottom of the screen:
        function self:doAlways()
            -- get current pos and window size:
            local x,y = self:getPosition()
            local w,h = blitwizard.graphics.getCameras()[1]:getDimensions()
            -- get size of the car image (=us)
            local iw, ih = self:getDimensions()
            -- advance position and warp from right to left border:
            x = x + 0.05
            if x >= w/2 + iw/2 then
                x = -(w/2) - iw/2
            end
            -- set final position to car:
            self:setPosition(x, h/2-ih/2)
        end
    end    

    -- create in-front night mask for nice vignette:
    local mask = blitwizard.object:new(
    blitwizard.object.o2d, "nightmask.png")
    mask:setZIndex(3)
end


function blitwizard.onClose()
    -- The user has attempted to close the window,
    -- so we want to respect his wishes and quit :-)
    os.exit(0)
end



