--[[
blitwizard.net.irc
Under the zlib license:

Copyright (c) 2012-2013 Jonas Thiem

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required. 

2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.

]]

--[[--
 This is the IRC module which allows you to connect to the popular
 <a href="http://en.wikipedia.org/wiki/IRC">Internet Relay Chat</a>.

 You can add ingame chat rooms to your game quite easily using this
 module.
 @module blitwizard.net.irc
]]


blitwizard.net.irc = {}

--[[--
 This is the IRC connection class type which embodies an IRC server
  connection.

 You can create as many as you like if you want to connect to
 multiple servers at once.
 @type connection
]]
blitwizard.net.irc.connection = {}

function blitwizard.net.irc.connection:new(name, callback_on_event,
nickname, port)
    --  
    --  *** Blitwizard IRC interface ***
    --
    -- Check the generated documentation for usage.

    local server = {
        connection = "",
        nickname = "",
        channels = {}
    }

    -- enforce proper server name:
    if (type(name) ~= "string") then
        error("bad argument #1 to blitwizard.net.irc.connection:new: " ..
        "provided server name is not a string")
    end

    if port == nil or type(port) ~= "number" or port < 1 or port > 33000 then
        port = 6667
    end
    port = math.floor(port)

    if type(callback_on_event) ~= "function" then
        error("bad argument #2 to blitwizard.net.irc.connection:new: " ..
        "provided callback function is not a function")
    end

    if type(nickname) ~= "string" then
        error("bad argument #3 to blitwizard.net.irc.connection:new: " ..
        "provided nickname is not a string")
    end

    -- set proper meta table settings:
    local metatb = {
        __index = blitwizard.net.irc.connection
    }
    setmetatable(server, metatb)

    server.nickname = nickname
    server.connection = 
    blitwizard.net.open({server=name, port=port, linebuffered=true},
        function(stream)
            blitwizard.net.send(stream,
            "Nick " .. nickname .. "\nUser " .. nickname ..
            " 0 0 :" .. nickname .. "\n")
            server.nickname = nickname
        end,
        function(stream, line)
            callback_on_event(server, "raw", line)
            -- split up IRC arguments:
            local args = {string.split(line, " ")}
            local i = 1
            local argc = #args
            while i <= argc do
                local start = 1
                if i == 1 then
                    start = 2
                end
                if string.find(args[i], ":", start) ~= nil then
                    -- we got one final arg
                    local j = i
                    local finalarg = ""
                    -- get all following args as one single thing
                    while j <= #args do
                        finalarg = finalarg .. args[j]
                        if j < #args then
                            finalarg = finalarg .. " "
                        end
                        j = j + 1
                    end
                    -- split it up correctly
                    args[i],args[i+1] = string.split(finalarg,":",1)
                    if #args[i] <= 0 then
                        args[i] = args[i+1]
                        i = i - 1
                    end
                    -- nil/remove all the following args
                    i = i+2
                    while i <= argc do
                        args[i] = nil
                        i = i + 1
                    end
                    break
                end
                i = i + 1
            end
            -- parse IRC numerics ere:
            if string.starts(args[1], ":") and #args >= 2 then
                local actor = string.split(string.sub(args[1], 2), "!", 1)
                if args[2] == "001" then
                    server.connected = true
                    callback_on_event(server, "connect")
                end
                if args[2] == "366" then
                    -- end of names for channel args[4]
                    if server.channels[args[4]] then
                        if server.channels[args[4]].joinDelayed then
                            -- fire delayed join event
                            server.channels[args[4]].joinDelayed = false
                            callback_on_event(server, "enter",
                            args[4], server.nickname)
                        end
                    end
                end
                if string.lower(args[2]) == "join" then
                    -- remember we are inside this channel if this is us:
                    if string.lower(actor) == string.lower(server.nickname) then
                        if not server.channels[args[3]] then
                            server.channels[args[3]] = {}
                            server.channels[args[3]].joinDelayed = true
                            server.channels[args[3]].delayedEvents = {}
                        end
                        -- don't emit a join event yet. we want for
                        -- the NAMES info first so we can present a user list.
                    else
                        -- a regular join by someone else:
                        if server.channels[args[3]] ~= nil then
                            if not server.channels[args[3]].joinDelayed
                            then
                                -- we are inside the room! -> deliver event
                                callback_on_event(server, "enter",
                                args[3], actor)
                            else
                                -- channel join was delayed (waiting for
                                -- NAMES).
                                -- store event for now:
                                local events = server.channels[args[3]].
                                    delayedEvents
                                events[#events+1] = { "enter", args[3],
                                actor }
                            end
                        end
                    end
                end
                if string.lower(args[2]) == "privmsg" then
                    if #args[4] > 4 then
                        -- check if CTCP action:
                        if string.sub(string.lower(args[4]), 1, #"xaction ")
                         == "\001action " and
                        string.sub(args[4], #args[4]) == "\001" then
                            -- This is a CTCP action
                            args[4] = string.sub(args[4], #"xaction x")
                            if #args[4] > 1 then
                                args[4] = string.sub(args[4], 1, #args[4] - 1)
                            else
                                args[4] = ""
                            end
                            if server.channels[args[3]] ~= nil then
                                if not server.channels[args[3]].joinDelayed
                                then
                                    -- we are inside the room!
                                    callback_on_event(server, "roomAction",
                                    args[3], actor, args[4])
                                else
                                    -- channel join was delayed (waiting for
                                    -- NAMES).
                                    -- store event for now:
                                    local events = server.channels[args[3]].
                                        delayedEvents
                                    events[#events+1] = { "roomAction", args[3],
                                    actor, args[4] }
                                end
                            end
                            return
                        end
                    end
                    -- if on channel, send channel talk event:
                    if server.channels[args[3]] ~= nil then
                        if not server.channels[args[3]].joinDelayed then
                            -- we're on that channel! send talk event:
                            callback_on_event(server, "roomTalk", args[3],
                            actor, args[4])
                        else
                            -- channel join is delayed (to wait for NAMES).
                            -- store event to emit it later:
                            local events = server.channels[args[3]].
                                delayedEvents
                            events[#events+1] = { "roomTalk", args[3],   
                            actor, args[4] }
                        end
                    end
                end
            end
            if string.lower(args[1]) == "ping" then
                blitwizard.net.send(stream, "PONG :" .. args[2] .. "\n")
            end
        end,
        function(stream, errmsg)
            callback_on_event(server, "disconnect")
        end
    )
    return server
end

--[[--
  Create a new IRC instance. (see usage example below!)
  @function new
  @tparam string server the address of the server, e.g. irc.eloxoph.de
  @tparam function callback the event callback. It will be called with first parameter being this IRC instance, the second one the event name and all additional parameters depending on the event.

  The following events are known:

  "connect": the connection was successfully established. Use @{blitwizard.net.irc.connection:enterRoom|connection:enterRoom} to enter a chat room now!

  "enter": parameters: room, user name. The user with the given name entered the specified chat room. <b>If the user name is yours (@{blitwizard.net.irc.connection:getUsername|connection:getUsername}), this means you are inside the chat room now</b> and you may participate in conversations using @{blitwizard.net.irc.connection:talkToRoom|connection:talkToRoom} or @{blitwizard.net.irc.connection:doRoomAction|connection:doRoomAction}.

  @tparam number username the desired user name to be used on the chat server
  @tparam port (optional) specify the server port. Defaults to 6667, the most commonly used IRC server port
  @usage
  -- connect to the #blitwizard chat room:
  myIRC = blitwizard.net.irc.connection:new("irc.eloxoph.de",
  function(self, event, param1, param2, param3)
      if event == "connect" then
          print("Connected! Will now enter chat room...")
          self:enterRoom("#blitwizard")
      elseif event == "enter" then
          if string.lower(param1) == "#blitwizard" and
          param2 == self:getUsername() then
              print("Now inside #blitwizard! Say \"Hello World!\"...")
              self:talkToRoom("#blitwizard", "Hello World!")
          end
       elseif event == "roomTalk" and
        string.lower(param1) == "#blitwizard" then
           -- a person talked in #blitwizard! display it!
           print("<" .. param2 .. "> " .. param3)
       elseif event == "roomAction" and
        string.lower(param1) == "#blitwizard" then
           -- a person did an 'action' in #blitwizard. display it:
           print("<" .. param2 .. " " .. param3 .. ">")
       end
  end, "testUser")
]]

--[[--
  Close an IRC connection.
  @function close
]]
function blitwizard.net.irc.connection:quit(reason)
    if reason == nil then
        reason = "Bye"
    end
    blitwizard.net.send(self.connection, "QUIT :" .. reason .. "\n")
    blitwizard.net.close(self.connection)
    self.connection = nil
end

--[[--
   Get the user name currently used on the chat server.
   @function getUsername
   @treturn string the user name currently used
]]
function blitwizard.net.irc.connection:getUsername()
    return self.nickname
end

--[[--
   Join a specific chat room
   @function enterRoom
   @tparam string room the name of the chat room. Please note that on most servers, the name is required to start with #. Space chars are not allowed.
]]
function blitwizard.net.irc.connection:enterRoom(channel)
    if self.connected ~= true then
        error("not connected to server")
    end
    if channel == nil then
        error("bad argument #1 to blitwizard.net.irc.connection:enterRoom: " ..
        "string expected")
    end
    channel = channel:gsub("\n", " ")
    channel = channel:gsub("\r", " ")
    channel = string.split(channel, " ", 1)
    channel = channel:gsub(",", "")
    blitwizard.net.send(self["connection"], "JOIN " .. channel .. "\n")
end

--[[--
   Talk to a specific person (in private chat).

   Please note offline chat is not supported (message will not arrive
   if user is offline!)
   @function talkToPerson
   @tparam string username the user name of the person to talk to
   @tparam string msg the message you want to send to the person
]]
function blitwizard.net.irc.connection:talkToPerson(nickname, msg)
    blitwizard.net.send(self["connection"],
    "PRIVMSG " .. nickname .. " :" .. string.split(
        string.split(msg, "\n", 2)[1], "\r", 2)[1] .. "\n")
end

--[[--
   Talk to a specific chat room. Please note this is not possible
   before the "enter" event!
   @function talkToRoom
   @tparam string room the chat room to talk to
   @tparam string msg the message you want to send
]]
function blitwizard.net.irc.connection:talkToRoom(channel, msg)
    if self.channels[channel] ~= nil then
        if not self.channels[channel].joinDelayed then
            blitwizard.net.send(self["connection"], "PRIVMSG " .. channel ..
            " :" ..
            string.split(string.split(msg, "\n", 2), "\r", 2) .. "\n")
            return
        end
    end
    error("you are not inside chat room " .. channel)
end

--[[--
   Roleplay a specific action in a room, e.g. "looks outside of window"
   as an action will show up as "<Yourname> looks outside the window".

   Similar to @{blitwizard.net.irc.connection:talkToRoom|connection.talkToRoom}, this is not
   possible before entering a room and getting the "enter" event.
   @function doRoomAction
   @tparam string room the chat room you want to do the action in
   @tparam string action the description of your action
]]
function blitwizard.net.irc.connection:doRoomAction(channel, msg)
    if self.channels[channel] ~= nil then
        if not self.channels[channel].joinDelayed then
            blitwizard.net.send(self["connection"], "PRIVMSG " .. channel ..
            " :\001ACTION " ..
            string.split(string.split(msg, "\n", 2), "\r", 2) ..
            "\001\n")
            return
        end
    end
    error("you are not inside chat room " .. channel)
end



