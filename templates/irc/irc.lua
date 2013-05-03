--[[-----
blitwizard.net.irc
Under the zlib license:

Copyright (c) 2012-2013 Jonas Thiem

This software is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications, and to alter it and redistribute it freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim that you wrote the original software. If you use this software in a product, an acknowledgment in the product documentation would be appreciated but is not required. 

2. Altered source versions must be plainly marked as such, and must not be misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.

--]]-----

blitwizard.net.irc = {}

blitwizard.net.irc.open = function(name, port, nickname, callback_on_event)
    --  
    --  *** Blitwizard IRC interface ***
    --
    --  Read on for documentation.
    --
    -- This opens a new blitwizard IRC interface instance to an IRC server
    -- (one instance can handle one server). It returns an IRC object.
    --
    -- callback_on_event needs to be a lua function provided by you which
    -- gets called with event infos as soon as something happens.
    --
    -- callback_on_event gets the irc object as first parameter,
    -- an event name as a second parameter,
    -- and additional parameters depending on the event type.
    --
    -- Once you get the "connect" event, use joinChannel to enter chat rooms
    -- (=channels).
    --
    -- Event types:
    --     "connect": parameters: irc
    --                The given IRC instance has connected to the server.
    --                Now you might want to use
    --                irc:joinCannel("#somechannel")
    --     "join": parameters: irc, channel, nickname
    --             Someone with the given nickname joined the given channel.
    --             If it is yourself, you'll get a "names" event shortly after.
    --             You can use this to speak in a channel you joined:
    --               irc:talkToChannel(channel, "hello")
    --             You can obtain a list of users like this:
    --               irc:getChannelUsers(channel)
    --     "leave": parameters: irc, channel, nickname, reason
    --              Someone with the given nickname left the given channel.
    --              The reason can contain a free text string specifying
    --              a reason.
    --              This can also be yourself.
    --              You might want to update your user list with
    --                irc:getChannelUsers(channel)
    --     "quit": parameters: irc, nickname, reason
    --             Someone with the given nickname disconnected (-> leaves all
    --             channels). The reason can contain a free text string
    --             specifying a reason.
    --             You might want to update your user list with:
    --                irc:getChannelUsers(channel)
    --     "channelTalk": parameters: irc, channel, nickname, message
    --                       Someone said something in a channel.
    --                       message is a string with the message.
    --                       This should show up e.g. like that:
    --                         <nickname> hello
    --                       (with "hello" being the message)
    --     "channelAction": same as channel message, but indicates it is a
    --                      /me action of the user
    --                      This should show up e.g. like that:
    --                        <nickname does something>
    --                      (with "does something" being the message)
    --     "privateTalk": parameters: nickname, message
    --                       Someone said something privately to you.
    --                       You might want to respond with
    --                       irc:talkToPerson(nickname, "hello friend")
    --     "privateAction": same as private message, but indiciates it is a
    --                      /me action of the user
    --                      (see channelAction)
    --     "disconnect": parameters: irc
    --                     You were disconnected from the server.
    --                     You should use irc:quit() and leave the irc object
    --                     alone.
    --
    --  Additionally, you might be interested in those functions sitting
    --  on the IRC object returned:
    --
    --   irc:joinChannel(channel)
    --     Join the channel named by the specific string.
    --   irc:leaveChannel(channel)
    --     Leave the specified channel again.
    --   irc:talkToChannel(channel, message)
    --     Talk in a given channel. Please note on most servers,
    --     you need to be inside the channel to do that.
    --   irc:talkToPerson(channel, nickname)
    --     Talk to a person with the given nickname in private.
    --     Please note offline messages are not supported
    --     (the message won't go anywhere if that person is offline)
    --   irc:getChannelUsers(channel)
    --     Return an array of strings with the names of all users
    --     inside the given channel.
    --     Please note this will error if you're not inside the
    --     specified cannel.
    --   irc:getNickname(server)
    --    Get the nickname you have on the given server/blitwizard IRC instance
    --   irc:quit(server, reason)
    --    Leave the server with the specified reason (a text message/string,
    --    e.g. "Goodbye")
    --

    local server = {
        connection = "",
        nickname = "",
        channels = {}
    }

    -- enforce proper server name:
    if (type(name) ~= "string") then
        error("bad argument #1 to blitwizard.net.irc.open: " ..
        "provided server name is not a string")
    end

    -- set proper meta table settings:
    local metatb = {
        __index = blitwizard.net.irc
    }
    setmetatable(server, metatb)

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
                    callback_on_event(server, "connect")
                end
                if string.lower(args[2]) == "join" then
                    -- remember we are inside this channel if this is us:
                    if string.lower(actor) == string.lower(self.nickname) then
                        if not server.channels[args[3]] then
                            server.channels[args[3]] = {}
                            server.channels[args[3]].joinDelayed = true
                            server.channels[args[3]].delayedEvents = {}
                        end
                        -- don't emit a join event yet. we want for
                        -- the NAMES info first so we can present a user list.
                    else
                        -- a regular join by someone else:
                        callback_on_event(server, "join", args[3], actor)
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
                            callback_on_event(server, "channelAction",
                            args[3], actor, args[4])
                            return
                        end
                    end
                    -- if on channel, send channel talk event:
                    if server.channels[args[3]] ~= nil then
                        if not server.channels[args[3]].joinDelayed then
                            -- we're on that channel! send talk event:
                            callback_on_event(server, "channelTalk", args[3],
                            actor, args[4])
                        else
                            -- channel join is delayed (to wait for NAMES).
                            -- store event to emit it later:
                            local events = server.channels[args[3]].
                                delayedEvents
                            events[#events+1] = { "channelTalk", args[3],   
                            actor, args[4] }
                        end
                    end
                end
            end
            if string.lower(args[1]) == "ping" then
                blitwiz.net.send(stream, "PONG :" .. args[2] .. "\n")
            end
        end,
        function(stream, errmsg)
            callback_on_event(server, "disconnect")
        end
    )
    return server
end

function blitwizard.net.irc:quit(reason)
    if reason == nil then
        reason = "Bye"
    end
    blitwizard.net.send(self.connection, "QUIT :" .. reason .. "\n")
    blitwizard.net.close(self.connection)
    self.connection = nil
end

function blitwizard.net.irc:getNickname(server)
    return self.nickname
end

function blitwizard.net.irc:joinChannel(channel)
    blitwizard.net.send(self["connection"], "JOIN " .. channel .. "\n")
end

function blitwizard.net.irc:talkToPerson(nickname, msg)
    blitwizard.net.send(self["connection"],
    "PRIVMSG " .. nickname .. " :" .. string.split(
        string.split(msg, "\n", 2)[1], "\r", 2)[1] .. "\n")
end

function blitwizard.net.irc:talkToChannel(channel, msg)
    blitwizard.net.send(self["connection"], "PRIVMSG " .. channel .. " :" ..
    string.split(string.split(msg, "\n", 2)[1], "\r", 2)[1] .. "\n")
end

function blitwizard.net.irc:doChannelAction(channel, msg)
    blitwizard.net.send(self.connection, "PRIVMSG " .. channel ..
    " :\001ACTION " .. string.split(string.split(msg, "\n", 2)[1], "\r", 2)[1]
     .. "\001\n")
end



