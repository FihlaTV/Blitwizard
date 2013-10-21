
--[[
   This is a simple example that plays a short music track.

   You can copy, alter or reuse this file any way you like.
   It is available without restrictions (public domain).
]]

print("Sound example in blitwizard")

function blitwizard.onInit()
    -- Open a window
    function openwindow()
        blitwizard.graphics.setMode(640, 480, "Sound", false)
    end
    if pcall(openwindow) == false then
        -- Opening a window failed.
        -- Open fullscreen at any resolution (for Android)
        resolution = blitwizard.graphics.getDisplayModes()[1]
        blitwizard.graphics.setMode(resolution[1], resolution[2], "Sound", true)
    end    

    -- Load image
    local background = blitwizard.object:new(
       blitwizard.object.o2d, "background.png")
    background:pinToCamera()

    -- Play song
    local sound = blitwizard.audio.simpleSound:new(
        "blitwizardtragicallyepic.ogg")

    -- This will play sound "blitwizardtragicallyepic.ogg" with
    -- full volume (1.0), repeated forever (repeat parameter true)
    -- and faded in over 0.1 seconds.
    sound:play(1.0, true, 0.1)
end

function blitwizard.onKeyDown(key)
    -- When escape is pressed, we want to quit
    if key == "escape" then
        os.exit(0)
    end
end

function blitwizard.onClose()
    -- This function gets called whenever the user clicks
    -- the close button in the window title bar.
    
    -- The user has attempted to close the window,
    -- so we want to respect his wishes and quit :-)
    os.exit(0)
end


