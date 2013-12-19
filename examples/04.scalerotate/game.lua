
--[[
   This example loads an image, then slowly shows it in
   increasing size to the user, while rotating it.

   You can copy, alter or reuse this file any way you like.
   It is available without restrictions (public domain).
]]

print("Scaling/rotation example in blitwizard")

function blitwizard.onInit()
    -- Open a window
    blitwizard.graphics.setMode(640, 480, "Scaling/Rotating", false)

    print("We are scaling using the renderer: "
        .. blitwizard.graphics.getRendererName())   

    -- Create rotating object:
    rotatingObject = blitwizard.object:new(
        blitwizard.object.o2d, "hello_world.png")

    -- Make the oject scale and rotate as soon as loaded:
    function rotatingObject:onLoaded()
        -- when this is called, the object has been loaded.
        -- set an initial scale:
        self:setScale(0.6, 0.6)

        -- set a function which slowly scales us:
        function self:doAlways()
            -- scale up:
            local sx,sy = self:getScale()
            sx = sx + 0.0005
            sy = sy + 0.0005
            self:setScale(sx, sy)
            -- rotate:
            local angle = self:getRotationAngle()
            angle = angle + 0.5
            self:setRotationAngle(angle)
        end
    end
end

function blitwizard.onClose()
    -- The user has attempted to close the window,
    -- so we want to respect his wishes and quit :-)
    os.exit(0)
end


