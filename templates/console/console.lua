--[[
blitwizard.console
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

 <i>Keep in mind when releasing your game:</i> please note this module
 is implemented in the
 <b>blitwizard templates</b>, so it is unavailable if you don't ship
 the templates with your game.

 @author Jonas Thiem (jonas.thiem@gmail.com)
 @copyright 2013
 @license zlib
 @module blitwizard.console
]]

--[[
 This will load blitwizard.font twice, but that's not
 really an issue and it ensures we can use it here:
 ]]
dofile(os.templatedir() .. "/font/font.lua")

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
    local consoleHeight = 240 / blitwizard.graphics.getCameras()[1]
        :gameUnitToPixels()
    local consoleLinesShown = (function()
        local text = blitwizard.font.text:new("Hello", "default", 0.8)
        consoleLineHeight = text:height()
        local result = math.floor(consoleHeight/text:height())
        text:destroy()
        return result-2
    end)()
    local commandHistory = { "" }
    local inCommandHistory = 1

    -- blinking cursor offset (from right side)
    local blinkCursorOffset = 0

    -- create blinking cursor:
    local blinkCursorText = blitwizard.font.text:new("|", "default", 1)
    blinkCursorText:setVisible(false)
    blinkCursorText:setZIndex(10001)

    -- blinking cursor timing, text input line end in game units:
    local blinkCursorTime = 0
    local textEndX = 0 -- line end on screen

    -- lines storage:
    local lines = {}
    -- each line is { line = "text", text = blitwizard.font.text or nil }

    -- create background image for console:
    local consoleBg = blitwizard.object:new(blitwizard.object.o2d,
        os.templatedir() ..
    "/console/console.png")
    consoleBg:setZIndex(9999)
    consoleBg:setVisible(true)
    consoleBg:pinToCamera(blitwizard.graphics.getCameras()[1])
    function consoleBg:onGeometryLoaded()
        -- calculate proper size of the dev console:
        local w,h = consoleBg:getDimensions()
        local cw,ch = blitwizard.graphics.getCameras()[1]:
        getVisible2dAreaDimensions(false)
        consoleBg:setScale(cw/w, consoleHeight/h)
        consoleBg:setPosition(0, -h)
    end

    local function addConsoleLine(line)
        if line == nil then
            return
        end

        if line:find("\n") then
            local lines = {string.split(line, "\n")}
            local i = 1
            while i <= #lines do
                addConsoleLine(lines[i])
                i = i + 1
            end
            return
        end

        line = line:gsub("\r", "")

        lines[#lines+1] = { line = line, text = nil }
    end
    local function trimConsole()
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

    local printvar = function(var)
        print(tostring(var))
    end

    function blitwizard.onLog(msgtype, msg)
        addConsoleLine("[LOG:" .. msgtype .. "] " .. msg)
    end

    local oldprint = print
    function print(...)
        -- bake everything into one string:
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

        -- send stuff to stdout too:
        oldprint(str)

        -- add line to console:
        addConsoleLine(str)
    end
    addConsoleLine(_VERSION .. ", developer console")

    local function isassignment(text)
        -- see if a specific code line starts with an assignment or not
        local i = 1
        local endingWasVariable = true
        local insideQuotation = nil
        local escaped = false
        while i <= #text do
            local part = text:sub(i)

            -- handle quotations:
            if part:startswith("\"") or part:startswith("'")
            or part:startswith("[[") then
                if insideQuotation == nil then
                    -- start quotation:
                    if part:startswith("[[") then
                        insideQuotation = "]]"
                    else
                        insideQuotation = part:sub(1, 1)
                    end
                else
                    -- leave quotation:
                    if part:startswith(insideQuotation) then
                        i = i + (#insideQuotation - 1)
                        insideQuotation = nil
                    end
                end
            elseif part:startswith(";") and insideQuotation == nil then
                -- command ends here. no assignment found up to here!
                return false
            elseif (part:startswith(" end") or part:startswith(" do") or
            part:startswith(" if") or part:startswith(" function") or
            part:startswith(" for") or part:startswith(" until") or
            part:startswith(" while") or part:startswith(" return"))
            and insideQuotation == nil then
                -- next command starts here. no assignment found so far!
                return false
            elseif part:startswith("=") and insideQuotation == nil then
                if i < #text then
                    if text[i+1] == '=' then
                        -- it is a comparison wth ==
                        return false
                    else
                        -- looks like a legitimate assignment!
                        return true
                    end
                end
                return true
            elseif part:startswith("\\") and insideQuotation then
                -- skip next char since it is escaped
                i = i + 1
            end
            i = i + 1
        end
        return false
    end

    function consoleBg:doAlways()
        trimConsole()
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
                        lines[i].line, "default", 0.88)
                        lines[i].text:setZIndex(10000)
                    end
                    lines[i].text:move(0.1,
                    0.1 + y + ((i - 1) * consoleLineHeight))
                end
                i = i + 1
            end
        else
            -- hide blinking text cursor:
            blinkCursorText:setVisible(false)

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
            consoleTextObj:setZIndex(10000)
        end
        -- move the console input line object if it exists:
        if consoleTextObj then
            -- get a bit of info about screen size etc:
            local cw,ch = blitwizard.graphics.getCameras()[1]:
            getVisible2dAreaDimensions(false)

            -- shift to the right when entering very long lines:
            local shiftleft = 0        
            if consoleTextObj:width() > cw - 0.1 then
                shiftleft = consoleTextObj:width() - (cw - 0.1)
            end

            -- move console text to calculated position:
            consoleTextObj:move(0.1 - shiftleft, y + consoleHeight 
            - consoleLineHeight - 0.1)

            -- remember text ending for blinking cursor:
            local tx,ty = consoleTextObj:getPosition()
            textEndPos = tx + consoleTextObj:width()
        end

        -- handle blinking cursor
        if consoleOpened or y > -consoleHeight then
            -- draw blinking cursor
            blinkCursorTime = blinkCursorTime + 1
            if blinkCursorTime > 80 then
                blinkCursorTime = 0
            end
            if blinkCursorTime < 40 then
                -- cursor is visible
                blinkCursorText:setVisible(true)

                -- get glyph size of input line font:
                local gw,gh = consoleTextObj:getGlyphDimensions()
                blinkCursorText:move(textEndPos - blinkCursorOffset * gw
                - gw * math.min(0.4, blinkCursorOffset),
                y + consoleHeight - consoleLineHeight - 0.1)
            else
                blinkCursorText:setVisible(false)
            end
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
                    if #consoleText > 0 and
                    blinkCursorOffset < #consoleText then
                        consoleText = consoleText:sub(1,
                        #consoleText - blinkCursorOffset - 1) ..
                        consoleText:sub(#consoleText - blinkCursorOffset +1)
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
                    if #cmd > 0 then
                        commandHistory[#commandHistory] = cmd
                        commandHistory[#commandHistory+1] = ""
                        inCommandHistory = #commandHistory
                    end
                    if consoleTextObj then
                        consoleTextObj:destroy()
                        consoleTextObj = nil
                    end
                    blinkCursorOffset = 0
                    -- run the entered command:
                    local result = nil
                    local resultcount = 0;
                    local success,msg = pcall(
                    function()
                        -- extract first possible keyword from cmd
                        local k = string.split(cmd, " ")
                        k = string.split(k, "(")
                        -- check if it is a known lua keyword:
                        local iskeyword = false
                        if k == "if" or k == "while" or k == "then"
                        or k == "for" or k == "return" or k == "end"
                        or k == "local" or k == "repeat" or k == "until"
                        or k == "break" then
                            iskeyword = true
                        end
                        -- if it is not a known keyword, prepend with
                        -- return to get return value:
                        if not iskeyword and not isassignment(cmd) then
                            cmd = "return " .. cmd
                        end
                        -- execute command:
                        resultcount, result =
                        dostring_returnvalues(cmd)
                    end
                    )

                    -- if that worked, print return value if present:
                    if success and resultcount > 0 then
                        printvar(result)
                    end

                    -- if that didn't work, see if we can do other things:
                    if not success then
                        if _G[cmd] then  -- print global var
                            printvar(_G[cmd])
                            success = true
                        end
                    end

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
                    blinkCursorOffset = 0
                    return true
                end
                if key == "left" then
                    blinkCursorOffset = blinkCursorOffset + 1
                    if blinkCursorOffset > #consoleText then
                        blinkCursorOffset = #consoleText
                    end
                    blinkCursorTime = 0
                end
                if key == "right" then
                    blinkCursorOffset = blinkCursorOffset - 1
                    if blinkCursorOffset < 0 then
                        blinkCursorOffset = 0
                    end
                    blinkCursorTime = 0
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
                    blinkCursorOffset = 0
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

            consoleText = consoleText:sub(1, #consoleText - blinkCursorOffset)
             .. text .. consoleText:sub(#consoleText + 1 - blinkCursorOffset)
            if consoleTextObj then
                consoleTextObj:destroy()
                consoleTextObj = nil
            end
            -- this text is for the console
            return true
        end
    end
end



