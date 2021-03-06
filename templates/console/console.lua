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
    local updateInputLine = false
    local consoleText = ""
    local consoleTextObj = nil
    local consoleOpened = false
    local slideSpeed = 0.1
    local consoleDisabled = false
    local consoleLineHeight = 0
    local consoleHeight = 240 / 40 -- 40 should be roughly gameUnitToPixels
    local consoleLoaded = false
    blitwizard.console.consoleLinesShown = 99

    -- lines storage:
    local lines = {}
    -- each line is { line = "text", text = blitwizard.font.text or nil }

    -- blinking cursor offset (from right side)
    local blinkCursorOffset = 0

    -- this will hold the blinking cursor once create:
    local blinkCursorText = nil

    local function trimConsole()
        if #lines >
        blitwizard.console.consoleLinesShown then
            local trimLines = (#lines - blitwizard.console.consoleLinesShown)
            -- remove first lines:
            local i = 1
            while i <= trimLines do
                if lines[i].text then
                    lines[i].text:destroy()
                end
                i = i + 1
            end
            local i = 1
            while i <= #lines - trimLines do
                lines[i] = lines[i + trimLines]
                i = i + 1
            end
            local i = 1
            while i <= trimLines do
                table.remove(lines, #lines)
                i = i + 1
            end
        end
    end
    local function calculateConsoleLinesShown()
        local text = blitwizard.font.text:new(
            "H", "default", 0.8)
        local result = 0
        if pcall(function()
            consoleLineHeight = text:height()
            result = math.floor(
                consoleHeight/text:height())
        end) ~= true then
            text:destroy()
            error("couldn't calculate lines")
        end
        text:destroy()
        blitwizard.console.consoleLinesShown = result-2
        trimConsole()
    end
    local commandHistory = { "" }
    local inCommandHistory = 1

    local function createBlinkingCursor()
        -- create the blinking cursor
        blinkCursorText = blitwizard.font.text:new("|", "default", 0.7)
        blinkCursorText:setVisible(false)
        blinkCursorText:setZIndex(10001)
        blinkCursorText:_markAsTemplateObj()
    end

    -- blinking cursor timing, text input line end in game units:
    local blinkCursorTime = 0
    local textEndX = 0 -- line end on screen

    -- create background image for console:
    local consoleBg = blitwizard.object:new(blitwizard.object.o2d,
        os.templatedir() ..
    "/console/console.png")
    consoleBg:setZIndex(9999)
    consoleBg:setVisible(true)
    consoleBg:pinToCamera()
    consoleBg:setPosition(0, -10)
    consoleBg._templateObj = true
    local function calculateConsoleBgSize()
        -- calculate proper size of the dev console:
        consoleBg:setScale(1, 1)
        local w,h = consoleBg:getDimensions()
        local cw,ch = blitwizard.graphics.getCameras()[1]:
        getDimensions()
        consoleBg:setScale(cw/w, consoleHeight/h)
    end
    function consoleBg:onGeometryLoaded()
        consoleBg:setVisible(false)
        pcall(calculateConsoleBgSize)
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

    local printvar = function(var)
        print(tostring(var))
    end

    --function blitwizard.onLog(msgtype, msg)
    --    addConsoleLine("[LOG:" .. msgtype .. "] " .. msg)
    --end

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
                        insideQuotation = "]" .. "]"
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
            -- this will work as soon as blitwizard.graphics.setMode
            -- has been called for first time:
            local f = function()
                consoleHeight = 240 / blitwizard.graphics.gameUnitToPixels()
            end
            if pcall(f) == true then
                calculateConsoleLinesShown()
                pcall(calculateConsoleBgSize)
                if consoleLoaded == false then
                    consoleLoaded = true
                    createBlinkingCursor()
                end
                consoleBg:setVisible(true)
            end

            local x,y = consoleBg:getPosition()
            if y < 0 then -- slide down
                y = y + slideSpeed
                if y > 0 then
                    y = 0
                end
            end
            if y < -consoleHeight then
                y = -consoleHeight
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
            if y > 0 then
                y = 0
            end
            consoleBg:setPosition(x, y)
        end

        -- place lines properly:
        local x,y = consoleBg:getPosition()
        if consoleOpened or (y > -consoleHeight + 1.0) then
            -- Cycle through lines, load their text glyphs and move them:
            local i = 1
            while i <= #lines do
                if lines[i] ~= nil then
                    if lines[i].text == nil then
                        -- load glyphs
                        local newfontobj = nil
                        pcall(function()
                            newfontobj = blitwizard.font.text:new(
                            lines[i].line, "default", 0.7)
                            newfontobj:setZIndex(10000)
                            newfontobj:_markAsTemplateObj()
                        end)
                        lines[i].text = newfontobj
                    end
                    if lines[i].text then
                        lines[i].text:setPosition(0.1,
                        0.1 + y + ((i - 1) * consoleLineHeight))
                    end
                end
                i = i + 1
            end
        else
            -- hide blinking text cursor:
            if blinkCursorText ~= nil then
                blinkCursorText:setVisible(false)
            end

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
        if consoleOpened or y > -consoleHeight then
            if updateInputLine then
                updateInputLine = false
                consoleTextObj:destroy()
                consoleTextObj = nil
            end
            if consoleTextObj == nil then
                local textobj = nil
                pcall(function()
                    textobj = blitwizard.font.text:new(
                    "> " .. consoleText, "default", 0.7)
                    textobj:setZIndex(10000)
                    textobj:_markAsTemplateObj()
                end)
                consoleTextObj = textobj
            end
            -- move the console input line object if it exists:
            if consoleTextObj then
                -- get a bit of info about screen size etc:
                local cw,ch = blitwizard.graphics.getCameras()[1]:
                getDimensions()

                -- shift to the right when entering very long lines:
                local shiftleft = 0        
                if consoleTextObj:width() > cw - 0.1 then
                    shiftleft = consoleTextObj:width() - (cw - 0.1)
                end

                -- move console text to calculated position:
                consoleTextObj:setPosition(0.1 - shiftleft, y + consoleHeight 
                - consoleLineHeight - 0.1)

                -- remember text ending for blinking cursor:
                local tx,ty = consoleTextObj:getPosition()
                textEndPos = tx + consoleTextObj:width()
            end
        else
            if consoleTextObj then
                consoleTextObj:destroy()
                consoleTextObj = nil
            end
        end

        -- handle blinking cursor
        if (consoleOpened or y > -consoleHeight)
        and blinkCursorText ~= nil then
            -- draw blinking cursor
            blinkCursorTime = blinkCursorTime + 1
            if blinkCursorTime > 80 then
                blinkCursorTime = 0
            end
            if blinkCursorTime < 40 then
                -- cursor is visible
                blinkCursorText:setVisible(true)

                -- get glyph size of input line font:
                local gw,gh = 0.2, 0.2
                if consoleTextObj then
                    gw,gh = consoleTextObj:getGlyphDimensions()
                end
                blinkCursorText:setPosition((textEndPos
                    or consoleBg:getDimensions()) - blinkCursorOffset * gw
                    - gw * math.min(0.4, blinkCursorOffset),
                y + consoleHeight - consoleLineHeight - 0.1)
            else
                blinkCursorText:setVisible(false)
            end
        else
            if blinkCursorText then
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
    
    --[[--
      Print something to the console (works similar to Lua's print)
      @function print
    ]] 

    function blitwizard.console.print(str)
        addConsoleLine(str)
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
                if key ~= "f9" then
                    if key[1] == "f" and #key >= 2 then
                        return false
                    end
                end
                if key == "escape" then
                    return false
                end
                if key == "backspace" then
                    -- remove one character:
                    if #consoleText > 0 and
                    blinkCursorOffset < #consoleText then
                        consoleText = consoleText:sub(1,
                        #consoleText - blinkCursorOffset - 1) ..
                        consoleText:sub(#consoleText - blinkCursorOffset +1)
                        updateInputLine = true
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
                    updateInputLine = true
                    blinkCursorOffset = 0
                    -- run the entered command:
                    local results = nil
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
                        local values =
                        {dostring_returnvalues(cmd)}
                        -- analyse results:
                        resultcount = values[1]
                        if resultcount > 0 then
                            results = {}
                            local i = 2
                            while i <= resultcount + 1 do
                                results[i-1] = values[i]
                                i = i + 1
                            end
                        end
                    end
                    )

                    -- if that worked, print return value if present:
                    if success and resultcount > 0 then
                        -- print list of all results:
                        local s = ""
                        local i = 1
                        while i <= resultcount do
                            if i > 1 then
                                s = s .. ", "
                            end
                            local f = function()
                                s = s .. tostring(results[i])
                            end
                            pcall(f)
                            i = i + 1
                        end
                        printvar(s)
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
                    updateInputLine = true
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
                    updateInputLine = true
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

            consoleText = consoleText:sub(1,
                #consoleText - blinkCursorOffset)
             .. text .. consoleText:sub(
                #consoleText + 1 - blinkCursorOffset)
            updateInputLine = true
            -- this text is for the console
            return true
        end
    end
end

dofile(os.templatedir() .. "/console/reload.lua")

