--[[
blitwizard.font
Under the zlib license:

Copyright (c) 2013 Jonas Thiem

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required. 

2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.

]]

--[[--
 This is an ingame developer console. Press F9 to open.
 The console allows arbitrary code execution! (That is the point of it!)

 Hence you might want to disable it in your final game with
 @{blitwizard.console.disable}.

 You can explicitely request the console to open using
 @{blitwizard.console.open}.

 @author Jonas Thiem (jonas.thiem@gmail.com)
 @copyright 2013
 @license zlib
 @module blitwizard.console
]]

do
    blitwizard.console = {}

    -- calculate a few things:
    local consoleText = ""
    local consoleTextObj = nil
    local consoleOpened = false
    local consoleYPos = 0
    local slideSpeed = 0.1
    local consoleDisabled = false
    local consoleLineHeight = 0
    local consoleHeight = 200 / blitwizard.graphics.getCameras()[1]
        :gameUnitToPixels()
    local consoleLinesShown = (function()
        local text = blitwizard.font.text:new("Hello", "default", 1)
        consoleLineHeight = text:height()
        local result = math.floor(consoleHeight/text:height())
        text:destroy()
        return result-2
    end)()
    local commandHistory = { "" }
    local inCommandHistory = 1

    -- lines storage:
    local lines = {}
    -- each line is { line = "text", text = blitwizard.font.text or nil }

    -- create background image for console:
    local consoleBg = blitwizard.object:new(false, os.templatedir() ..
    "/console/console.png")
    consoleBg:setVisible(true)
    consoleBg:pinToCamera(blitwizard.graphics.getCameras()[1])
    function consoleBg:onGeometryLoaded()
        -- calculate proper size of the dev console:
        local w,h = consoleBg:getDimensions()
        local cw,ch = blitwizard.graphics.getCameras()[1]:
        getScreenDimensions()
        cw = cw / blitwizard.graphics.getCameras()[1]:gameUnitToPixels()
        ch = ch / blitwizard.graphics.getCameras()[1]:gameUnitToPixels()
        consoleBg:setScale(cw/w, consoleHeight/h)
        consoleBg:setPosition(0, -h)
    end

    local function addConsoleLine(line)
        lines[#lines+1] = { line = line, text = nil }
        while #lines > consoleLinesShown do
            -- remove first line:
            if lines[1].text then
                lines[1].text:destroy()
            end
            local i = 1
            while i < #lines do
                lines[i] = lines[i+1]
                i = i + 1
            end
            table.remove(lines, #lines)
        end
    end

    local oldprint = print
    function print(...)
        local str = ""
        local args = table.pack(...)
        local i = 1
        while i <= args.n do
            if i > 1 then
                str = str .. "\t"
            end
            str = str .. tostring(args[i])
            i = i + 1
        end
        oldprint(str)
        addConsoleLine(str)
    end
    addConsoleLine(_VERSION .. ", developer console")

    function consoleBg:doAlways()
        if consoleOpened then
            local x,y = consoleBg:getPosition()
            if y < 0 then -- slide down
                y = y + slideSpeed
                if y > 0 then
                    y = 0
                end
            end
            consoleBg:setPosition(x, y)
        else
            local x,y = consoleBg:getPosition()
            if y > -consoleHeight then
                y = y - slideSpeed
                if y < -consoleHeight then
                    y = -consoleHeight
                end
            end
            consoleBg:setPosition(x, y)
        end

        -- place lines properly:
        local x,y = consoleBg:getPosition()
        if consoleOpened or y > -consoleHeight then
            -- Cycle through lines, load their text glyphs and move them:
            local i = 1
            while i <= #lines do
                if lines[i] ~= nil then
                    if lines[i].text == nil then
                        -- load glyphs
                        lines[i].text = blitwizard.font.text:new(
                        lines[i].line, "default", 1)
                    end
                    lines[i].text:move(0.1,
                    0.1 + y + ((i - 1) * consoleLineHeight))
                end
                i = i + 1
            end
        else
            -- delete the glyph objects of all lines since they're invisible:
            local i = 1
            while i <= #lines do
                if lines[i] ~= nil then
                    if lines[i].text ~= nil then
                        lines[i].text:destroy()
                        lines[i].text = nil
                    end
                end
                i = i + 1
            end
        end
        if consoleTextObj == nil then
            consoleTextObj = blitwizard.font.text:new(
            "> " .. consoleText, "default", 1)
        end
        -- move the console input line object if it exists:
        if consoleTextObj then
            local cw,ch = blitwizard.graphics.getCameras()[1]:
            getScreenDimensions()
            local gameunitpix = blitwizard.graphics.getCameras()[1]:
                gameUnitToPixels()
            cw = cw / gameunitpix

            -- shift to the right when entering very long lines:
            local shiftleft = 0        
            if consoleTextObj:width() > cw - 10/gameunitpix then
                shiftleft = consoleTextObj:width() - (cw - 10/gameunitpix)
            end

            consoleTextObj:move(0.1 - shiftleft, y + consoleHeight 
            - consoleLineHeight - 0.1)
        end
    end

    --[[--
      Open up the developer console by explicit request
      (it can also be opened by simply pressing F9).

      @function open
    ]]

    function blitwizard.console.open()
        consoleOpened = true
    end

    --[[--
      Disable the console completely. If it is currently open,
      it will be force-closed.

      You might want to do this in your final release.
      @function disable
    ]] 

    function blitwizard.console.disable()
        consoleOpened = false
        consoleDisabled = true
    end

    -- listening for keyboard events here:
    function blitwizard._onKeyDown_Templates(key)
        if key == "f9" or consoleOpened then
            if key == "f9" then
                -- toggle console
                consoleOpened = not consoleOpened
                if consoleDisabled then
                    consoleOpened = false
                end
            end
            if consoleOpened then
                if key == "backspace" then
                    -- remove one character:
                    if #consoleText > 0 then
                        consoleText = consoleText:sub(1, #consoleText - 1)
                        if consoleTextObj then
                            consoleTextObj:destroy()
                            consoleTextObj = nil
                        end
                    end
                end
                if key == "return" then
                    -- clear entered command from input bar:
                    local cmd = consoleText
                    addConsoleLine("> " .. cmd)
                    consoleText = ""
                    -- update command history:
                    commandHistory[#commandHistory] = cmd
                    commandHistory[#commandHistory+1] = ""
                    inCommandHistory = #commandHistory
                    if consoleTextObj then
                        consoleTextObj:destroy()
                        consoleTextObj = nil
                    end
                    -- run the entered command:
                    local success,msg = pcall(function() dostring(cmd) end)
                    -- print error if it failed:
                    if not success then
                        print("Error: " .. msg)
                    end
                end
                if key == "up" then
                    -- go to older command history entries
                    commandHistory[inCommandHistory] = (consoleText or "")
                    inCommandHistory = inCommandHistory - 1
                    if inCommandHistory < 1 then
                        inCommandHistory = 1
                    end
                    consoleText = commandHistory[inCommandHistory]
                    if consoleTextObj then
                        consoleTextObj:destroy()
                        consoleTextObj = nil
                    end
                    return true
                end
                if key == "down" then
                    -- go to newer command history entries
                    commandHistory[inCommandHistory] = (consoleText or "")
                    inCommandHistory = inCommandHistory + 1
                    if inCommandHistory > #commandHistory then
                        inCommandHistory = #commandHistory
                    end
                    consoleText = commandHistory[inCommandHistory]
                    if consoleTextObj then
                        consoleTextObj:destroy()
                        consoleTextObj = nil
                    end
                    return true
                end
            end
            return true
        end
    end
    function blitwizard._onKeyUp_Templates(key)
        -- swallow f9 on key up too:
        if key == "f9" or consoleOpened then
            if consoleOpened then
            end
            return true
        end
    end

    -- process text input here:
    function blitwizard._onText_Templates(text)
        if consoleOpened then
            -- if console is open, append text:
            consoleText = consoleText .. text
            if consoleTextObj then
                consoleTextObj:destroy()
                consoleTextObj = nil
            end
            -- this text is for the console
            return true
        end
    end
end
